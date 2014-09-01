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

#define BUF_SIZE 4096
#define ARG_NUMBER 8   /* The number of argument lisod takes*/
#define LISTENQ  1024  /* second argument to listen() */

/** @brief The pool of fd that works with select()
 *
 */
typedef struct {
    int maxfd;
    fd_set read_set;  /* The set of fd that Liso is looking at*/
    fd_set ready_set; /* The set of fd that is ready to recv */
    int nready;       /* The # of fd that is ready to recv */
    int maxi;         /* The max fd */
    int client_sock[FD_SETSIZE]; /* array for client fd */
} Pool;



/* Functions prototypes */
void usage();
int open_listen_socket(int port);
void init_pool(int listen_sock, Pool *p);
void add_client(int conn_sock, Pool *p);
void serve_clients(Pool *p, int listen_sock);


/** @brief Wrapper function for closing socket
 *  @param sock The socket fd to be closed
 *  @return 0 on sucess, 1 on error
 */ 
int close_socket(int sock) {
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

    int http_port;

    Pool pool;

    if (argc != ARG_NUMBER + 1) {
      usage();
    }

    /* Parse arguments */
    http_port = atoi(argv[1]);
    https_port = ato1(argv[2]);
    



    fprintf(stdout, "----- Echo Server -----\n");
    
    listen_sock = open_listen_socket(http_port);
    init_pool(listen_sock, &pool);
    
    /* finally, loop waiting for input and then write it back */
    while (1) {

        pool.ready_set = pool.read_set;
        pool.nready = select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        
        if (FD_ISSET(listen_sock, &pool.ready_set)) {
            
            if ((client_sock = 
                    accept(listen_sock, 
                    (struct sockaddr *) &cli_addr, 
                    &cli_size)) == -1) {
                close(listen_sock);
                fprintf(stderr, "Error accepting connection.\n");
                continue;
            }
            add_client(client_sock, &pool);
        } else {       
            serve_clients(&pool, listen_sock);
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
    FD_SET(listen_sock, &p->read_set);
}

/** @brief Add a new client fd
 *  @param conn_sock The socket of client to be addd
 *  @param p the pointer to the pool
 *  @return Void
 */
void add_client(int conn_sock, Pool *p) {
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++)
        if (p->client_sock[i] < 0) {
            p->client_sock[i] = conn_sock;

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

/** @brief A simple each server to the client
 *  @param listen_sock The socket on which the server is listenning
 *  @param p the pointer to the pool
 *  @return Void
 */
void serve_clients(Pool *p, int listen_sock) {
    int conn_sock, i;
    ssize_t readret;
    char *buf;
    char *msg_size[10];

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        conn_sock = p->client_sock[i];

        if ((conn_sock > 0) && (FD_ISSET(conn_sock, &p->ready_set))) {
            p->nready--;



            if ((readret = recv(conn_sock, buf, BUF_SIZE, 0)) > 1) {
                printf("Server received %d bytes data on %d\n", 
                (int)readret, conn_sock);

                if (send(conn_sock, buf, readret, 0) != readret) {
                    close_socket(conn_sock);
                    close_socket(listen_sock);
                    fprintf(stderr, "Error sending to client.\n");
                    exit(EXIT_FAILURE);
                }
                printf("Server sent %d bytes data to %d\n", 
                       (int)readret, conn_sock);
                memset(buf, 0, BUF_SIZE);
            } else {
                if (readret == 0) 
                    printf("serve_clients: socket %d hung up\n", conn_sock);
                else
                    fprintf(stderr, "serve_clients: recv return -1\n");
                if (close_socket(conn_sock)) {
                    close_socket(listen_sock);
                    fprintf(stderr, "Error closing client socket.\n");
                                         
                }
                FD_CLR(conn_sock, &p->read_set);
                p->client_sock[i] = -1;
            }
        }
    }
}