/******************************************************************************
* lisod.c                                                               *
*                                                                             *
* Description: This file contains the C source code for The Liso Server. The  *
* server will take 8 arugments, support HEAD, GET, POST as well as TLS, CGI   *
*                                                                             *
* Authors: Ming Fang <mingf@cs.cmu.edu>,                                      *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define ARG_NUMBER 1
#define LISTENQ  1024  /* second argument to listen() */


typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int client_sock[FD_SETSIZE];
    char client_buf[FD_SETSIZE];
} Pool;



/* Functions prototypes */
void usage();
int open_listen_socket(int port);
void init_pool(int listen_sock, Pool *p);
void add_client(int conn_sock, Pool *p);
void serve_clients(Pool *p, int listen_sock);



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
                return EXIT_FAILURE;
            }
            add_client(client_sock, &pool);
            
        } else {
       
            serve_clients(&pool, listen_sock);
        }
        
    }

    close_socket(listen_sock);

    return EXIT_SUCCESS;
}


/*
 * usage - print a help message
 */
void 
usage(void) {
    fprintf(stderr, "usage: ./lisod <HTTP port> <HTTPS port> <log file> "
      "<lock file> <www folder> <CGI script path> <private key file> "
      "<certificate file>\n");
    exit(EXIT_FAILURE);
}


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

void init_pool(int listen_sock, Pool *p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++)
        p->client_sock[i] = -1;

    p->maxfd = listen_sock;
    FD_ZERO(&p->read_set);
    FD_SET(listen_sock, &p->read_set);
}


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

void serve_clients(Pool *p, int listen_sock) {
    int conn_sock, i;
    ssize_t readret;
    char buf[BUF_SIZE];

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
                    exit(EXIT_FAILURE);                     
                }
                FD_CLR(conn_sock, &p->read_set);
                p->client_sock[i] = -1;
            }
        }
    }
}