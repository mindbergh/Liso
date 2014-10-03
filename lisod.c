/** @file lisod.c          
 *  @brief This is a select()-based echo server
 *  @auther Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>

#include "mio.h"
#include "loglib.h"
#include "cgi.h"

#define BUF_SIZE      8192   /* Initial buff size */
#define MAX_SIZE_HEADER 8192 /* Max length of size info for the incomming msg */
#define ARG_NUMBER    8    /* The number of argument lisod takes*/
#define LISTENQ       1024   /* second argument to listen() */
#define VERBOSE       0 /* Whether to print out debug infomations */
#define DATE_SIZE     35 /* The max length for date string */
#define FILETYPE_SIZE 15 /* The max length for file type */
#define DEAMON        1 /* Wether to do daemon */
#define AB            1  /* Wether to check http/1.1*/



/* Functions prototypes */
void usage();
int open_listen_socket(int port);
void init_pool(int listen_sock, int ssl_sock, Pool *p);
void add_client(int conn_sock, Pool *p,
                struct sockaddr_in *cli_addr, int port);
void add_client_ssl(SSL *client_context, int conn_sock, Pool *p, 
                    struct sockaddr_in *cli_addr, int port);
void serve_clients(Pool *p);
void server_send(Pool *p);
void clean_state(Pool *p, int listen_sock, int ssl_sock);

void free_buf(Buff *bufi);
void clienterror(Requests *req, char *addr, char *cause,
                 char *errnum, char *shortmsg, char *longmsg);
int read_requesthdrs(Buff *b, Requests *req);
void get_time(char *date);
Requests *get_freereq(Buff *b);
void put_header(Requests * req, char *key, char *value);
void close_conn(Pool *p, int i);
int parse_uri(Pool *p, char *uri, char *filename, char *cgiargs);
void get_filetype(char *filename, char *filetype);
void serve_static(Buff *b, char *filename, struct stat sbuf);
void put_req(Requests *req, char *method, char *uri, char *version);
int is_valid_method(char *method);
char *get_hdr_value_by_key(Headers *hdr, char *key);
int isnumeric(char *str);
int daemonize(char* lock_file);
void liso_shutdown(int ret);

/** @brief Wrapper function for closing socket
 *  @param sock The socket fd to be closed
 *  @return 0 on sucess, 1 on error
 */ 
