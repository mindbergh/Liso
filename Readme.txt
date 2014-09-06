################################################################################
# README                                                                       #
#                                                                              #
# Description: This file serves as a README and documentation for Checkpoint1. #
#                                                                              #
# Authors: Ming Fang <mingf@cs.cmu.edu>,                                       #
#                                                                              #
################################################################################




[TOC-1] Table of Contents
--------------------------------------------------------------------------------

        [TOC-1] Table of Contents
        [DES-2] Description of Design
        [DES-3] Description of Files
        [RUN-4] How to Run


[DES-2] Description of Design
--------------------------------------------------------------------------------
This is a server called Liso, which is a selec()-based server that 
implements echo service.

I use two different fd_set to keep track of read set and write set
When new client sockets accepted, it is added to read set
When new data is recv'd from certain socket, it is added to write set
When data is sent to certain socket, it is removed from both write set

For each connection, the initial buff size is malloced as 16 bytes, when current
used size is greater half of current buff size, server calls realloc to double 
the buff size. When buff size goes acorss 2^21 bytes, server will close this
socket.

The maximum number of connection if 1024, server will exit once the number of 
established sockets reaches 1024.


[DES-2] Description of Files
--------------------------------------------------------------------------------

Here is a listing of all files associated with Recitation 1 and what their
purpose is:

        .../Readme.txt          - Current document 
        .../echo_client.c       - Simple echo network client
        .../lisod.c             - Liso echo server
        .../mio.c               - C file for IO functions
        .../mio.h               - Header for IO functions
        .../Makefile            - Contains rules for make
        .../tests.txt           - your test cases and any known issues you have
        .../vulnerabilities.ext - The vulnerabilities for my impementation

