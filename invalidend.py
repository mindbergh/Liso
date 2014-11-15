#!/usr/bin/env python
# coding=utf-8


import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 8888))

for i in xrange(10):
    s.send("GET /index.html HTTP/1.1\r\nConnection: Keep-Alive\r\nA: B\r\nC: D\r\n" * 10)
    #s.send("GET /index.html HTTP/1.1\r\na: b\r\n" * 2)
    s.recv(2000000000)
s.send("GET /index.html HTTP/1.1\r\nConnection: close\r\n\r\n")
s.recv(1024)
#s.close()
