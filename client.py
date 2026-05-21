import socket
import time

host = 'localhost'
port = 8080

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((host, port))
    print(f"Connected to server at {host}:{port}")

    # Send a simple HTTP GET request
    request = b"hello.txt\r\n\r\n"
    s.sendall(b"hello.txt\r\n\r\n")
    print("Sent HTTP GET request")

    # Receive the response from the server
    response = b""
    while True:
        data = s.recv(1024)
        if not data:
            break
        response += data

    print("Received response from server:")
    print(response.decode())