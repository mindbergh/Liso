/** @file mio.c                                                               
 *  @brief The Ming I/O package 
 *         modified from 15213 to handle EPIPE and maintain states
 *         for each buff and also perform operation depending on http or https
 *  @author Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include "mio.h"


/** @brief Send n bytes to a socket or ssl
 *	@param fd the fd to send to
 *  @param ssl_context the ssl context to send to
 *  @param ubuf the buf containing things to be sent
 *	@param n the number of bytes to sent
 *  @return -1 on error
 *	@return 0 on EOF
 *  @return the number of bytes sent
 */
ssize_t mio_sendn(int fd, SSL *ssl_context, char *ubuf, size_t n) {	
    size_t nleft = n;
    ssize_t nsend;
    char *buf = ubuf;
    if (ssl_context != NULL) {
		while (nleft > 0) {
		if ((nsend = SSL_write(ssl_context, buf, nleft)) <= 0) {
		    if (errno == EINTR)  /* interrupted by sig handler return */
				nsend = 0;    /* and call send() again */
			else if (errno == EPIPE) {
				fprintf(stderr, "EPIPE handled\n");
				return -1;
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

    while (nleft > 0) {
	if ((nsend = send(fd, buf, nleft, 0)) <= 0) {
	    if (errno == EINTR)  /* interrupted by sig handler return */
			nsend = 0;    /* and call send() again */
		else if (errno == EPIPE) {
			fprintf(stderr, "EPIPE handled\n");
			return -1;
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

/** @brief Read n bytes from a socket or ssl
 *	@param fd the fd to read from
 *  @param ssl_context the ssl context to read from
 *  @param ubuf the buf to store things
 *	@param n the number of bytes to reads
 *  @return -1 on error
 *  @return other the number of bytes reqad
 */
ssize_t mio_readn(int fd, SSL *ssl_context, char *buf, size_t n) {
	size_t res = 0;
	ssize_t nread;
	size_t nleft = n;

	if (ssl_context != NULL) {
		while (nleft > 0 && (nread = SSL_read(ssl_context, 
											  buf + res, 
											  nleft)) > 0) {
			nleft -= nread;
			res += nread;
		}
		if (nread == -1) {
			if (errno == EAGAIN)
				return res;
			else {
				printf("send error on %s\n", strerror(errno));
				return -1;
			}
		}
		return res;		
	}
	while (nleft > 0 && (nread = read(fd, buf + res, nleft)) > 0) {
		nleft -= nread;
		res += nread;
	}
	if (nread == -1) {
		if (errno == EAGAIN)
			return res;
		else {
			printf("send error on %s\n", strerror(errno));
			return -1;
		}
	}
	return res;
}

/** @brief Recv a line from a socket or ssl
 *	@param fd the fd to read from
 *  @param ssl_context the ssl context to read from
 *  @param ubuf the buf to store things
 *	@param n the max number of bytes to read
 *  @return -1 on error
 *	@return 0 on EOF
 *  @return the number of bytes read
 */
ssize_t mio_recvlineb(int fd, SSL *ssl_context, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    if (ssl_context != NULL) {
	    for (n = 1; n < maxlen; n++) { 
			if ((rc = SSL_read(ssl_context, &c, 1)) == 1) {
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
			printf("send error on %s\n", strerror(errno));
		    return -1;	  /* error */
		}
    }
    *bufp = 0;
    return n;
}