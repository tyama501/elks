#ifndef __LINUXMT_SOCKET_H
#define __LINUXMT_SOCKET_H

#include <linuxmt/types.h>

struct sockaddr {
    unsigned short sa_family;
    char sa_data[20];       /* sufficient size for AF_UNIX sockaddr_un in un.h */
};

/* setsockopt parameters */
#define SOL_SOCKET      1

/* careful: option names are close to internal SF_ options in net.h*/
#define SO_REUSEADDR    2
#define SO_RCVBUF       8               /* set TCP CB receive buffer size*/
#define SO_LINGER       13              /* only implemented for l_linger = 0*/

/* non-standard options */
#define SO_NOBUFFER     14              /* don't allocate rcv buffer (e.g. for listen) */

struct linger {
        int             l_onoff;        /* Linger active                */
        int             l_linger;       /* How long to linger for       */
};

#define AF_INET 0
#define AF_UNIX 1

#define PF_INET AF_INET
#define PF_UNIX AF_UNIX

#define AF_LOCAL AF_UNIX
#define PF_LOCAL PF_UNIX

#define SOCK_STREAM     1       /* stream (connection) socket   */
#define SOCK_DGRAM      2       /* datagram (conn.less) socket  */
#define SOCK_RAW        3       /* raw socket                   */

#ifdef __KERNEL__
struct proto_ops;
struct socket;
int sock_register(int,struct proto_ops *);
int move_addr_to_user(char *,size_t,char *,int *);
int sock_awaitconn(struct socket *mysock, struct socket *servsock, int flags);
#endif

#endif
