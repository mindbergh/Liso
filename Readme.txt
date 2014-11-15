################################################################################
# README                                                                       #
#                                                                              #
# Description: This file serves as a README and documentation.                 #
#                                                                              #
# Authors: Ming Fang <mingf@cs.cmu.edu>,                                       #
#                                                                              #
################################################################################




[TOC-1] Table of Contents
--------------------------------------------------------------------------------

        [TOC-1] Table of Contents
        [DES-2] Description of Design for checkpoint1
        [DES-3] Description of Files for checkpoint1
        [DES-4] Description of Design for checkpoint2
        [DES-5] Description of Files for checkpoint2
        [DES-4] Description of Design for checkpoint3
        [DES-5] Description of Files for checkpoint3

[DES-2] Description of Design for checkpoint1
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


[DES-2] Description of Files for checkpoint1
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



[DES-2] Description of Design for checkpoint2
--------------------------------------------------------------------------------
This is a server called Liso, which is a selec()-based server that 
implements GET, POST, HEAD http requests.

The Liso server responses minimally contain the following header:
        Content-Length
        Content-Type
        Last-Modified
        
The Liso server responses        
200_OK                         if everything is fine
404_NOT_FOUND                  if objects do not exist in the file system
411_LENGTH_REQUIRED            if for POST requests do not contain Content-Length
501_NOT_IMPLEMENTED            if request methods are other than GET POST HEAD 
505_HTTP_VERSION_NOT_SUPPORTED if the requests are other than 1.1


The maximum number of connection if 1024, server will exit once the number of 
established sockets reaches 1024.


[DES-2] Description of Files for checkpoint2
--------------------------------------------------------------------------------

Here is a listing of all files associated with Recitation 1 and what their
purposes are:

        .../Readme.txt          - Current document 
        .../lisod.c             - Liso echo server
        .../mio.c               - C file for IO functions
        .../mio.h               - Header for IO functions
        .../loglib.c            - C file for log related functions
        .../loglib.h            - Header for log related functions
        .../liglib_test         - Test cases for loglib
        .../Makefile            - Contains rules for make
        .../tests.txt           - your test cases and any known issues you have
        .../vulnerabilities.ext - The vulnerabilities for my impementation


[DES-3] Description of Design for checkpoint3
--------------------------------------------------------------------------------
This is a server called Liso, which is a selec()-based server that 
supports GET, POST, HEAD http requests and cgi. It also contains a dynamic blog
implemented by Flaskr, which supports browsing, adding entries, login and logout
operations. 
Liso supports persistent connections
Liso supports requests and responses logging
Liso support HTTPS connections
Liso returns 200 on good request.
Liso returns 411 on POST request without Content-Length field
Liso returns 400 on all kinds of bad requests.
Liso returns 404 on not finding target file
Liso returns 501 on method other than 'GET', 'POST', 'HEAD'

[DES-3] Description of Files for checkpoint3
--------------------------------------------------------------------------------

Here is a listing of all files associated with Recitation 1 and what their
purposes are:

        .../Readme.txt          - Current document 
        .../lisod.c             - Liso echo server
        .../mio.c               - C file for IO functions
        .../mio.h               - Header for IO functions
        .../cgi.c               - C file foi cgi functions
        .../cgi.h               - Header for cgi functions
        .../loglib.c            - C file for log related functions
        .../loglib.h            - Header for log related functions
        .../Makefile            - Contains rules for make
        .../tests.txt           - your test cases and any known issues you have
        .../vulnerabilities.ext - The vulnerabilities for my impementation
        .../flaskr              - Folder that contains the Flaskr blog
        .../ssl_client.py       - script to test ssl requests by urllib
        .../ssl_socket.py       - script to test ssl requests by socket
        .../keepalive.py        - scripte to test keepalive