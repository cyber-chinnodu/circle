import socket

# Create TCP socket
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_ip = '192.168.1.5'  # Replace with actual server IP
port = 5000

# Connect to server
client_socket.connect((server_ip, port))
print("Connected to server. Type 'bye' to exit.")

# Chat loop
while True:
    message = input("Client: ")
    client_socket.send(message.encode())
    
    if message.lower() == 'bye':
        print("Client closed the connection.")
        break

    data = client_socket.recv(1024).decode()
    if data.lower() == 'bye':
        print("Server closed the connection.")
        break

    print("Server:", data)

client_socket.close()
