/** @file mio.c                                                               *
 *  @brief The Ming I/O package 
 *         modified from 15213 to handle EPIPE and maintain states
 *         for each buff                                                         *
 *  @author Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include "mio.h"


/** @brief Send n bytes to a socket
 *  @param p The Buff struct that contains state of buff
 *  @return -1 on error or the number of bytes sent
 */
ssize_t mio_sendn(Buff *b) {
    size_t nleft = b->cur_size;
    ssize_t nsend;
    char *buf = b->buf;

    while (nleft > 0) {
	if ((nsend = send(b->fd, buf, nleft, 0)) <= 0) {
	    if (errno == EINTR)  /* interrupted by sig handler return */
			nsend = 0;    /* and call write() again */
		else if (errno == EPIPE) {
			fprintf(stderr, "EPIPE handled\n");
			return nsend;
		} else return -1;       /* errorno set by write() */
	}
	nleft -= nsend;
	buf += nsend;
    }
    return b->cur_size;
}