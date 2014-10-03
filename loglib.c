/** @file loglib.c                   
 *  @brief The log manipulation library for Liso sever 
 *  @author Ming Fang - mingf@cs.cmu.edu
 *  @bug I am finding
 */

#include "loglib.h"

int log_file;

/** @brief initalize log file
 *	@param the name of log file to create
 *  @return void
 */
void log_init(char *file) {
	log_file = open(file, O_RDWR|O_CREAT, 0640);
}

/** @brief write a log record with date
 *	@param req Requests struct to read info from
 *  @param addr the address info to log
 *  @param date date info to log
 *  @param status status info to log
 *  @param size size info to log
 *  @return Void
 */
void log_write(Requests *req, char *addr, char *date, char *status, int size) {
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

/** @brief write a formated string to log
 *	@param format the string specify the format
 *  @return void
 */
void log_write_string(char *format, ...) {
	char str[256] = {0};
	va_list args;
    va_start(args, format);
	vsprintf(str, format, args);
	write(log_file, str, strlen(str)); 
	//fflush(log_file);
	va_end(args);
}


/** @brief close log file
 *  @return Void
 */
void log_close() {
	close(log_file);
}