int close_socket(int sock) {
    printf("Close sock %d\n", sock);
    log_write_string("Close sock %d\n", sock);
    if (close(sock)) {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

/**
 * internal signal handler
 */
void signal_handler(int sig)
{
        switch(sig)
        {
                case SIGHUP:
                        /* rehash the server */
                        break;          
                case SIGTERM:
                        /* finalize and shutdown the server */
                        liso_shutdown(EXIT_SUCCESS);
                        break;    
                default:
                        break;
                        /* unhandled signal */      
        }       
}

int main(int argc, char* argv[]) {
    int listen_sock, client_sock;
    socklen_t cli_size;
    struct sockaddr cli_addr;

    sigset_t mask, old_mask;

    SSL_CTX *ssl_context;
    SSL *client_context;
    int ssl_sock;


    int http_port;  /* The port for the HTTP server to listen on */
    int https_port; /* The port for the HTTPS server to listen on */
    char *log_file; /* File to send log messages to (debug, info, error) */
    char *lock_file; /* File to lock on when becoming a daemon process */
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
    pool.www = argv[5];
    pool.cgi = argv[6];
    pri_key = argv[7];
    cert = argv[8];

    if (DEAMON)
        daemonize(lock_file);


    sigemptyset(&mask);
    sigemptyset(&old_mask);
    sigaddset(&mask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);


    //signal(SIGPIPE, SIG_IGN);
    SSL_load_error_strings();
    SSL_library_init();

    /* we want to use TLSv1 only */
    if ((ssl_context = SSL_CTX_new(TLSv1_server_method())) == NULL)
    {
        fprintf(stderr, "Error creating SSL context.\n");
        return EXIT_FAILURE;
    }

    /* register private key */
    if (SSL_CTX_use_PrivateKey_file(ssl_context, pri_key,
                                    SSL_FILETYPE_PEM) == 0)
    {
        SSL_CTX_free(ssl_context);
        fprintf(stderr, "Error associating private key.\n");
        return EXIT_FAILURE;
    }

    /* register public key (certificate) */
    if (SSL_CTX_use_certificate_file(ssl_context, cert,
                                     SSL_FILETYPE_PEM) == 0)
    {
        SSL_CTX_free(ssl_context);
        fprintf(stderr, "Error associating certificate.\n");
        return EXIT_FAILURE;
    }
    /************ END SSL INIT ************/

    fprintf(stdout, "----- Echo Server -----\n");
    

    /************ SERVER SOCKET SETUP ************/
    listen_sock = open_listen_socket(http_port);
    ssl_sock = open_listen_socket(https_port);
    if (ssl_sock == -1) {
        close_socket(listen_sock);
        SSL_CTX_free(ssl_context);
        return EXIT_FAILURE;
    }


    init_pool(listen_sock, ssl_sock, &pool);
    log_init(log_file);
    memset(&cli_addr, 0, sizeof(struct sockaddr));
    memset(&cli_size, 0, sizeof(socklen_t));
    
    /* finally, loop waiting for input and then write it back */
    while (1) {

        pool.ready_read = pool.read_set;
        pool.ready_write = pool.write_set;
        if (VERBOSE)
            printf("New select\n");
        pool.nready = select(pool.maxfd + 1,
                            &pool.ready_read, 
                            &pool.ready_write, NULL, NULL);
        if (VERBOSE)
            printf("nready = %d\n", pool.nready);

        if (pool.nready == -1) {
            /* Something wrong with select */
            if (VERBOSE)
                printf("Select error on %s\n", strerror(errno));
            clean_state(&pool, listen_sock, ssl_sock);
        }

        if (FD_ISSET(ssl_sock, &pool.ready_read) && 
                     pool.cur_conn <= FD_SETSIZE - 4) {
            
            if ((client_sock = accept(ssl_sock, 
                                    (struct sockaddr *) &cli_addr, 
                                    &cli_size)) == -1) {
                clean_state(&pool, listen_sock, ssl_sock);
                fprintf(stderr, "Error accepting connection.\n");
                continue;
            }
            if (VERBOSE)
                printf("New client %d accepted via https\n", client_sock);

            /************ WRAP SOCKET WITH SSL ************/
            if ((client_context = SSL_new(ssl_context)) == NULL)
            {
                fprintf(stderr, "Error creating client SSL context.\n");
                continue;
            }

            if (SSL_set_fd(client_context, client_sock) == 0)
            {
                fprintf(stderr, "Error creating client SSL context.\n");
                continue;
            }

            if (SSL_accept(client_context) <= 0)
            {
                fprintf(stderr, "Error accepting (handshake) "
                                "client SSL context.\n");
                close_socket(client_sock);
                SSL_free(client_context);
                continue;    
            }
            add_client_ssl(client_context, client_sock, &pool, 
                          (struct sockaddr_in *) &cli_addr, https_port);
        }
        
        if (FD_ISSET(listen_sock, &pool.ready_read) && 
                     pool.cur_conn <= FD_SETSIZE - 4) {
            
            if ((client_sock = accept(listen_sock, 
                                    (struct sockaddr *) &cli_addr, 
                                    &cli_size)) == -1) {
                clean_state(&pool, listen_sock, ssl_sock);
                fprintf(stderr, "Error accepting connection.\n");
                continue;
            }
            if (VERBOSE)
                printf("New client %d accepted via http\n", client_sock);

            fcntl(client_sock, F_SETFL, O_NONBLOCK);
            add_client(client_sock, &pool, 
                      (struct sockaddr_in *) &cli_addr, http_port);
        }

        serve_clients(&pool);
        if (pool.nready)
            server_send(&pool);
        
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

    /* servers bind sockets to ports--notify the OS they accept connections */
    if (bind(listen_socket, (struct sockaddr *) &addr, sizeof(addr))) {
        close_socket(listen_socket);
        fprintf(stderr, "Failed binding socket for port %d.\n", port);
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
void init_pool(int listen_sock, int ssl_sock, Pool *p) {
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++) {
        p->buf[i] = NULL;
        p->pipes[i] = -1;
    }

    p->maxfd = listen_sock > ssl_sock ? listen_sock:ssl_sock;
    p->cur_conn = 0;
    FD_ZERO(&p->read_set);
    FD_ZERO(&p->write_set);
    FD_SET(listen_sock, &p->read_set);
    FD_SET(ssl_sock, &p->read_set);
}

/** @brief Add a new client fd
 *  @param conn_sock The socket of client to be added
 *  @param p the pointer to the pool
 *  @param cli_addr the struct contains addr info
 *  @param port the port of the client
 *  @return Void
 */
void add_client(int conn_sock, Pool *p, 
                struct sockaddr_in *cli_addr, int port) {
    int i;
    Buff *bufi;
    p->cur_conn++;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++)
        if (p->buf[i] == NULL) {

            p->buf[i] = (Buff *)malloc(sizeof(Buff));
            bufi = p->buf[i];
            bufi->buf = (char *)malloc(BUF_SIZE);
            bufi->stage = STAGE_MUV;
            bufi->request = (Requests *)malloc(sizeof(Requests));
            //bufi->request->response = (char *)malloc(BUF_SIZE);
            bufi->request->response = NULL;
            bufi->request->next = NULL;
            bufi->request->header = NULL;
            bufi->request->method = NULL;
            bufi->request->uri = NULL;
            bufi->request->version = NULL;
            bufi->request->valid = REQ_INVALID;
            bufi->request->post_body = NULL;
            bufi->cur_size = 0;
            bufi->cur_parsed = 0;
            bufi->size = BUF_SIZE;
            bufi->fd = conn_sock;
            bufi->client_context = NULL;
            bufi->port = port;
            inet_ntop(AF_INET, &(cli_addr->sin_addr),
                      bufi->addr, INET_ADDRSTRLEN);
            FD_SET(conn_sock, &p->read_set);

            if (conn_sock > p->maxfd)
                p->maxfd = conn_sock;
            if (i > p->maxi)
                p->maxi = i;
            log_write_string("HTTP client added: %s", bufi->addr);
            break;
        }


    if (i == FD_SETSIZE) {
        fprintf(stderr, "Too many client.\n");        
        exit(EXIT_FAILURE);
    }
}

/** @brief Add a new client fd
 *  @param client_context The SSL struct of SSL connection
 *  @param p the pointer to the pool
 *  @param cli_addr the struct contains addr info
 *  @param port the port of the client
 *  @return Void
 */
void add_client_ssl(SSL *client_context, 
                    int conn_sock, Pool *p, 
                    struct sockaddr_in *cli_addr, int port) {
    int i;
    Buff *bufi;
    p->cur_conn++;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++)
        if (p->buf[i] == NULL) {

            p->buf[i] = (Buff *)malloc(sizeof(Buff));
            bufi = p->buf[i];
            bufi->buf = (char *)malloc(BUF_SIZE);
            bufi->stage = STAGE_MUV;
            bufi->request = (Requests *)malloc(sizeof(Requests));
            //bufi->request->response = (char *)malloc(BUF_SIZE);
            bufi->request->response = NULL;
            bufi->request->next = NULL;
            bufi->request->header = NULL;
            bufi->request->method = NULL;
            bufi->request->uri = NULL;
            bufi->request->version = NULL;
            bufi->request->valid = REQ_INVALID;
            bufi->request->post_body = NULL;
            bufi->cur_size = 0;
            bufi->cur_parsed = 0;
            bufi->size = BUF_SIZE;
            bufi->client_context = client_context;
            bufi->fd = conn_sock;
            bufi->port = port;
            inet_ntop(AF_INET, &(cli_addr->sin_addr),
                      bufi->addr, INET_ADDRSTRLEN);
            FD_SET(conn_sock, &p->read_set);

            if (conn_sock > p->maxfd)
                p->maxfd = conn_sock;
            if (i > p->maxi)
                p->maxi = i;
            break;
            log_write_string("HTTPS client added: %s", bufi->addr);
        }

    if (i == FD_SETSIZE) {
        fprintf(stderr, "Too many client.\n");        
        exit(EXIT_FAILURE);
    }
}

/** @brief Perform recv on available sockets in pool
 *  @param p the pointer to the pool
 *  @return Void
 */
void serve_clients(Pool *p) {
    int conn_sock, i;
    SSL *client_context;
    ssize_t readret;
    size_t buf_size;
    char method[BUF_SIZE], uri[BUF_SIZE], version[BUF_SIZE];
    char filename[BUF_SIZE], cgiquery[BUF_SIZE];
    char buf[BUF_SIZE];
    char *value;
    int j;
    struct stat sbuf;
    Requests *req;
    Buff *bufi;

    if (VERBOSE)
        printf("entering recv, nready = %d\n", p->nready);

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        if (p->buf[i] == NULL)
            continue;
        bufi = p->buf[i];
        conn_sock = bufi->fd;
        client_context = bufi->client_context;

        if (FD_ISSET(conn_sock, &p->ready_read)) {
            p->nready--;

            if (bufi->stage == STAGE_ERROR)
                continue;
            if (bufi->stage == STAGE_CLOSE)
                continue;

            if (bufi->stage == STAGE_MUV) {
                buf_size = bufi->size - bufi->cur_size;
                readret = mio_recvlineb(conn_sock, client_context,
                                        bufi->buf + bufi->cur_size,
                                        buf_size);
                if (readret <= 0) {
                    printf("serve_clients: readret = %d\n", (int)readret);
                    close_conn(p, i);
                    continue;
                }
                bufi->cur_size += readret;
                j = sscanf(bufi->buf, "%s %s %s", method, uri, version);
                if (j < 3) {
                    continue;
                }
                
                

                bufi->cur_request = get_freereq(bufi);

                req = bufi->cur_request;
                put_req(req, method, uri, version);
                if (!is_valid_method(method)) {
                    clienterror(req, 
                                bufi->addr, method, 
                                "501", "Not Implemented",
                                "Liso does not implement this method");
                    bufi->stage = STAGE_ERROR;
                    FD_SET(conn_sock, &p->write_set);                    
                    continue;                  
                }
                if (AB)
                    if (strcasecmp(version, "HTTP/1.1")) {
                        clienterror(req, 
                                    bufi->addr, version, 
                                    "501", "Not Implemented",
                                    "Liso does not support the http version");
                        bufi->stage = STAGE_ERROR;
                        FD_SET(conn_sock, &p->write_set);
                        continue;
                    }
                bufi->stage = STAGE_HEADER;
                bufi->cur_parsed = bufi->cur_size;
            }
            if (bufi->stage == STAGE_HEADER) {
                req = bufi->cur_request;
                j = read_requesthdrs(bufi, req);
                
                if (j == -2) {
                    clienterror(bufi->cur_request,
                                bufi->addr, "", 
                                "400", "Bad Request",
                                "Liso couldn't parse the request");
                    //close_conn(p, i);
                    bufi->stage = STAGE_ERROR;
                    FD_SET(conn_sock, &p->write_set);
                    continue;
                } else if (j == 0) {
                    close_conn(p, i);
                    continue;                    
                } else if (j == -1)
                    continue; 
                else
                    bufi->stage = STAGE_BODY;

                if (!strcmp(req->method, "POST")) {
                    if (NULL == (value = get_hdr_value_by_key(req->header, 
                                                     "Content-Length"))) {
                        clienterror(bufi->cur_request,
                                bufi->addr, "", 
                                "411", "Length Required",
                                "Liso needs Content-Length header");
                        bufi->stage = STAGE_ERROR;
                        FD_SET(conn_sock, &p->write_set);
                        continue;
                    }

                    if (!isnumeric(value)) {
                        clienterror(bufi->cur_request,
                                bufi->addr, "", 
                                "400", "Bad Request",
                                "Liso couldn't parse the request");
                        bufi->stage = STAGE_ERROR;
                        FD_SET(conn_sock, &p->write_set);
                        continue;
                    }

                    int length = atoi(value);
                    if (VERBOSE)
                        printf("length atoi = %d\n", length);                    
                    if (client_context != NULL) {
                        readret = SSL_read(client_context, 
                                                buf, 
                                                BUF_SIZE - 1);
                    } else {
                        readret = recv(conn_sock, buf, BUF_SIZE - 1, 0);        
                    }
                    if (VERBOSE)
                        printf("readret =  %d\n", readret);
                    if (readret != length) {
                        clienterror(bufi->cur_request,
                                    bufi->addr, "", 
                                    "400", "Bad Request",
                                    "Liso couldn't parse the request");
                        bufi->stage = STAGE_ERROR;
                        FD_SET(conn_sock, &p->write_set);
                        continue;
                    }
                    req->post_body = (char *)malloc(length + 1);
                    strncpy(req->post_body, buf, length);
                    req->post_body[length] = '\0';
                    if (VERBOSE)
                        printf("post_body:%s\n", req->post_body);
                }

                value = get_hdr_value_by_key(req->header, "Connection");
                if (value) {
                    if (VERBOSE)
                        printf("Req->Connection: %s\n", value);
                    if (!strcmp(value, "close"))
                        bufi->stage = STAGE_CLOSE;
                }
            }



            j = parse_uri(p, uri, filename, cgiquery);
            if (stat(filename, &sbuf) < 0) {                    
                clienterror(bufi->cur_request, 
                            bufi->addr, filename, 
                            "404", "Not found",
                            "Liso couldn't find this file");
                //close_conn(p, i);
                bufi->stage = STAGE_ERROR;
                FD_SET(conn_sock, &p->write_set);
                continue;
            }

            if (j) {
                serve_static(bufi, filename, sbuf);
                FD_SET(conn_sock, &p->write_set);
            }
            else
                serve_dynamic(p, bufi, filename, cgiquery);



            /* Now we can select this fd to test if it can be sent to */
            
            if (VERBOSE)
                printf("Server received on %d, request:\n%s", 
                    conn_sock, bufi->buf);
            if (bufi->stage != STAGE_ERROR && bufi->stage != STAGE_CLOSE)
                bufi->stage = STAGE_MUV;
            bufi->cur_size = 0;
            bufi->cur_parsed = 0;
            FD_CLR(conn_sock, &p->ready_read); /* Remove it from ready read */
        }
    }
}


/** @brief Perform send on available buffs in pool
 *  @param p the pointer to the pool
 *  @return Void
 */
void server_send(Pool *p) {
    int i, conn_sock;
    SSL *client_context;
    ssize_t sendret;
    ssize_t readret;
    Requests *req;
    Buff *bufi;
    if (VERBOSE)
        printf("entering send, nready = %d\n", p->nready);

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
        /* Go thru all pool unit to see if it is a vaild socket 
           and is available to write */
        if (p->buf[i] == NULL)
            continue;
        bufi = p->buf[i];
        conn_sock = bufi->fd;
        client_context = bufi->client_context;

        if (FD_ISSET(conn_sock, &p->ready_write)) {
            req = bufi->request;
            while (req) {
                if (req->valid != REQ_VALID) {
                    req = req->next;
                    continue;
                }

                if ((sendret = mio_sendn(conn_sock, client_context, 
                                         req->response, 
                                         strlen(req->response))) > 0) {
                    if (VERBOSE)
                        printf("Server send header to %d\n", conn_sock);
                    
                } else {
                    close_conn(p, i);
                    req->valid = REQ_INVALID;
                    req = req->next;
                    break;

                }

                if (req->body != NULL) {
                    if ((sendret = mio_sendn(conn_sock, client_context,
                                             req->body, 
                                             req->body_size)) > 0) {
                        if (VERBOSE)
                            printf("Server send %d bytes to %d\n",
                                   (int)sendret, conn_sock);
                        munmap(req->body, req->body_size);
                        req->body = NULL;
                    } else {                        
                        close_conn(p, i);
                        req->valid = REQ_INVALID;
                        req = req->next;
                        break;
                    }
                }

                req->valid = REQ_INVALID;
                req = req->next;
            }
            if (bufi->stage == STAGE_CLOSE) {
                close_conn(p, i);
            }
            if (bufi->stage == STAGE_ERROR)
                bufi->stage = STAGE_MUV;

            FD_CLR(conn_sock, &p->write_set);
        } /* end if FD_ISSET(conn_sock, &p->ready_write) */

        if (p->buf[i] == NULL)
            continue;
        /* if this is a cgi client */
        req = bufi->request;
        while (req) {
            if (req->valid == REQ_PIPE) {
                if (FD_ISSET(req->pipefd, &p->ready_read)) {
                    if (VERBOSE)
                        printf("About to read from pipe\n");
                    char pipebuf[BUF_SIZE];
                    readret = mio_readn(req->pipefd, 
                                        NULL, pipebuf, 
                                        BUF_SIZE-1);
                    pipebuf[readret] = '\0';
                    if (VERBOSE)
                        printf("Pipe return:%s\n", pipebuf);
                    req->response = (char *)malloc(readret + 1);
                    strcpy(req->response, pipebuf);
                    req->valid = REQ_VALID;
                    close_socket(req->pipefd);
                    
                    p->cur_conn -= 1;
                    FD_SET(conn_sock, &p->write_set);
                    FD_CLR(req->pipefd, &p->read_set);
                    req->pipefd = -1;   
                }
            }
            req = req->next;
        }
    }
}


