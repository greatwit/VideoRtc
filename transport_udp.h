#ifndef __PJMEDIA_TRANSPORT_UDP_H__
#define __PJMEDIA_TRANSPORT_UDP_H__

#include <pthread.h>
#include <types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "packets_list.h"

#define HYT_VID_RECVFROM_THREAD
#define HYT_VID_SENDTO_THREAD


#ifndef PJMEDIA_MAX_MRU
#define PJMEDIA_MAX_MRU                        2000
#endif

#define RTP_LEN            PJMEDIA_MAX_MRU
/* Maximum size of incoming RTCP packet */
#define RTCP_LEN    600

#ifndef PJ_SOCKADDR_IN_SIN_ZERO_LEN
#   define PJ_SOCKADDR_IN_SIN_ZERO_LEN	8
#endif

#define PJ_INVALID_SOCKET   (-1)

typedef struct pj_in_addr
{
    pj_uint32_t	s_addr;		/**< The 32bit IP address.	    */
} pj_in_addr;

typedef union pj_in6_addr
{
    /* This is the main entry */
    //pj_uint8_t  s6_addr[16];   /**< 8-bit array */

    /* While these are used for proper alignment */
    pj_uint32_t	u6_addr32[4];

    /* Do not use this with Winsock2, as this will align pj_sockaddr_in6
     * to 64-bit boundary and Winsock2 doesn't like it!
     * Update 26/04/2010:
     *  This is now disabled, see http://trac.pjsip.org/repos/ticket/1058
     */
#if 0 && defined(PJ_HAS_INT64) && PJ_HAS_INT64!=0 && \
    (!defined(PJ_WIN32) || PJ_WIN32==0)
    pj_int64_t	u6_addr64[2];
#endif

} pj_in6_addr;

struct pj_sockaddr_in
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sin_zero_len;	/**< Just ignore this.		    */
    pj_uint8_t  sin_family;	/**< Address family.		    */
#else
    pj_uint16_t	sin_family;	/**< Address family.		    */
#endif
    pj_uint16_t	sin_port;	/**< Transport layer port number.   */
    pj_in_addr	sin_addr;	/**< IP address.		    */
    char	sin_zero[PJ_SOCKADDR_IN_SIN_ZERO_LEN]; /**< Padding.*/
};

typedef struct pj_sockaddr_in6
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sin6_zero_len;	    /**< Just ignore this.	   */
    pj_uint8_t  sin6_family;	    /**< Address family.	   */
#else
    pj_uint16_t	sin6_family;	    /**< Address family		    */
#endif
    pj_uint16_t	sin6_port;	    /**< Transport layer port number. */
    pj_uint32_t	sin6_flowinfo;	    /**< IPv6 flow information	    */
    pj_in6_addr sin6_addr;	    /**< IPv6 address.		    */
    pj_uint32_t sin6_scope_id;	    /**< Set of interfaces for a scope	*/
} pj_sockaddr_in6;

typedef struct pj_addr_hdr
{
#if defined(PJ_SOCKADDR_HAS_LEN) && PJ_SOCKADDR_HAS_LEN!=0
    pj_uint8_t  sa_zero_len;
    pj_uint8_t  sa_family;
#else
    pj_uint16_t	sa_family;	/**< Common data: address family.   */
#endif
} pj_addr_hdr;


/**
 * This union describes a generic socket address.
 */
typedef union pj_sockaddr
{
    pj_addr_hdr	    addr;	/**< Generic transport address.	    */
    pj_sockaddr_in  ipv4;	/**< IPv4 transport address.	    */
    pj_sockaddr_in6 ipv6;	/**< IPv6 transport address.	    */
} pj_sockaddr;


struct transport_udp
{
    //pjmedia_transport	base;		/**< Base transport.		    */

    pj_pool_t	       *pool;		/**< Memory pool		    */
    unsigned		options;	/**< Transport options.		    */
    unsigned		media_options;	/**< Transport media options.	    */

