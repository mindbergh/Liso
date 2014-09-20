#ifndef MIO_H
#define MIO_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>



#define STAGE_MUV              1000
#define STAGE_HEADER           1001
#define STAGE_BODY             1002
#define STAGE_ERROR            1003
#define STAGE_CLOSE            1004


#define REQ_VALID               1
#define REQ_INVALID             0


typedef struct headers {
    char *key;
    char *value;
    struct headers *next;
} Headers;

typedef struct requests {
    char *method;
    char *uri;
    char *version;
    Headers *header;
    int valid;
    char *response;
    char *body; 
    int body_size;
    struct requests *next;
} Requests;


/** @brief The buff struct that keeps track of current 
 *         used size and whole size
 *
 */
typedef struct buff {
    char addr[INET_ADDRSTRLEN];  /* client ip address */
    char *buf;    /* actual buf, dinamically allocated */
    int fd;        /* client fd */ 
    unsigned int cur_size; /* current used size of this buf */
    unsigned int cur_parsed;
    unsigned int size;     /* whole size of this buf */
    Requests *cur_request;
    int stage;
    Requests *request;
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
    FILE *logfd;
    char *www;

    Buff *buf[FD_SETSIZE];    /* array of points to buff */
} Pool;



/* Mio (Ming I/O) package */
ssize_t mio_sendn(int fd, char *ubuf, size_t n);
ssize_t mio_recvlineb(int fd, void *usrbuf, size_t maxlen);

#endif