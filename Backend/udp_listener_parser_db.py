import socket
import mysql.connector
from datetime import datetime
import ast  # To safely convert string to list

UDP_IP = "10.1.1.4"
UDP_PORT = 12345

# Set up socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"📡 Listening on {UDP_IP}:{UDP_PORT}")

while True:
    data, addr = sock.recvfrom(1024)

    try:
        decoded_data = data.decode("utf-8")
        message_list = ast.literal_eval(decoded_data)  # Safer than eval
    except (UnicodeDecodeError, ValueError, SyntaxError) as e:
        print(f"❌ Failed to decode/parse data from {addr}: {e}")
        continue

    if not isinstance(message_list, list) or len(message_list) < 3:
        print("❌ Invalid message format. Skipping.")
        continue

    device_id = message_list[0]
    macs = message_list[2:]  # Skip deviceID and temperature
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    try:
        conn = mysql.connector.connect(
            host="localhost",
            user="lau",
            password="lau123",
            database="probe_sniffing"
        )
        cursor = conn.cursor()

        for mac in macs:
            query = """
            INSERT INTO macs (deviceID, MACaddr, insertTime)
            VALUES (%s, %s, %s)
            """
            values = (device_id, mac, timestamp)
            cursor.execute(query, values)

        conn.commit()
        print(f"✅ Inserted {len(macs)} MACs for device {device_id} from {addr}")

        cursor.close()
        conn.close()

    except mysql.connector.Error as err:
        print(f"❌ Database error: {err}")