/** @brief Clean up all current connected socket
 *  @param p the pointer to the pool
 *  @return Void
 */
void clean_state(Pool *p, int listen_sock, int ssl_sock) {
    int i, conn_sock;
    for (i = 0; i <= p->maxi; i++) {
        if (p->buf[i]) {
            conn_sock = p->buf[i]->fd;
            if (close_socket(conn_sock)) {
                fprintf(stderr, "Error closing client socket.\n");                        
            }   
            p->cur_conn--;
            FD_CLR(conn_sock, &p->read_set);
            free_buf(p->buf[i]);
            p->buf[i] = NULL;                               
        }
    }
    p->maxi = -1;
    p->maxfd = listen_sock > ssl_sock?listen_sock:ssl_sock;
    p->cur_conn = 0;
}

/** @brief Free a Buff struct that represents a connection
 *  @param bufi the Buff struct to be freeed
 *  @return Void
 */
void free_buf(Buff *bufi) {
    Headers *hdr = NULL;
    Headers *hdr_pre = NULL;
    Requests *req = NULL;
    Requests *req_pre = NULL;
    free(bufi->buf);

    req = bufi->request;
    if (bufi->client_context)
        SSL_free(bufi->client_context);
    while (req) {
        req_pre = req;
        req = req->next;

        hdr = req_pre->header;
        while (hdr) {
            hdr_pre = hdr;
            hdr = hdr->next;
            free(hdr_pre);
        }
        if (req_pre->method)
            free(req_pre->method);
        if (req_pre->uri)
            free(req_pre->uri);
        if (req_pre->version)
            free(req_pre->version);
        if (req_pre->post_body)
            free(req_pre->post_body);
        if (req_pre->pipefd != -1)
            close(req_pre->pipefd);
        free(req_pre->response);
        free(req_pre);
    }
    free(bufi);
}

