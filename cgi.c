/** @file cgi.c          
 *  @brief Functions for handle cgi requests
 *  @auther Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */



#include "cgi.h"


/**************** BEGIN CONSTANTS ***************/
#define BUF_SIZE    4096
#define ENVP_SIZE   30
#define VERBOSE     0

/**************** END CONSTANTS ***************/



/**************** BEGIN UTILITY FUNCTIONS ***************/
/* error messages stolen from: http://linux.die.net/man/2/execve */
void execve_error_handler()
{
    switch (errno)
    {
        case E2BIG:
            fprintf(stderr, "The total number of bytes in the environment \
(envp) and argument list (argv) is too large.\n");
            return;
        case EACCES:
            fprintf(stderr, "Execute permission is denied for the file or a \
script or ELF interpreter.\n");
            return;
        case EFAULT:
            fprintf(stderr, "filename points outside your accessible address \
space.\n");
            return;
        case EINVAL:
            fprintf(stderr, "An ELF executable had more than one PT_INTERP \
segment (i.e., tried to name more than one \
interpreter).\n");
            return;
        case EIO:
            fprintf(stderr, "An I/O error occurred.\n");
            return;
        case EISDIR:
            fprintf(stderr, "An ELF interpreter was a directory.\n");
            return;
        case ELIBBAD:
            fprintf(stderr, "An ELF interpreter was not in a recognised \
format.\n");
            return;
        case ELOOP:
            fprintf(stderr, "Too many symbolic links were encountered in \
resolving filename or the name of a script \
or ELF interpreter.\n");
            return;
        case EMFILE:
            fprintf(stderr, "The process has the maximum number of files \
open.\n");
            return;
        case ENAMETOOLONG:
            fprintf(stderr, "filename is too long.\n");
            return;
        case ENFILE:
            fprintf(stderr, "The system limit on the total number of open \
files has been reached.\n");
            return;
        case ENOENT:
            fprintf(stderr, "The file filename or a script or ELF interpreter \
does not exist, or a shared library needed for \
file or interpreter cannot be found.\n");
            return;
        case ENOEXEC:
            fprintf(stderr, "An executable is not in a recognised format, is \
for the wrong architecture, or has some other \
format error that means it cannot be \
executed.\n");
            return;
        case ENOMEM:
            fprintf(stderr, "Insufficient kernel memory was available.\n");
            return;
        case ENOTDIR:
            fprintf(stderr, "A component of the path prefix of filename or a \
script or ELF interpreter is not a directory.\n");
            return;
        case EPERM:
            fprintf(stderr, "The file system is mounted nosuid, the user is \
not the superuser, and the file has an SUID or \
SGID bit set.\n");
            return;
        case ETXTBSY:
            fprintf(stderr, "Executable was open for writing by one or more \
processes.\n");
            return;
        default:
            fprintf(stderr, "Unkown error occurred with execve().\n");
            return;
    }
}
/**************** END UTILITY FUNCTIONS ***************/

/** @brief Serve a dynamic request
 *  @param p Pool struct of the server
 *  @param b Buff struct representing a connection
 *  @param filename name of file to get dynamic content from
 *  @param cgiquery string of query
 *  @return EXIT_FAILURE on fail
 *  @retuen EXIT_SUCCESS on success
 */
int serve_dynamic(Pool *p, Buff *b, char *filename, char *cgiquery) {
    /*************** BEGIN VARIABLE DECLARATIONS **************/
    Requests *req = b->cur_request;
    pid_t pid;
    char buf[4096];
    int readret;
    int stdin_pipe[2];
    int stdout_pipe[2];
    char *envp[ENVP_SIZE];
    char* argv[] = {
        filename,
        NULL
    };
    
    /*************** END VARIABLE DECLARATIONS **************/


    build_envp(envp, b, cgiquery);

    /*************** BEGIN PIPE **************/
    /* 0 can be read from, 1 can be written to */
    if (pipe(stdin_pipe) < 0)
    {
        fprintf(stderr, "Error piping for stdin.\n");
        return EXIT_FAILURE;
    }

    if (pipe(stdout_pipe) < 0)
    {
        fprintf(stderr, "Error piping for stdout.\n");
        return EXIT_FAILURE;
    }
    /*************** END PIPE **************/

    /*************** BEGIN FORK **************/
    pid = fork();
    /* not good */
    if (pid < 0)
    {
        fprintf(stderr, "Something really bad happened when fork()ing.\n");
        return EXIT_FAILURE;
    }

    /* child, setup environment, execve */
    if (pid == 0)
    {
        /*************** BEGIN EXECVE ****************/
        close(stdout_pipe[0]);
        close(stdin_pipe[1]);
        dup2(stdout_pipe[1], fileno(stdout));
        dup2(stdin_pipe[0], fileno(stdin));
        dup2(stdout_pipe[1], fileno(stderr));
        /* you should probably do something with stderr */

        /* pretty much no matter what, if it returns bad things happened... */
        if (execve(filename, argv, envp))
        {
            execve_error_handler();
            fprintf(stderr, "Error executing execve syscall.\n");
            return EXIT_FAILURE;
        }
        /*************** END EXECVE ****************/
        //free_envp(envp); 
    }

    if (pid > 0)
    {
        free_envp(envp);
        if (VERBOSE)
            fprintf(stdout, "Parent: Heading to select() loop.\n");
        close(stdout_pipe[1]);
        close(stdin_pipe[0]);
        if (!strcmp(req->method, "POST")) {
            if ((readret = write(stdin_pipe[1], 
                                 req->post_body, 
                                 strlen(req->post_body))) < 0)
            {
                fprintf(stderr, "Error writing to spawned CGI program.\n");
                return EXIT_FAILURE;
            }
            if (VERBOSE)
                printf("Write post %d bytes body:%s\n", 
                        readret, req->post_body);
        }

        close(stdin_pipe[1]); /* finished writing to spawn */

        /* you want to be looping with select() telling you when to read */
        // while((readret = read(stdout_pipe[0], buf, BUF_SIZE-1)) > 0)
        // {
        //     buf[readret] = '\0'; /* nul-terminate string */
        //     fprintf(stdout, "Got from CGI: %s\n", buf);
        // }
        req->valid = REQ_PIPE;
        req->pipefd = stdout_pipe[0];

            // int flag = fcntl(req->pipefd, F_GETFL, 0);
            // fcntl(req->pipefd, F_SETFL, flag | O_NONBLOCK);
        FD_SET(req->pipefd, &p->read_set);
        if (req->pipefd > p->maxfd)
            p->maxfd = req->pipefd;
        p->cur_conn += 1;

        //close(stdout_pipe[0]);
        //close(stdin_pipe[1]);

//         if (readret == 0)
//         {
//             fprintf(stdout, "CGI spawned process returned with EOF as 
// expected.\n");
//             return EXIT_SUCCESS;
//         }
        return EXIT_SUCCESS;
    }
    /*************** END FORK **************/

    fprintf(stderr, "Process exiting, badly...how did we get here!?\n");
    return EXIT_FAILURE;
}



