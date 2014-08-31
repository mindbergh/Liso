################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for Recitation 1.             #
#                                                                              #
# Authors: Ming Fang <mingf@cs.cmu.edu>,                                       #
#                                                                              #
################################################################################



CFLAGS = -Wall -g -Werror  -Wextra
CC = gcc

all: lisod echo_client

lisod: lisod.o 
	${CC} ${CFLAGS} lisod.o -o $@

echo_client: echo_client.o 
	${CC} ${CFLAGS} echo_client.o -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *~ *.o lisod echo_client