/** @brief Set clienterror
 *  @param req the Requests struct that represents a connection
 *  @param addr the address string
 *  @param cause the string about the cause
 *  @param errnum error status number
 *  @param shortmsg the string of shortmsg to be sent
 *  @param longmsg the string of longmsg to be sent
 *  @return Void
 */
void clienterror(Requests *req, char *addr, 
                 char *cause, char *errnum, char *shortmsg, 
                 char *longmsg) {
    char date[DATE_SIZE], body[BUF_SIZE], hdr[BUF_SIZE];
    int len = 0;
    get_time(date);
    /* Build the HTTPS response body */
    sprintf(body, "<html><title>Liso</title>");
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Liso Web server</em>\r\n", body);

     /* Print the HTTPS response */
    sprintf(hdr, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    sprintf(hdr, "%sContent-Type: text/html\r\n",hdr);
    sprintf(hdr, "%sConnection: close\r\n",hdr);
    sprintf(hdr, "%sDate: %s\r\n",hdr, date);
    sprintf(hdr, "%sContent-Length: %d\r\n\r\n%s",hdr, 
                                                (int)strlen(body), body);
    len = strlen(hdr);
    req->response = (char *)malloc(len + 1);
    sprintf(req->response, "%s", hdr);
    log_write(req, addr, date, errnum, len);
    req->body = NULL;
    req->valid = REQ_VALID;
}


/** @brief Clean up all current connected socket
 *  @param p the pointer to the pool
 *  @return -1 received a line that does not terminated by \n
 *          -2 received a line that does not match the header format
             0 EOF encountered
 */
int read_requesthdrs(Buff *b, Requests *req) {
    int len = 0;
    int i;
    char key[BUF_SIZE];
    char value[BUF_SIZE];
    char *buf = b->buf + b->cur_parsed;


    if (VERBOSE)
        printf("entering read request:\n%s", buf);

    //printf("%d\n", strcmp(buf, "\r\n"));

    while (1) {
        buf += len;
        mio_recvlineb(b->fd, b->client_context, buf, b->size - b->cur_size);
        len = strlen(buf);

        if (len == 0)
            return 0;
        b->cur_size += len;

        if (buf[len - 1] != '\n') return -1;

        if (VERBOSE)
            printf("Receive line:\n%s", buf);

        if (!strcmp(buf, "\r\n"))
            break;

        i = sscanf(b->buf + b->cur_parsed, "%[^':']: %[^'\r\n']", key, value);

        if (i != 2) return -2;
        
        b->cur_parsed += len;
        
        put_header(req, key, value);
    }
    b->stage = STAGE_BODY;
    if (VERBOSE)
        printf("leaving read request\n");
    return 1;
}


/** @brief get time
 *  @param date the string to put time
 *  @return Void
 */
void get_time(char *date) {
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = localtime(&t);
    strftime(date, DATE_SIZE, "%a, %d %b %Y %T %Z", tmp);
}


/** @brief Retuen a available Requests struct and init it
 *  @param b the Buff that represents a connection
 *  @return a pointer to the new Requests
 */
Requests *get_freereq(Buff *b) {
    Requests *req = b->request;
    while (req->valid == REQ_VALID) {
        if (req->next == NULL)
            break;
        req = req->next;
    }

    if (req->valid == REQ_INVALID) {
        Headers *hdr = req->header;
        Headers *hdr_pre;
        while (hdr) {
            hdr_pre = hdr;
            hdr = hdr->next;
            free(hdr_pre);
        }
        req->header = NULL;
        
    } else {
        req->next = (Requests *)malloc(sizeof(Requests));

        req = req->next;  
    }
    req->pipefd = -1;
    req->response = NULL;
    req->header = NULL;
    req->next = NULL;
    req->method = NULL;
    req->uri = NULL;
    req->version = NULL;
    req->valid = REQ_INVALID;
    req->post_body = NULL;
    return req;
}

/** @brief Put key value pairs of a header in linked list
 *  @param req the pointer to the head of the linked list
 *  @param key the key string
 *  @param value the value string 
 *  @return Void
 */
void put_header(Requests * req, char *key, char *value) {
    Headers *hdr = req->header;
    if (req->header == NULL) {
        req->header = (Headers *)malloc(sizeof(Headers));
        req->header->next = NULL;
        req->header->key = (char *)malloc(strlen(key) + 1);
        req->header->value = (char *)malloc(strlen(value) + 1);
        strcpy(req->header->key, key);
        strcpy(req->header->value, value);
        return;
    }

    while (hdr->next != NULL)
        hdr = hdr->next;

    hdr->next = (Headers *)malloc(sizeof(Headers));
    hdr = hdr->next;
    hdr->next = NULL;
    hdr->key = (char *)malloc(strlen(key) + 1);
    hdr->value = (char *)malloc(strlen(value) + 1);
    strcpy(hdr->key, key);
    strcpy(hdr->value, value);
}

/** @brief Close given connection
 *  @param p the Pool struct
 *         i the ith connection in the pool
 *  @return Void
 */
void close_conn(Pool *p, int i) {
    //if (p->buf[i] == NULL)
        //return;
    int conn_sock = p->buf[i]->fd;
    if (close_socket(conn_sock)) {
        fprintf(stderr, "Error closing client socket.\n");                        
    }
    p->cur_conn--;
    free_buf(p->buf[i]);
    FD_CLR(conn_sock, &p->read_set);
    FD_CLR(conn_sock, &p->write_set);
    p->buf[i] = NULL;
}


/** @brief Parse the uri
 *  @param p the pointer to the pool
 *  @param uri the string of uri
 *  @param filename the pointer to store filename
 *  @param cgiargs the pointer to store cgi args
 *  @return 1 if it is a static request
 *          0 if it is a dynamic request
 */
int parse_uri(Pool *p, char *uri, char *filename, char *cgiargs) {
    char *ptr;

    if (!strstr(uri, "/cgi/")) {  
        strcpy(cgiargs, "");                             
        strcpy(filename, p->www);                           
        strcat(filename, uri);                           
        if (uri[strlen(uri)-1] == '/')                   
            strcat(filename, "index.html");
        if (VERBOSE)
            printf("Static!\n");                   
        return 1;
    } else {  /* Dynamic content */                        
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        } else 
            strcpy(cgiargs, "");                         
        strcpy(filename, p->cgi);  /* cgi */                         
        //strcat(filename, uri + 4);   /* skip '/cgi/' */
        //uri += 4; /* skip '/cgi'*/
        if (VERBOSE) {
            printf("Dynamic!\n");
            printf("uri:%s\n", uri);
            printf("filename:%s\n", filename);
            printf("cgiargs:%s\n", cgiargs);
        }
        return 0;
    }
}


