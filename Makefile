################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for Recitation 1.             #
#                                                                              #
# Authors: Ming Fang <mingf@cs.cmu.edu>,                                       #
#                                                                              #
################################################################################



CFLAGS = -Wall -g  -Wextra
CC = gcc

all: lisod echo_client

mio.o: mio.c mio.h
	$(CC) $(CFLAGS) -c mio.c

echo_client.o: echo_client.c
	$(CC) $(CFLAGS) -c echo_client.c

lisod.o: lisod.c
	$(CC) $(CFLAGS) -c lisod.c

lisod: lisod.o mio.o

echo_client: echo_client.o 

clean:
	rm -f *~ *.o lisod echo_client
