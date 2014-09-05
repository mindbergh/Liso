/** @file lisod.c                                                               *
 *  @brief This is a select()-based each server                                                                          *
 *  @auther Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <errno.h>

#include "mio.h"

#define BUF_SIZE 16
#define MAX_SIZE_INFO 8 /* Max length of size info for the incomming msg */
#define ARG_NUMBER 8   /* The number of argument lisod takes*/
#define LISTENQ  20  /* second argument to listen() */
#define VERBOSE  1



typedef struct buff {
    char *buf;    /* actually buf, dinamically allocated */
    unsigned int cur_size; /* send size of this buf */
    unsigned int size;     /* whole size of this buf */
} Buff; 

/** @brief The pool of fd that works with select()
 *
 */
typedef struct pool {
    int maxfd;
    fd_set read_set;  /* The set of fd that Liso is looking at*/
    fd_set ready_set; /* The set of fd that is ready to recv */
    fd_set write_set; /* The set of fd that Liso is looking at */
    fd_set ready_write_set; /* The set of fd that is ready to write */
    int nready;       /* The # of fd that is ready to recv */
    int maxi;         /* The max fd */
    int client_sock[FD_SETSIZE]; /* array for client fd */
    Buff *buf[FD_SETSIZE];
} Pool;





/* Functions prototypes */
void usage();
int open_listen_socket(int port);
void init_pool(int listen_sock, Pool *p);
void add_client(int conn_sock, Pool *p);
void serve_clients(Pool *p);
void server_send(Pool *p);


/** @brief Wrapper function for closing socket
 *  @param sock The socket fd to be closed
 *  @return 0 on sucess, 1 on error
 */ 
