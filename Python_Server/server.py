import socket
import struct


# Define server IP address and port
SERVER_IP = "0.0.0.0"
SERVER_PORT = 8081

server_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
# server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind((SERVER_IP, SERVER_PORT))
# server_socket.listen(5)
print(f'Server listening on {SERVER_IP}:{SERVER_PORT}')

while True:
    data, client_address = server_socket.recvfrom(1024)
    print(f'Accepted connection from {client_address[0]}:{client_address[1]}')
    print(f'Received data: {data} with size {len(data)} bytes')
    header_bytes = data[:4:-1]
    version = (header_bytes[0] << 2) & 0x03
    type_ = (header_bytes[0] >> 4) & 0x03
    token_length = header_bytes[0] & 0x0F
    code_class = (header_bytes[1] >> 5) & 0x08
    detail = header_bytes[1] & 0x1F
    msg_id = struct.unpack('!H', header_bytes[2:4])[0]
    print(f"Version: {version}")
    print(f"Type: {type_}")
    print(f"Token Length: {token_length}")
    print(f"Code Class: {code_class}")
    print(f"Detail: {detail}")
    print(f"Message ID: {msg_id}")
    # coap_header = struct.unpack('!BBHBB', header_bytes)
    # version = (coap_header[0] >> 6) & 0x03
    # type_ = (coap_header[0] >> 4) & 0x03
    # token_length = coap_header[0] & 0x0F
    # code_class = (coap_header[1] >> 5) & 0x07
    # detail = coap_header[1] & 0x1F
    # msg_id = coap_header[2]

    # # Printing parsed CoAP Header information
    # print(f"Version: {version}")
    # print(f"Type: {type_}")
    # print(f"Token Length: {token_length}")
    # print(f"Code Class: {code_class}")
    # print(f"Detail: {detail}")
    # print(f"Message ID: {msg_id}")

    # # Splitting payload if present
    # payload = data[4:]
    # print(f"Payload: {payload}")

    # Sending the response back to the client
    # server_socket.sendto(data, client_address)
