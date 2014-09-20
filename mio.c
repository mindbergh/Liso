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
//ssize_t mio_sendn(Buff *b) {
ssize_t mio_sendn(int fd, char *ubuf, size_t n) {	
    size_t nleft = n;
    ssize_t nsend;
    char *buf = ubuf;

    while (nleft > 0) {
	if ((nsend = send(fd, buf, nleft, 0)) <= 0) {
	    if (errno == EINTR)  /* interrupted by sig handler return */
			nsend = 0;    /* and call send() again */
		else if (errno == EPIPE) {
			fprintf(stderr, "EPIPE handled\n");
			return nsend;
		} else if (errno == EAGAIN) {
			nsend = 0;
		} else {
			printf("send error on %s\n", strerror(errno));
			return -1;       /* errorno set by send() */
		}
	}
	nleft -= nsend;
	buf += nsend;
    }
    return n;
}


/* 
 * mio_readlineb - mingly read a text line (buffered)
 */
ssize_t mio_recvlineb(int fd, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
		if ((rc = recv(fd, &c, 1, 0)) == 1) {
		    *bufp++ = c;
		    if (c == '\n')
			break;
		} else if (rc == 0) {
		    if (n == 1)
				return 0; /* EOF, no data read */
		    else
				return 0;    /* EOF, some data was read */
		} else {
			if (errno == EWOULDBLOCK)
				break;
		    return -1;	  /* error */
		}
    }
    *bufp = 0;
    return n;
}