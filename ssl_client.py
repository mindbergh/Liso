#!/usr/bin/python

from socket import *
import sys
import random
import os
import time
from urllib3 import HTTPSConnectionPool



serverHost = 'mingf.ddns.net'
serverPort = 9999
numTrials = 2
numWritesReads = 300
method = 'POST'
path = '/'

pool = HTTPSConnectionPool(serverHost, serverPort, ca_certs='./signer.crt')

RECV_TOTAL_TIMEOUT = 0.1
RECV_EACH_TIMEOUT = 0.01

a = time.time()
socketList = []
    
for i in xrange(numTrials):
    for j in xrange(numWritesReads):
        s = pool.request(method, path)
    	socketList.append(s)

    for s in socketList:
    	if s.status == 200:
    		print 'Success!'
    	else:
    		print 'Fail!'

for s in socketList:
    s.close()

print "Done!"