    void	       *user_data;	/**< Only valid when attached	    */
    pj_bool_t		attached;	/**< Has attachment?		    */
    struct sockaddr_in		rem_rtp_addr;	/**< Remote RTP address		    */
    struct sockaddr_in		rem_rtcp_addr;	/**< Remote RTCP address	    */
    socklen_t			    addr_len;	/**< Length of addresses.	    */
    void  (*rtp_cb)(	void*,		/**< To report incoming RTP.	    */
			void*, pj_ssize_t);

    void  (*rtcp_cb)(	void*,		/**< To report incoming RTCP.	    */
			void*, pj_ssize_t);

    unsigned		tx_drop_pct;	/**< Percent of tx pkts to drop.    */
    unsigned		rx_drop_pct;	/**< Percent of rx pkts to drop.    */

    pj_sock_t	    rtp_sock;	/**< RTP socket			    */
    struct sockaddr_in		rtp_addr_name;	/**< Published RTP address.	    */

    pj_sock_t		rtcp_sock;	/**< RTCP socket		    */
    struct sockaddr_in		rtcp_addr_name;	/**< Published RTCP address.	    */

    //pj_ioqueue_key_t   *rtp_key;	/**< RTP socket key in ioqueue	    */
    //pj_ioqueue_op_key_t	rtp_read_op;	/**< Pending read operation	    */
    // unsigned		rtp_write_op_id;/**< Next write_op to use	    */
    // //pending_write	rtp_pending_write[MAX_PENDING];  /**< Pending write */
    // pj_sockaddr		rtp_src_addr;	/**< Actual packet src addr.	    */
    // unsigned		rtp_src_cnt;	/**< How many pkt from this addr.   */
    // int			rtp_addrlen;	/**< Address length.		    */

    char		rtp_pkt[RTP_LEN];/**< Incoming RTP packet buffer    */


    // pj_sockaddr		rtcp_src_addr;	/**< Actual source RTCP address.    */
    // unsigned		rtcp_src_cnt;	/**< How many pkt from this addr.   */
    // int			rtcp_addr_len;	/**< Length of RTCP src address.    */

    //pj_ioqueue_key_t   *rtcp_key;	/**< RTCP socket key in ioqueue	    */
    //pj_ioqueue_op_key_t rtcp_read_op;	/**< Pending read operation	    */
    //pj_ioqueue_op_key_t rtcp_write_op;	/**< Pending write operation	    */
    char		rtcp_pkt[RTCP_LEN];/**< Incoming RTCP packet buffer */
/* begin add by t13693 for video independent thread,2016.3.16 */


#ifdef HYT_VID_RECVFROM_THREAD
    pj_thread_t          	*vid_recv_thread;
    pj_bool_t		 recvfrom_thread_quit;
    pj_bool_t		 used_independent_thread_dec;
#endif	
#ifdef HYT_VID_SENDTO_THREAD
    pj_thread_t		 *vid_rtp_sendto_thread;
    pj_bool_t	  	 sendto_thread_quit;
    pj_bool_t		 used_independent_thread_enc;

    struct  rtp_sendto_thread_list_header     rtp_thread_list_header;
#endif 

    memory_list *mem_list;

	pthread_mutex_t  rtp_cache_mutex;

	pthread_mutex_t  udp_socket_mutex;  /* add by j33783 20190509 */
};

typedef int (pj_thread_proc)(void*);
pj_status_t pj_thread_create( 
				      const char *thread_name,
				      pj_thread_proc *proc,
				      void *arg,
				      pj_size_t stack_size,
				      unsigned flags,
				      pj_thread_t **ptr_thread);

pj_status_t transport_udp_create( struct transport_udp** tpout, const pj_str_t *addr, int port,
                                    void (*rtp_cb)(void*, void*, pj_ssize_t),
				                    void (*rtcp_cb)(void*, void*, pj_ssize_t));
pj_status_t transport_udp_destroy( struct transport_udp* tp);

pj_status_t transport_udp_start( struct transport_udp* tp);
pj_status_t transport_udp_stop( struct transport_udp* tp);
pj_status_t transport_send_rtp( struct transport_udp*tp, const void *pkt, pj_size_t size);
pj_status_t  transport_reset_socket(struct transport_udp*  tp);
pj_status_t transport_reset_rtp_socket(struct transport_udp*  tp);

#endif

