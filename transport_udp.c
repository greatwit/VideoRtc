

#include "transport_udp.h"
#include <sys/socket.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>

int thread_sleep(unsigned msec) {
    return usleep(msec *1000);
} 

struct pj_thread_t
{
    char	    obj_name[PJ_MAX_OBJ_NAME];
    pthread_t	    thread;
    pj_thread_proc *proc;
    void	   *arg;
    pj_uint32_t	    signature1;
    pj_uint32_t	    signature2;

    pj_mutex_t	   *suspended_mutex;
};

#if defined (HYT_VID_SENDTO_THREAD) ||defined (HYT_VID_RECVFROM_THREAD)
void vid_sendto_exit_deal(int arg)
{	
	 //log_debug("sendto thread quit:%p", pthread_self());
	 pthread_exit(0);
}

void vid_recv_from_exit_deal(int arg)
{	
	//log_debug("recvfrom thread quit:%p", pthread_self());
	pthread_exit(0);
}

#endif

#ifdef HYT_VID_SENDTO_THREAD

static  void  worker_thread_sendto(void *arg)
{	
	struct transport_udp *udp;
	pj_status_t status;
	rtp_sendto_thread_list_node  *new_node;
	//pj_fd_set_t  send_set;
	pj_size_t	send_len = 0;	
	sigset_t all_mask, new_mask;
	struct sigaction new_act;
	struct timeval tv_s, tv_e; //tv_f;
	unsigned short *seq;
	unsigned int loop_cnt = 0;
	
	
	// int max = pj_thread_get_prio_max(pj_thread_this());
	// if (max > 0)
	//     pj_thread_set_prio(pj_thread_this(), max);
	// log_debug("enter to worker_thread_sendto thread, prior:%d", max);


	 sigprocmask(SIG_BLOCK, NULL , &all_mask);
	 new_mask = all_mask;
	 if(1==sigismember(&all_mask, SIGUSR1))
	 {
		sigdelset(&new_mask , SIGUSR1);
		//log_debug("worker_thread_sendto sigismember SIGUSR1");
	 }
	 else
	 {
		sigaddset(&all_mask , SIGUSR1);
		 //log_debug("worker_thread_sendto all_mask add  SIGUSR1");
	 }

	 if(0==sigismember(&new_mask, SIGUSR1))
	 {
	 	 //log_debug("worker_thread_sendto sigismember new_mask no SIGUSR1");
	 }
	 
	memset(&new_act, 0, sizeof(new_act));	
	new_act.sa_handler = vid_sendto_exit_deal;	
	sigemptyset( &new_act.sa_mask);	
	sigaction(SIGUSR1, &new_act, NULL);


	new_node = NULL;
	udp = (struct transport_udp*) arg;


	unsigned optVal = 0;
	int optLen = sizeof(int);    
    unsigned nSendBuf = 2048*1024;
    unsigned nRecvBuf = 2048*1024;
	
	setsockopt(udp->rtp_sock,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));
	setsockopt(udp->rtp_sock,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
	
	getsockopt(udp->rtp_sock, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, &optLen);
	//log_debug("socket send buffer size:%d", optVal);
	getsockopt(udp->rtp_sock, SOL_SOCKET, SO_RCVBUF, (char*)&optVal, &optLen);
	//log_debug("socket recv buffer size:%d", optVal);


	// log_debug("begin to start worker_thread_sendto thread...");
	while(udp->rtp_sock != PJ_INVALID_SOCKET && !udp->sendto_thread_quit)
	{
		if(!udp->rtp_thread_list_header.list_current_send)
		{
			usleep(1000);
			loop_cnt++;
			if(!(loop_cnt%1000))
			{
				//log_debug("empty loop over 1000 times flag.");
			}
			continue;
		}
		else
		{
			loop_cnt = 0;
		}
		
	    gettimeofday(&tv_s, NULL);
		
		new_node = udp->rtp_thread_list_header.list_current_send;
		send_len = new_node->rtp_buf_size;

		if(send_len != 0)
		{
			//pj_mutex_lock(udp->udp_socket_mutex);
			status = sendto(udp->rtp_sock, new_node->rtp_buf, send_len, 0, (const struct sockaddr *)&udp->rem_rtp_addr ,udp->addr_len);
			//pj_mutex_unlock(udp->udp_socket_mutex);
			if(status <= 0)
			{
				seq =  (unsigned short *)(&new_node->rtp_buf[2]);			
				//log_debug("send rtp packet:[%u] failure, rtp seq:[%u], errno:[%d]", udp->rtp_thread_list_header.list_send_size,  pj_ntohs(*seq), status);
				usleep(5000);			
			}
			else
			{
				gettimeofday(&tv_e, NULL);
				/*if(((tv_e.sec - tv_s.sec)*1000 + (tv_e.msec - tv_s.msec)) >= 10)*/
				{
					seq =  (unsigned short *)(&new_node->rtp_buf[2]);
					//log_debug("send rtp packet:[%u] sucesss, rtp seq:[%u], marker:[%d], size:[%d], expired:[%d]ms", 
					//		udp->rtp_thread_list_header.list_send_size, pj_ntohs(*seq),  (new_node->rtp_buf[1]&0x80)?1:0,
					//		status, (tv_e.sec - tv_s.sec)*1000 + (tv_e.msec - tv_s.msec));
				}
				//pj_thread_sleep(1);
			}

			if(udp->rtp_thread_list_header.list_send_size == MAX_LOOP_NUM)
			{	
				udp->rtp_thread_list_header.list_send_size = 0;
				udp->rtp_thread_list_header.send_loop_flg = PJ_TRUE;
			}
			else
			{
				if(udp->rtp_thread_list_header.list_send_size < udp->rtp_thread_list_header.list_write_size)
				udp->rtp_thread_list_header.list_send_size++;
			} 
		}

		while(udp->rtp_sock != PJ_INVALID_SOCKET && !udp->sendto_thread_quit 
			&& !udp->rtp_thread_list_header.list_current_send && !udp->rtp_thread_list_header.list_current_send->next)
		{
			usleep(5000);
		}
		
		udp->rtp_thread_list_header.list_current_send = udp->rtp_thread_list_header.list_current_send->next;
		sigprocmask(SIG_SETMASK, &new_mask , NULL);			
		pthread_mutex_lock(&udp->rtp_cache_mutex);
		if(udp->mem_list)
		{
			memory_list_free(udp->mem_list, new_node);
			new_node = NULL;
		}
		pthread_mutex_unlock(&udp->rtp_cache_mutex);
		sigprocmask(SIG_SETMASK, &all_mask , NULL);	
	}


	// log_debug("thread end");
	return;
}

