#ifndef MIO_H
#define MIO_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>




/* Mio (Ming I/O) package */
ssize_t mio_sendn(int fd, void *usrbuf, size_t n);
ssize_t mio_recvn(int fd, void *usrbuf, size_t n);
ssize_t	mio_recvline(int fd, void *usrbuf, size_t maxlen);

/* Wrappers for Mio package */
//ssize_t Mio_sendn(int fd, void *usrbuf, size_t n);
//void Mio_recvn(int fd, void *usrbuf, size_t n);
//ssize_t Mio_recvlineb(mio_t *rp, void *usrbuf, size_t maxlen);

#endif