################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for checkpoint 1.             #
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


handin:
	(make clean; cd ..; tar cvf handin.tar 15-441-project-1 --exclude cp1_checker.py --exclude README)


clean:
	rm -f *~ *.o lisod echo_client
