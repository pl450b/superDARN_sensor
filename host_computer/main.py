import sys
import serial
import os
import csv
from datetime import datetime

def read_serial(port, log_file):
    try:
        ser = serial.Serial(
            port=port,
            baudrate=115200,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1  # 100ms timeout
        )
        print(f"Listening on {port}...")

        file_exists = os.path.exists(log_file) and os.path.getsize(log_file) > 0
        if not file_exists:
            print(f"Created new log file: {log_file}")
            with open(log_file, mode='w', newline='') as file:
                writer = csv.writer(file)
                writer.writerow(["Timestamp", "UnitNum", "Status", "Data"])

        with open(log_file, mode='a', newline='') as file:
            print(f"Appending data to log file: {log_file}")
            while True:
                data = ser.readline().decode('utf-8')
                if data:
                    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    file.write(f"{timestamp},{data}")
                    print(f"{timestamp},{data}")

    except serial.SerialException as e:
        print(f"Error: {e}")
    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <serial_port> <log_file.csv>")
        sys.exit(1)
    
    serial_port = sys.argv[1]
    log_file = sys.argv[2]
    read_serial(serial_port, log_file)
