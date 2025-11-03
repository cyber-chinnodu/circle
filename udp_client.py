import socket

# Create UDP socket
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Server IP and port (replace with actual server IP)
server_ip = '192.168.1.5'
port = 6000

print("UDP Client ready. Type 'bye' to exit.")

# Chat loop
while True:
    message = input("Client: ")
    client_socket.sendto(message.encode(), (server_ip, port))
    
    if message.lower() == 'bye':
        print("Client closed the connection.")
        break

    data, addr = client_socket.recvfrom(1024)
    reply = data.decode()

    if reply.lower() == 'bye':
        print("Server closed the connection.")
        break

    print(f"Server: {reply}")
