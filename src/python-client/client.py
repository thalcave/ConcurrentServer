#!/usr/bin/env python

"""
Send a file to a server
"""


import sys
import socket
import os

from stat import S_ISREG

def receive_buf(sock, msglen):
    """Receives msglen from network"""
    msg = ''
    while len(msg) < msglen:
        chunk = sock.recv(msglen-len(msg))
        #print "chunk: {0} {1}".format(chunk, len(chunk))
        if chunk == '':
            raise RuntimeError("socket connection broken")
        msg = msg + chunk
    return msg

def send_buf(sock, msg):
    """Sends msg on sock"""
    totalsent = 0
    while totalsent < len(msg):
        sent = sock.send(msg[totalsent:])
        if sent == 0:
            raise RuntimeError("socket connection broken")
        totalsent = totalsent + sent

def start_client(port, filename):
    """Connects to server (on given port), transfers file"""

    if len(filename) > 256:
        raise Exception('filename too long (256-char accepted)')

    # connect to server
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(('127.0.0.1', int(port)))

        print "connected to server"

        sock.setblocking(True)

        #send 256-char string
        fname = filename + '\0' * (256-len(filename))
        #sock.send(fname)
        send_buf(sock, fname)

        #read answer
        answer = receive_buf(sock, 3)
        print "received {0}".format(answer)

        if answer[:2] != 'OK':
            raise Exception("error from server: "+answer)

        #send file size
        mstat = os.stat(filename)

        if not S_ISREG(mstat.st_mode):
            raise Exception(filename+' is not a file')

        #put size in a string
        tmp_str = str(mstat.st_size)
        #pad it to 8 bytes
        size_str = tmp_str + '\0' * (8-len(tmp_str))
        print "size_str {0}".format(size_str)
        send_buf(sock, size_str)

        #now send the file
        end_file = False
        with open(filename, 'r') as fread:
            while True:
                data = fread.read(65536)
                if data == "":
                    data = '\0' * 65536
                    end_file = True
                send_buf(sock, data)
                if end_file:
                    break



        #time.sleep(2)
        #read answer
        answer = receive_buf(sock, 3)
        print "received {0}".format(answer)

        #list[start:stop]
        if answer[:2] != 'OK':
            raise Exception("error from server: "+answer)

        print "File transferred successfully. Exiting...!"

    except socket.error, msg:
        sock.close()
        print "error: {0}".format(msg[1])
    except Exception, exc:
        sock.close()
        print "error {0}".format(exc)
    except:
        sock.close()
        print "Unexpected error: {0}".format(sys.exc_info()[0])
    finally:
        sock.close()

if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit('Usage {0} <port> <file_to_transfer>'.format(sys.argv[0]))

    start_client(sys.argv[1], sys.argv[2])
