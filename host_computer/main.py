import sys
import serial
import os
import csv
import argparse
import daemon
from datetime import datetime

def read_serial(port, log_file, verbose):
    try:
        ser = serial.Serial(
            port=port,
            baudrate=115200,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.1
        )
        if verbose:
            print(f"Listening on {port}...")

        file_exists = os.path.exists(log_file) and os.path.getsize(log_file) > 0
        if not file_exists:
            if verbose:
                print(f"Created new log file: {log_file}")
            with open(log_file, mode='w', newline='') as file:
                writer = csv.writer(file)
                writer.writerow(["Timestamp", "UnitNum", "Status", "HV line", "RX Pulse", "RF Pulse", "Temp(C)"])

        with open(log_file, mode='a', newline='') as file:
            if verbose:
                print(f"Appending data to log file: {log_file}")
            while True:
                try:
                    data = ser.readline().decode('utf-8').strip()
                    if data.startswith("<record>"): # only log data marked as "<record>"
                        data = data[8:]
                        timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                        file.write(f"{timestamp},{data}\n")
                        file.flush()
                        if verbose:
                            print(f"{timestamp},{data}")
                except:
                    pass

    except serial.SerialException as e:
        print(f"Error: {e}")
    except KeyboardInterrupt:
        print("\nExiting...")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

def main():
    parser = argparse.ArgumentParser(description="Serial reader and CSV logger")
    parser.add_argument("serial_port", help="Serial port to listen on (e.g., /dev/ttyUSB0)")
    parser.add_argument("log_file", help="CSV file to log data")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print received data to stdout")
    parser.add_argument("-b", "--background", action="store_true", help="Run process in the background, will print PID")

    args = parser.parse_args()

    if args.background:
        print(f"Running in background. PID: {os.getpid()}")
        with daemon.DaemonContext():
            read_serial(args.serial_port, args.log_file, args.verbose)
    else:
        read_serial(args.serial_port, args.log_file, args.verbose)

if __name__ == "__main__":
    main()
