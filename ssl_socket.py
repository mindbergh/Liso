#!/usr/bin/env python
#!/usr/bin/env python
#
# This script serves as a simple TLSv1 client for 15-441
#
# Authors: Athula Balachandran <abalacha@cs.cmu.edu>,
#          Charles Rang <rang972@gmail.com>,
#          Wolfgang Richter <wolf@cs.cmu.edu>

import pprint
import socket
import ssl

# try a connection
sock = socket.create_connection(('mingf.ddns.net', 9999))
tls = ssl.wrap_socket(sock, cert_reqs=ssl.CERT_REQUIRED,
                                                 ca_certs='./signer.crt',
                                                 ssl_version=ssl.PROTOCOL_TLSv1)

# what cert did he present?
pprint.pprint(tls.getpeercert())

tls.sendall('POST /cgi/ HTTP/1.1\r\nContent-Length: 10\r\n\r\nabc=123456')
print tls.recv(4096)
tls.close()



# try another connection!

exit(0)
