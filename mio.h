#ifndef MIO_H
#define MIO_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>

/** @brief The buff struct that keeps track of current 
 *         used size and whole size
 *
 */
typedef struct buff {
    char *buf;    /* actual buf, dinamically allocated */
    int fd;        /* client fd */
    unsigned int cur_size; /* current used size of this buf */
    unsigned int size;     /* whole size of this buf */
} Buff; 

/** @brief The pool of fd that works with select()
 *
 */
typedef struct pool {
    int maxfd;
    fd_set read_set; /* The set of fd Liso is looking at before recving */
    fd_set ready_read; /* The set of fd that is ready to recv */
    fd_set write_set; /* The set of fd Liso is looking at before sending*/
    fd_set ready_write; /* The set of fd that is ready to write */
    int nready;       /* The # of fd that is ready to recv or send */
    int cur_conn;     /* The current number of established connection */
    int maxi;         /* The max fd */
    int client_sock[FD_SETSIZE]; /* array for client fd */
    Buff *buf[FD_SETSIZE];    /* array of points to buff */
} Pool;



/* Mio (Ming I/O) package */
ssize_t mio_sendn(Buff *b);

#endif