/* add by j33783 20190509 */

#endif
/** end add by t13693 for video pkg to seng whit a independent thread,2016.3.18 */

/* begin add by t13693 for video independent thread,2016.3.16 */
#ifdef HYT_VID_RECVFROM_THREAD

static  void  worker_recv_from(void *arg)
{
	    struct transport_udp *udp;
	    pj_status_t status;
	    pj_ssize_t  bytes_read = 0;
	    pj_uint32_t  value = 0;

	    fd_set  recv_set; /* add by j33783 20190621 */
	    struct timeval timeout;
 
	   struct sigaction new_act;

	   //log_debug("thread starting.....");
	     
	   memset(&new_act, 0, sizeof(new_act));
	   
	   new_act.sa_handler = vid_recv_from_exit_deal;

	   sigemptyset( &new_act.sa_mask);
	   
	   sigaction(SIGUSR2, &new_act, NULL);
   
	   
	    udp = (struct transport_udp *) arg;

	   //log_debug("udp:%0x, rtp_cb:%0x", udp, udp->rtp_cb);
		
	   //log_debug("running, %08x,%08x",pthread_self(),pj_getpid_from_pj_thread_t(udp->vid_recv_thread));  /* modify by j33783 20190509 */


	   if (ioctl(udp->rtp_sock, FIONBIO, &value)) 
	   {
	    	  //status  = pj_get_netos_error();
		  return ;
	   }

	   
	   while(udp->rtp_sock != PJ_INVALID_SOCKET && !udp->recvfrom_thread_quit)  /* modify by j33783 20190509 */
	   {
            void (*cb)(void*,void*,pj_ssize_t);
            void *user_data;
            pj_bool_t discard = PJ_FALSE;

            cb = udp->rtp_cb;
            user_data = udp->user_data;

            /* add by j33783 20190621 begin */
            timeout.tv_sec = 0;
            timeout.tv_usec = 100000;//100ms
            FD_ZERO(&recv_set);
            FD_SET(udp->rtp_sock, &recv_set);

            status = select(udp->rtp_sock + 1, &recv_set, NULL, NULL, &timeout);

            if(0 == status)
            {
                usleep(10000);//10ms
                continue;
            }
            else if(0 > status)
            {
                //log_error("thread will exit, pj_sock_select return:%d", status);
                break;
            }
            else
            {
                bytes_read = RTP_LEN;
                status = recvfrom(udp->rtp_sock,  udp->rtp_pkt, bytes_read, 0,
                                (struct sockaddr *)&udp->rem_rtp_addr, &udp->addr_len);

                // if (status != PJ_EPENDING && status != PJ_SUCCESS)
                // {
                //     log_error("pj_sock_recvfrom status code:%d", status);
                //         bytes_read = -status;
                // }

                // if(status  == 120009 || pj_get_native_netos_error() == 9)
                // {
                //     log_error("thread will exit, pj_sock_recvfrom return:%d", status);
                //     break;
                // }

                // if(0 == bytes_read)
                // {
                //     log_error("thread will exit, pj_sock_recvfrom receive socket close");
                //     break;
                // }

            }

            if (!discard && udp->attached && cb)  /* 20190328 vid_stream.c on_rx_rtp */   /* add by j33783 20190509 */
            {
                (*cb)(user_data, udp->rtp_pkt, bytes_read);
            }
            else
            {
                //log_debug("skip a loop, discard:%d, attached:%d, cb:%p, bytes:%lld", discard, udp->attached, cb, bytes_read);
            }
            /* add by j33783 20190621 end */
	    }

	 /*commment by 180732700,because the transport_destroy funtion need to kill this thread,20191214*/	 //udp->vid_recv_thread = NULL;
	 //log_debug("thread exit.");
	 return ;
}
#endif
 

