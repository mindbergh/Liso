#!/usr/bin/env python
# coding=utf-8

from urllib3 import HTTPConnectionPool

def test():
    pool = HTTPConnectionPool(host='127.0.0.1', port=8888)
    r = pool.request('GET', '/images/liso_header.png')
    print r.status


if __name__  == "__main__":
    test();
