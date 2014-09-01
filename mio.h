/* $begin mio.h */
#ifndef MIO_H
#define MIO_H


#define MIO_BUFSIZE 8192



/* Mio (Ming I/O) package */
ssize_t mio_sendn(int fd, void *usrbuf, size_t n);
ssize_t mio_recvn(int fd, void *usrbuf, size_t n);
ssize_t	mio_recvline(int fd, void *usrbuf, size_t maxlen);

/* Wrappers for Mio package */
//ssize_t Mio_sendn(int fd, void *usrbuf, size_t n);
//void Mio_recvn(int fd, void *usrbuf, size_t n);
//ssize_t Mio_recvlineb(mio_t *rp, void *usrbuf, size_t maxlen);

#endif