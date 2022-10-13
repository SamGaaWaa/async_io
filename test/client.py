import socket

HOST = '127.0.0.1'    # The remote host
PORT = 8022              # The same port as used by the server
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    while True:
        tmp = input("")
        s.sendall(str(tmp, 'b'))        
        data = s.recv(1024)
        print('Received', repr(data))