/** @brief set filetype of a file
 *  @param filename the string of filename to look at
 *  @param filetype the string to put the filetype
 *  @return void
 */
void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".css"))
        strcpy(filetype, "text/css");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else
        strcpy(filetype, "text/plain");
} 

/** @brief put method, uri and version to the Requests struct
 *  @param req the Requests struct to put things in
 *  @param method the string of method
 *  @param uri the string of uri
 *  @param version the string of version
 *  @return Void
 */
void put_req(Requests *req, char *method, char *uri, char *version) {
    req->method = (char *)malloc(strlen(method) + 1);
    req->uri = (char *)malloc(strlen(uri) + 1);
    req->version = (char *)malloc(strlen(version) + 1);
    strcpy(req->method, method);
    strcpy(req->uri, uri);
    strcpy(req->version, version);
} 


/** @brief Serve static content
 *  @param b the Buff struct that represent a connection
 *  @param filename the name of file to be sent
 *  @param sbuf the stat struct contain file attributes
 *  @return Void
 */
void serve_static(Buff *b, char *filename, struct stat sbuf) {
    int srcfd;
    int filesize = sbuf.st_size;
    int len = 0;
    char *srcp, filetype[FILETYPE_SIZE], date[DATE_SIZE], buf[BUF_SIZE];
    Requests *req = b->cur_request;
    char modify_time[DATE_SIZE];


    strftime(modify_time, DATE_SIZE, "%a, %d %b %Y %T %Z", 
             localtime(&sbuf.st_mtime));
    
    get_time(date);
    /* Send response headers to client */
    get_filetype(filename, filetype);       
    sprintf(buf, "HTTP/1.1 200 OK\r\n");    
    sprintf(buf, "%sServer: Liso/1.0\r\n", buf);
    sprintf(buf, "%sDate:%s\r\n", buf, date);
    sprintf(buf, "%sConnection:keep-alive\r\n", buf);
    sprintf(buf, "%sContent-Length: %d\r\n", buf, filesize);
    sprintf(buf, "%sLast-Modified:%s\r\n", buf, modify_time);
    sprintf(buf, "%sContent-Type: %s\r\n\r\n", buf, filetype);

    len = strlen(buf);
    req->response = (char *)malloc(len + 1);
    sprintf(req->response, "%s", buf);
    
    if (strcmp(req->method, "HEAD")) {
        srcfd = open(filename, O_RDONLY, 0);    
        srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
        req->body = srcp;
        req->body_size = filesize;
        close(srcfd);                       
        log_write(req, b->addr, date, "200", len + filesize);    
    } else {
        log_write(req, b->addr, date, "200", len);
    }
 
    

    req->valid = REQ_VALID; 
}


