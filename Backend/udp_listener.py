import socket

UDP_IP = "10.1.1.4"  # Change this if needed
UDP_PORT = 12345  # Port to listen on

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on {UDP_IP}:{UDP_PORT}")

while True:
    data, addr = sock.recvfrom(1024)  # Receive data (max 1024 bytes)

    try:
        decoded_data = data.decode("utf-8")  # Try decoding as UTF-8
    except UnicodeDecodeError:
        decoded_data = data.hex()  # If decoding fails, show as hex

    print(f"Received message: {decoded_data} from {addr}")
