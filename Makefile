################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for liso                      #
#                                                                              #
# Authors: Ming Fang <mingf@cs.cmu.edu>,                                       #
#                                                                              #
################################################################################

CFLAGS = -Wall -g
CC = gcc
LDFLAGS = -lssl

objects = loglib.o mio.o cgi.o lisod.o


default: lisod

.PHONY: default clean clobber handin

lisod: $(objects)
	$(CC) -o $@ $^ $(LDFLAGS)

lisod.o: lisod.c mio.h loglib.h cgi.h
mio.o: mio.c mio.h
cgi.o: cgi.c cgi.h mio.h
loglib.o: loglib.c loglib.h mio.h
loglib_test.o: loglib_test.c loglib.h mio.h

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<


loglib_test: loglib_test.o loglib.o loglib.h mio.o mio.h
	${CC} loglib.o loglib_test.o mio.o -o $@ $(LDFLAGS)


clean:
	rm -f  loglib.o mio.o lisod.o echo_client.o loglib_test.o lisod loglib_test echo_client log cgi.o liso_ssl.o *.tar

clobber: clean
	rm -f lisod

handin:
	(make clean; cd ..; tar cvf handin.tar 15-441-project-1 --exclude test --exclude cp1_checker.py --exclude README --exclude static_site --exclude dumper.py --exclude liso_prototype.py --exclude http_parser.h)



