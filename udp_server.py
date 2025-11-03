import socket

# Create UDP socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Bind to host and port
host = '0.0.0.0'  # Accept messages from any client
port = 6000
server_socket.bind((host, port))
print(f"UDP Server listening on {host}:{port}...")

# Chat loop
while True:
    data, addr = server_socket.recvfrom(1024)
    message = data.decode()
    
    if message.lower() == 'bye':
        print(f"Client {addr} disconnected.")
        break

    print(f"Client ({addr}): {message}")
    
    reply = input("Server: ")
    server_socket.sendto(reply.encode(), addr)
    
    if reply.lower() == 'bye':
        print("Server closed the connection.")
        break
