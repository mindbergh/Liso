/** @file loglib.c                   
 *  @brief The log manipulation library for Liso sever 
 *  @author Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include "loglib.h"

int log_file;

void log_init(char *file) {
	log_file = open(file, O_RDWR|O_CREAT, 0640);
}


void log_write(Requests *req, char *addr, char *date, char *status, int size) {
	//printf("Logged!!!!\n");
	char str[256] = {0};
	sprintf(str, "%s [%s] \"%s %s %s\" %s %d\n", addr,
	                                            date, 
	                                            req->method, 
	                                            req->uri, 
	                                            req->version, 
	                                            status, 
	                                            size);
    write(log_file, str, strlen(str)); 
}

void log_write_string(char *format, ...) {
	char str[256] = {0};
	va_list args;
    va_start(args, format);
	vsprintf(str, format, args);
	write(log_file, str, strlen(str)); 
	//fflush(log_file);
	va_end(args);
}

void log_close() {
	close(log_file);
}