/** @brief To check if the method is implemented
 *  @param method the pointer to the string containing method
 *  @return 1 on yes 0 on no.
 */
int is_valid_method(char *method) {
    if (!strcasecmp(method, "GET")) {
        return 1;
    } else if (!strcasecmp(method, "POST")) {
        return 1;
    } else if (!strcasecmp(method, "HEAD")) {
        return 1;
    }
    return 0;
}

/** @brief Get header value by key
 *  @param hdr the pointer to the header to look up
 *  @param key the pointer to the key to look up
 *  @return the string of value
 */
char *get_hdr_value_by_key(Headers *hdr, char *key) {
    while (hdr) {
        if(!strcmp(hdr->key, key)) {
            return hdr->value;
        }
        hdr = hdr->next;
    }
    return NULL;
}

/** @brief is the input string numeric
 *  @param str the pointer to the string to be tested
 *  @return 0 on no
 *          1 on yes
 */
int isnumeric(char *str) {
  while(*str) {
    if(!isdigit(*str))
      return 0;
    str++;
  }
  return 1;
}

/** @brief Daemonize the server
 *  @param lock_file the the name of file to lock 
 *  @return EXIT_FAILURE on fail
 *  @return EXIT_SUCCESS on success
 */
int daemonize(char* lock_file)
{
        /* drop to having init() as parent */
        int i, lfp, pid = fork();
        char str[256] = {0};
        if (pid < 0) exit(EXIT_FAILURE);
        if (pid > 0) exit(EXIT_SUCCESS);

        setsid();

        for (i = getdtablesize(); i>=0; i--)
            close(i);

        i = open("/dev/null", O_RDWR);
        dup(i); /* stdout */
        dup(i); /* stderr */
        umask(027);

        lfp = open(lock_file, O_RDWR|O_CREAT, 0640);
        
        if (lfp < 0)
            exit(EXIT_FAILURE); /* can not open */

        if (lockf(lfp, F_TLOCK, 0) < 0)
            exit(EXIT_SUCCESS); /* can not lock */
        
        /* only first instance continues */
        sprintf(str, "%d\n", getpid());
        write(lfp, str, strlen(str)); /* record pid to lockfile */

        signal(SIGCHLD, SIG_IGN); /* child terminate signal */

        signal(SIGHUP, signal_handler); /* hangup signal */
        signal(SIGTERM, signal_handler); /* software termination signal from kill */
        log_write_string("Successfully daemonized lisod process, pid: %s\n",
                         str);

        return EXIT_SUCCESS;
}


/** @brief Shutdown the server
 *  @param ret the parameter feed to exit()
 *  @return Void
 */
void liso_shutdown(int ret) {
    log_write_string("Liso shutdown\n");
    log_close();
    exit(ret);
}