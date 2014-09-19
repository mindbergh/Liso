#ifndef LOGLIB_H
#define LOBLIB_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mio.h"


#define MAX_LINE 200

#define INIT_NEW      1
#define INIT_APPEND   2

FILE * log_init(char *log_file);
void log_write(FILE *fd, Requests *req, char *addr, char *date, char *status, int size);

#endif