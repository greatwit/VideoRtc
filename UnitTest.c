
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>

#include "h264.h"
#include "basedef.h"

#include <h264_packetizer.h>



#define	FILE_PATH	"bare720p.h264"

#ifndef PJMEDIA_MAX_MTU			
#  define PJMEDIA_MAX_MTU			1500
#endif

#ifndef PJMEDIA_MAX_VID_PAYLOAD_SIZE			
#  define PJMEDIA_MAX_VID_PAYLOAD_SIZE		(PJMEDIA_MAX_MTU - 500)
#endif

typedef unsigned char             WebRtc_UWord8;
typedef signed int              WebRtc_Word32;
typedef unsigned int            WebRtc_UWord32;

WebRtc_UWord32*   _moment1 = NULL;           // (Q8) First order moment (mean)
WebRtc_UWord32*   _moment2 = NULL;           // (Q8) Second order moment
WebRtc_UWord32    _frameSize = 0;         // Size (# of pixels) of frame
WebRtc_Word32     _denoiseFrameCnt = 0;   // Counter for subsampling in time
enum { kSubsamplingTime = 0 };       // Down-sampling in time (unit: number of frames)
enum { kSubsamplingWidth = 0 };      // Sub-sampling in width (unit: power of 2)
enum { kSubsamplingHeight = 0 };     // Sub-sampling in height (unit: power of 2)
enum { kDenoiseFiltParam = 179 };    // (Q8) De-noising filter parameter
enum { kDenoiseFiltParamRec = 77 };  // (Q8) 1 - filter parameter
enum { kDenoiseThreshold = 19200 };  // (Q8) De-noising threshold level
WebRtc_Word32 ProcessFrame(WebRtc_UWord8* frame,
                           const WebRtc_UWord32 width,
                           const WebRtc_UWord32 height)
{
    WebRtc_Word32     thevar;
    WebRtc_UWord32    k;
    WebRtc_UWord32    jsub, ksub;
    WebRtc_Word32     diff0;
    WebRtc_UWord32    tmpMoment1;
    WebRtc_UWord32    tmpMoment2;
    WebRtc_UWord32    tmp;
    WebRtc_Word32     numPixelsChanged = 0;

    if (frame == NULL)
    {
        //WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id, "Null frame pointer");
        return -1;
    }

    if (width == 0 || height == 0)
    {
        //WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoPreocessing, _id, "Invalid frame size");
        return -1;
    }

    /* Size of luminance component */
    const WebRtc_UWord32 ysize  = height * width;

    /* Initialization */
    if (ysize != _frameSize)
    {
        free(_moment1);
        _moment1 = NULL;

        free(_moment2);
        _moment2 = NULL;
    }
    _frameSize = ysize;

    if (!_moment1)
    {
        _moment1 = (WebRtc_UWord32*)malloc(ysize*sizeof(WebRtc_UWord32));
        memset(_moment1, 0, sizeof(WebRtc_UWord32)*ysize);
    }
    
    if (!_moment2)
    {
        _moment2 = (WebRtc_UWord32*)malloc(ysize*sizeof(WebRtc_UWord32));
        memset(_moment2, 0, sizeof(WebRtc_UWord32)*ysize);
    }

    /* Apply de-noising on each pixel, but update variance sub-sampled */
	WebRtc_UWord32 i = 0;
    for (; i < height; i++)
    { // Collect over height
        k = i * width;
        ksub = ((i >> kSubsamplingHeight) << kSubsamplingHeight) * width;
		WebRtc_UWord32 j = 0;
        for (; j < width; j++)
        { // Collect over width
            jsub = ((j >> kSubsamplingWidth) << kSubsamplingWidth);
            /* Update mean value for every pixel and every frame */
            tmpMoment1 = _moment1[k + j];
            tmpMoment1 *= kDenoiseFiltParam; // Q16
            tmpMoment1 += ((kDenoiseFiltParamRec * ((WebRtc_UWord32)frame[k + j])) << 8);
            tmpMoment1 >>= 8; // Q8
            _moment1[k + j] = tmpMoment1;

            tmpMoment2 = _moment2[ksub + jsub];
            if ((ksub == k) && (jsub == j) && (_denoiseFrameCnt == 0))
            {
                tmp = ((WebRtc_UWord32)frame[k + j] * (WebRtc_UWord32)frame[k + j]);
                tmpMoment2 *= kDenoiseFiltParam; // Q16
                tmpMoment2 += ((kDenoiseFiltParamRec * tmp)<<8);
                tmpMoment2 >>= 8; // Q8
            }
            _moment2[k + j] = tmpMoment2;
            /* Current event = deviation from mean value */
            diff0 = ((WebRtc_Word32)frame[k + j] << 8) - _moment1[k + j];
            /* Recent events = variance (variations over time) */
            thevar = _moment2[k + j];
            thevar -= ((_moment1[k + j] * _moment1[k + j]) >> 8);
            /***************************************************************************
             * De-noising criteria, i.e., when should we replace a pixel by its mean
             *
             * 1) recent events are minor
             * 2) current events are minor
             ***************************************************************************/
            if ((thevar < kDenoiseThreshold)
                && ((diff0 * diff0 >> 8) < kDenoiseThreshold))
            { // Replace with mean
                frame[k + j] = (WebRtc_UWord8)(_moment1[k + j] >> 8);
                numPixelsChanged++;
            }
        }
    }

    /* Update frame counter */
    _denoiseFrameCnt++;
    if (_denoiseFrameCnt > kSubsamplingTime)
    {
        _denoiseFrameCnt = 0;
    }

    return numPixelsChanged;
}