pj_status_t pj_thread_create( 
				      const char *thread_name,
				      pj_thread_proc *proc,
				      void *arg,
				      pj_size_t stack_size,
				      unsigned flags,
				      pj_thread_t **ptr_thread) {
                          return 0;
                      }


pj_status_t transport_udp_create( struct transport_udp** tpout, const pj_str_t *addr, int port,
									void (*rtp_cb)(void*, void*, pj_ssize_t),
				                    void (*rtcp_cb)(void*, void*, pj_ssize_t)) 
{
    pj_status_t status = 0;
    int rtp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(rtp_sock<0)
        return -1;

    int rtcp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(rtcp_sock<0)
        return -1;

    struct transport_udp* tp = (struct transport_udp*)malloc(sizeof(struct transport_udp));

	int optVal = 1;
	setsockopt(rtp_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optVal, sizeof(optVal));

    struct timeval tv = {3, 0};
	setsockopt(rtp_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
	setsockopt(rtp_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(struct timeval));
    
    tp->addr_len = sizeof(struct sockaddr_in);
    memset(&tp->rtp_addr_name, 0, tp->addr_len);
    tp->rtp_addr_name.sin_family = AF_INET;
    tp->rtp_addr_name.sin_port = htons(port);
    tp->rtp_addr_name.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(rtp_sock, (struct sockaddr *)&tp->rtp_addr_name, sizeof(struct sockaddr_in)) < 0)
        ;

	optVal = 1;
	setsockopt(rtcp_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optVal, sizeof(optVal));
    
    tp->addr_len = sizeof(struct sockaddr_in);
    memset(&tp->rtcp_addr_name, 0, tp->addr_len);
    tp->rtcp_addr_name.sin_family 	= AF_INET;
    tp->rtcp_addr_name.sin_port 	= htons(port+1);
    tp->rtcp_addr_name.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(rtcp_sock, (struct sockaddr *)&tp->rtcp_addr_name, sizeof(struct sockaddr_in)) < 0)
        ;

	tp->rtp_cb  = rtp_cb;
	tp->rtcp_cb = rtcp_cb;

    pthread_mutex_init(&tp->rtp_cache_mutex, NULL);
    pthread_mutex_init(&tp->udp_socket_mutex, NULL);

    return status;
} 

pj_status_t transport_udp_destroy( struct transport_udp* tp) {
    pj_status_t status = 0;
    pthread_mutex_destroy(&tp->rtp_cache_mutex);
    pthread_mutex_destroy(&tp->udp_socket_mutex);
    return status;
}

pj_status_t transport_udp_start( struct transport_udp* tp) {
    pj_status_t status = 0;
    return status;
}

pj_status_t transport_udp_stop( struct transport_udp* tp) {
    pj_status_t status = 0;
    return status;
}

pj_status_t transport_send_rtp( struct transport_udp*tp, const void *pkt, pj_size_t size) {
    pj_status_t status = 0;
    return status;
}

pj_status_t  transport_reset_socket(struct transport_udp*  tp) {
    pj_status_t status = 0;
    return status;
}

pj_status_t transport_reset_rtp_socket(struct transport_udp*  tp) {
    pj_status_t status = 0;
    return status;
}