/** @brief Build envp from connection info
 *  @param envp pointer to put the envp
 *  @param b Buff struct that represents a connection
 *  @param cgiquery string of cgi query
 *  @return Void
 */
void build_envp(char **envp, Buff *b, char *cgiquery) {
    Requests *req = b->cur_request;
    Headers *hdr = NULL;
    int i = 0;
    char temp[BUF_SIZE];
    envp[i++] = malloc_string("GATEWAY_INTERFACE=CGI/1.1");

    sprintf(temp, "PATH_INFO=%s", req->uri + 4); /* skip "/cgi" */
    envp[i++] = malloc_string(temp);
    sprintf(temp, "REQUEST_URI=%s", req->uri);
    envp[i++] = malloc_string(temp);
    if (*cgiquery != '\0') {
        sprintf(temp, "QUERY_STRING=%s", cgiquery);
        envp[i++] = malloc_string(temp);
    }
    envp[i++] = malloc_string("SCRIPT_NAME=/cgi");  /* hard coded */
    sprintf(temp, "REMOTE_ADDR=%s", b->addr);
    envp[i++] = malloc_string(temp);
    sprintf(temp, "REQUEST_METHOD=%s", req->method);
    envp[i++] = malloc_string(temp);
    sprintf(temp, "SERVER_PORT=%d", b->port);
    envp[i++] = malloc_string(temp);
    envp[i++] = malloc_string("SERVER_PROTOCOL=HTTP/1.1");
    envp[i++] = malloc_string("SERVER_SOFTWARE=Liso/1.0");
    envp[i++] = malloc_string("SERVER_NAME=Liso/1.0");
    if (b->client_context != NULL)
        envp[i++] = malloc_string("HTTPS=1");
    hdr = req->header;
    while (hdr) {       
        if (!strcasecmp(hdr->key, "Content-Length")) {
            sprintf(temp, "CONTENT_LENGTH=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Content-Type")) {
            sprintf(temp, "CONTENT_TYPE=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Accept")) {
            sprintf(temp, "HTTP_ACCEPT=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Referer")) {
            sprintf(temp, "HTTP_REFERER=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Accept-Encoding")) {
            sprintf(temp, "HTTP_ACCEPT_ENCODING=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Accept-Language")) {
            sprintf(temp, "HTTP_ACCEPT_LANGUAGE=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Accept-Charset")) {
            sprintf(temp, "HTTP_ACCEPT_CHARSET=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Host")) {
            sprintf(temp, "HTTP_HOST=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Cookie")) {
            sprintf(temp, "HTTP_COOKIE=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "User-Agent")) {
            sprintf(temp, "HTTP_USER_AGENT=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        } else if (!strcasecmp(hdr->key, "Connection")) {
            sprintf(temp, "HTTP_CONNECTION=%s", hdr->value);
            envp[i++] = malloc_string(temp);
        }
        hdr = hdr->next;
    }
    envp[i] = NULL;
}

/** @brief Make a string of env
 *  @param pointer of string to made from
 *  @return point of string just malloced
 */
char *malloc_string(char *str) {
    int length = strlen(str);
    char *res = (char *)malloc(length + 1);
    strcpy(res, str);
    res[length] = '\0';
    return res;
}


/** @brief free the envp
 *  @param pointer to the envp to be freeed
 *  @return Void
 */
void free_envp(char **str) {
    int i = 0;
    while (str[i] != NULL)
        free(str[i++]);
}