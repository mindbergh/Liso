/** @file loglib.c                   
 *  @brief The log manipulation library for Liso sever 
 *  @author Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include "loglib.h"

FILE * log_init(char *log_file) {
	return fopen(log_file, "w");
}


void log_write(FILE *fp, Requests *req, char *addr, char *date, char *status, int size) {
	//printf("Logged!!!!\n");
	fprintf(fp, "%s [%s] \"%s %s %s\" %s %d\n", addr,
	                                            date, 
	                                            req->method, 
	                                            req->uri, 
	                                            req->version, 
	                                            status, 
	                                            size);
	fflush(fp);
}
