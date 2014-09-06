/** @file mio.c                                                               *
 *  @brief The Ming I/O package                                                                          *
 *  @author Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include "mio.h"

/** @brief Receive n bytes from a socket
 *  @param fd The socket to recv from
 *  @param usrbuf the buff pointer
 *  @param n the max number of bytes to receive
 *  @return -1 on error or the number of bytes received
 */
ssize_t mio_recvn(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nrecv;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
        if ((nrecv = recv(fd, bufp, nleft, 0)) < 0) {
            if (errno == EINTR) /* interrupted by sig handler return */
                nrecv = 0;      /* and call recv() again */
            else if (errno == ECONNRESET) {
                fprintf(stderr, "ECONNRESET handled\n");
                return 0;
            } else
                return -1;      /* errno set by read() */
        } else if (nrecv == 0)
            break;              /* EOF */
        nleft -= nrecv;
        bufp += nrecv;
    }
    return (n - nleft);         /* return >= 0 */
}


/** @brief Send n bytes to a socket
 *  @param fd The socket to send to
 *  @param usrbuf the buff pointer
 *  @param n the max number of bytes to receive
 *  @return -1 on error or the number of bytes sent
 */
ssize_t mio_sendn(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nsend;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nsend = send(fd, bufp, nleft, 0)) <= 0) {
	    if (errno == EINTR)  /* interrupted by sig handler return */
			nsend = 0;    /* and call write() again */
		else if (errno == EPIPE) {
			fprintf(stderr, "EPIPE handled\n");
			return n;
		} else return -1;       /* errorno set by write() */
	}
	nleft -= nsend;
	bufp += nsend;
    }
    return n;
}

/** @brief Receive a line from a socket
 *  @param fd The socket to recv from
 *  @param usrbuf buf pointer
 *  @param maxlen the max length of the line
 *  @return -1 on error or the number of bytes received
 */
ssize_t mio_recvline(int fd, void *usrbuf, size_t maxlen)
{
    int rc;
    unsigned int n;
    char c, *bufp = usrbuf;
    
    for (n = 1; n < maxlen; n++) {
        if ((rc = mio_recvn(fd, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') break;
        } else if (rc == 0) {
            if (n == 1) return 0; /* EOF, no data read */
            else break;    /* EOF, some data was read */
        } else return -1;	  /* error */
    }
    *bufp = 0;
    return n;
}