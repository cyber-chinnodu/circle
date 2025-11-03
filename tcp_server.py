import socket

# Create TCP socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
host = '0.0.0.0'
port = 5000

# Bind socket and listen
server_socket.bind((host, port))
server_socket.listen(1)
print(f"Server is listening on {host}:{port}...")

# Accept client connection
conn, addr = server_socket.accept()
print(f"Connected by {addr}")

# Chat loop
while True:
    data = conn.recv(1024).decode()
    if not data or data.lower() == 'bye':
        print("Client disconnected.")
        break
    print("Client:", data)
    
    reply = input("Server: ")
    conn.send(reply.encode())
    if reply.lower() == 'bye':
        print("Server closed the connection.")
        break

conn.close()
server_socket.close()
