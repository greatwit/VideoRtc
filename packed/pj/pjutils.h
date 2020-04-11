
#ifndef __PJMEDIA_UTILS_H__
#define __PJMEDIA_UTILS_H__

#include <string.h>
#include <assert.h>
#include <pj/types.h>

#ifdef __GLIBC__
#   define PJ_HAS_BZERO		1
#endif

//#define PJ_INLINE_SPECIFIER	static __inline
//#define PJ_INLINE(type)	  PJ_INLINE_SPECIFIER type





//#define bzero(dst, size) memset(dst, 0, size)
/**
 * Fill the memory location with zero.
 *
 * @param dst	    The destination buffer.
 * @param size	    The number of bytes.
 */
PJ_INLINE(void) pj_bzero(void *dst, pj_size_t size)
{
    #if defined(PJ_HAS_BZERO) && PJ_HAS_BZERO!=0
        bzero(dst, size);
    #else
        memset(dst, 0, size);
    #endif
}



#endif