int close_socket(int sock) {
    printf("Close sock %d\n", sock);
    if (close(sock)) {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    int listen_sock, client_sock;
    socklen_t cli_size;
    struct sockaddr cli_addr;

    int http_port;  /* The port for the HTTP server to listen on */
    int https_port; /* The port for the HTTPS server to listen on */
    char *log_file; /* File to send log messages to (debug, info, error) */
    char *lock_file; /* File to lock on when becoming a daemon process */
    char *www;  /* Folder containing a tree to serve as the root of a website */
    char *cgi; /* Script where to redirect all CGI URIs */
    char *pri_key; /* Private key file path */
    char *cert;  /* Certificate file path */

    Pool pool;

    if (argc != ARG_NUMBER + 1) {
      usage();
    }

    /* Parse arguments */
    http_port = atoi(argv[1]);
    https_port = atoi(argv[2]);
    log_file = argv[3];
    lock_file = argv[4];
    www = argv[5];
    cgi = argv[6];
    pri_key = argv[7];
    cert = argv[8];

    signal(SIGPIPE, SIG_IGN);

    fprintf(stdout, "----- Echo Server -----\n");
    
    listen_sock = open_listen_socket(http_port);
    init_pool(listen_sock, &pool);
    memset(&cli_addr, 0, sizeof(struct sockaddr));
    memset(&cli_size, 0, sizeof(socklen_t));
    
    /* finally, loop waiting for input and then write it back */
    while (1) {

        pool.ready_set = pool.read_set;
        pool.ready_write_set = pool.write_set;
        if (VERBOSE)
            printf("New select\n");
        pool.nready = select(pool.maxfd+1, &pool.ready_set, &pool.ready_write_set, NULL, NULL);

        
        if (FD_ISSET(listen_sock, &pool.ready_set)) {
            
            if ((client_sock = accept(listen_sock, 
                    (struct sockaddr *) &cli_addr, 
                    &cli_size)) == -1) {
                close(listen_sock);
                fprintf(stderr, "Error accepting connection.\n");
                continue;
            }
            fcntl(client_sock, F_SETFL, O_NONBLOCK);
            add_client(client_sock, &pool);
        } else {       
            serve_clients(&pool);
            if (pool.nready)
                server_send(&pool);
        }
    }
    close_socket(listen_sock);
    return EXIT_SUCCESS;
}


/** @brief Print a help message
 *  print a help message and exit.
 *  @return Void
 */
void 
usage(void) {
    fprintf(stderr, "usage: ./lisod <HTTP port> <HTTPS port> <log file> "
      "<lock file> <www folder> <CGI script path> <private key file> "
      "<certificate file>\n");
    exit(EXIT_FAILURE);
}

/** @brief Create a socket to lesten
 *  @param port The number of port to be binded
 *  @return The fd of created listenning socket
 */
int open_listen_socket(int port) {
    int listen_socket;
    int yes = 1;        // for setsockopt() SO_REUSEADDR
    struct sockaddr_in addr;

    /* all networked programs must create a socket */
    if ((listen_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    // lose the pesky "address already in use" error message
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* servers bind sockets to ports---notify the OS they accept connections */
    if (bind(listen_socket, (struct sockaddr *) &addr, sizeof(addr))) {
        close_socket(listen_socket);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }


    if (listen(listen_socket, LISTENQ)) {
        close_socket(listen_socket);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    return listen_socket;
}

/** @brief Initial the pool of client fds to be select
 *  @param listen_sock The socket on which the server is listenning
 *         while initial, this should be the greatest fd
 *  @param p the pointer to the pool
 *  @return Void
 */
void init_pool(int listen_sock, Pool *p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++)
        p->client_sock[i] = -1;

    p->maxfd = listen_sock;
    FD_ZERO(&p->read_set);
    FD_ZERO(&p->write_set);
    FD_SET(listen_sock, &p->read_set);
}

/** @brief Add a new client fd
 *  @param conn_sock The socket of client to be added
 *  @param p the pointer to the pool
 *  @return Void
 */
void add_client(int conn_sock, Pool *p) {
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++)
        if (p->client_sock[i] < 0) {
            p->client_sock[i] = conn_sock;

            p->buf[i] = (Buff *)malloc(sizeof(Buff));
            p->buf[i]->buf = (char *)malloc(BUF_SIZE);
            p->buf[i]->cur_size = 0;
            p->buf[i]->size = BUF_SIZE;

            FD_SET(conn_sock, &p->read_set);

            if (conn_sock > p->maxfd)
                p->maxfd = conn_sock;
            if (i > p->maxi)
                p->maxi = i;
            break;
        }

    if (i == FD_SETSIZE) {
        fprintf(stderr, "Too many client.\n");        
        exit(EXIT_FAILURE);
    }
}

/** @brief A simple echo server to the client
 *  @param listen_sock The socket on which the server is listenning
 *  @param p the pointer to the pool
 *  @return Void
 */
void serve_clients(Pool *p) {
    int conn_sock, i;
    ssize_t readret;
    size_t buf_size;
    if (VERBOSE)
        printf("entering recv, nready = %d\n", p->nready);
    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        conn_sock = p->client_sock[i];

        if ((conn_sock > 0) && (FD_ISSET(conn_sock, &p->ready_set))) {
            p->nready--;
            
            while (1) {
                buf_size = p->buf[i]->size - p->buf[i]->cur_size;
                
                readret = recv(conn_sock, 
                                (p->buf[i]->buf + p->buf[i]->cur_size), 
                                buf_size, 0);
                if (readret < buf_size)
                    break;

                p->buf[i]->cur_size += readret;
                //printf("receive 1 byte\n");
                // if (p->buf[i]->cur_size >= (1 << 30)) {
                //     fprintf(stderr, "serve_clients: Incomming msg is greater than 2^31 bytes\n");
                //     if (close_socket(conn_sock)) {
                //         fprintf(stderr, "Error closing client socket.\n");
                //     }
                //     FD_CLR(conn_sock, &p->read_set);
                //     p->client_sock[i] = -1;
                //     free(p->buf[i]->buf);
                //     free(p->buf[i]);
                //     p->buf[i] = NULL;
                //     continue;
                // }
                if (p->buf[i]->cur_size >= (p->buf[i]->size / 2)) {
                    p->buf[i]->buf = realloc(p->buf[i]->buf, p->buf[i]->size * 2);
                    p->buf[i]->size *= 2;
                }                                    
            }
            if (errno == EWOULDBLOCK)
                printf("serve_clients: read all data, block prevented.\n");
            else if (readret <= 0) {
                fprintf(stderr, "serve_clients: recv return EOF or -1\n", errno);
                if (close_socket(conn_sock)) {
                    fprintf(stderr, "Error closing client socket.\n");                        
                }
                free(p->buf[i]->buf);
                free(p->buf[i]);
                FD_CLR(conn_sock, &p->read_set);
                p->client_sock[i] = -1;
                p->buf[i] = NULL;
                return;
            }

            FD_SET(conn_sock, &p->write_set);
            if (VERBOSE)
                printf("Server received %d bytes data on %d\n", 
                    (int)p->buf[i]->cur_size, conn_sock);
            FD_CLR(conn_sock, &p->ready_set);
        }
    }
}

void server_send(Pool *p) {
    int i, conn_sock;
    ssize_t sendret;
    if (VERBOSE)
        printf("entering send, nready = %d\n", p->nready);

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        conn_sock = p->client_sock[i];

        if ((conn_sock > 0) && (FD_ISSET(conn_sock, &p->ready_write_set))) {
            p->nready--;
            if (p->buf[i]->cur_size == 0)
                continue;
            if ((sendret = mio_sendn(conn_sock, p->buf[i]->buf, p->buf[i]->cur_size)) >= 0) {
                if (VERBOSE)
                    printf("Server send %d bytes to %d, (%d in buf)\n", 
                        (int)sendret, conn_sock, p->buf[i]->cur_size);
            } else {
                if (close_socket(conn_sock)) {
                    fprintf(stderr, "Error closing client socket.\n");                        
                }
            }
            
            FD_CLR(conn_sock, &p->ready_write_set);
            p->buf[i]->cur_size = 0;
        }
    }
}
