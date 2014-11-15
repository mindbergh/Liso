#ifndef CGI_H
#define CGI_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "mio.h" 



/* CGI package */
void execve_error_handler(void);
int serve_dynamic(Pool *p, Buff *b, char *filename, char *cgiquery);
void build_envp(char **envp, Buff *b, char *cgiquery);
char *malloc_string(char *str);
void free_envp(char **str);

#endif