void threadFunc( void *arg ) {
	long seconds = (long) arg;
}

void h264_test(pj_uint8_t *bits, pj_size_t bits_len) {
	static pjmedia_h264_packetizer *pktz;
	if(!pktz) {
		pktz = (pjmedia_h264_packetizer*)malloc(sizeof(pjmedia_h264_packetizer));
		memset(pktz, 0, sizeof(pjmedia_h265_packetizer));
		pktz->cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
		pktz->cfg.mtu  = PJMEDIA_MAX_VID_PAYLOAD_SIZE;
	}

	pj_size_t payload_len = 0;
	unsigned int	enc_packed_pos = 0;
	pj_uint8_t payload[PJMEDIA_MAX_VID_PAYLOAD_SIZE] = {0};
	while(enc_packed_pos<bits_len) {
		pj_status_t res =  pjmedia_h264_packetize(pktz,
										bits,
										bits_len,
										&enc_packed_pos,
										payload,
										&payload_len);
		GLOGE("------get size:%d payload_len:%d enc_packed_pos:%d indicator:%02X\n", 
			bits_len, payload_len, enc_packed_pos, payload[1]);
	}

}

void h265_test(pj_uint8_t *bits, pj_size_t bits_len) {
	static pjmedia_h265_packetizer *pktz;
	if(!pktz) {
		pktz = (pjmedia_h265_packetizer*)malloc(sizeof(pjmedia_h265_packetizer));
		memset(pktz, 0, sizeof(pjmedia_h265_packetizer));
		//pktz->cfg.mode = PJMEDIA_H264_PACKETIZER_MODE_NON_INTERLEAVED;
		pktz->cfg.mtu  = PJMEDIA_MAX_VID_PAYLOAD_SIZE;
	}

	pj_size_t payload_len = 0;
	unsigned int	enc_packed_pos = 0;
	pj_uint8_t payload[PJMEDIA_MAX_VID_PAYLOAD_SIZE] = {0};
	while(enc_packed_pos<bits_len) {
		pj_status_t res =  pjmedia_h265_packetize(pktz,
										bits,
										bits_len,
										&enc_packed_pos,
										payload,
										&payload_len);
		//GLOGE("------get size:%d payload_len:%d enc_packed_pos:%d indicator:%02X\n", bits_len, payload_len, enc_packed_pos, payload[0]);
	}

}

int main( int argc, char ** argv )
{
	if ((argc != 2)) {
		printf("Usage: exe bareflow_filename.\n");
		return 0;
	}

	FILE			*mwFile;
	NALU_t 			*mNALU;

	mwFile = OpenBitstreamFile( argv[1] ); //camera_640x480.h264 //camera_1280x720.h264
	if(mwFile==NULL) {
		GLOGE("open file:%s failed.", argv[1]);
		return 0;
	}

	mNALU  = AllocNALU(8000000);
	int count = 0;

	do{
		count++;
		int size=GetAnnexbNALU(mwFile, mNALU);//每执行一次，文件的指针指向本次找到的NALU的末尾，下一个位置即为下个NALU的起始码0x000001
		//GLOGE("GetAnnexbNALU type:0x%02X size:%d count:%d\n", mNALU->buf[4], size, count);
		if(size<4) {
			GLOGE("get nul error!\n");
			continue;
		}

		h264_test(mNALU->buf, size);

	}while(!feof(mwFile));

	if(mwFile != NULL)
		fclose(mwFile);

	FreeNALU(mNALU);

	return 0;
}



