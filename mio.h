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

#endif