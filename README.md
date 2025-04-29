# S25-25 Senior Design Project

**SuperDARN Transmitter Monitor System**  
_A real-time embedded monitoring and logging system for SuperDARN radar transmitters._

---

## Table of Contents
- [Overview](#overview)
- [Setup for Use](#setup-for-use)
- [Code Organization](#code-organization)
- [Installation & Setup](#installation--setup)
- [Running the Code](#running-the-code)
- [Contributors](#contributors)

---

## Overview
This project exists to provide the SuperDARN team with remotely accesible logging of the status of their transmitter arrays. A Sensor Unit installed in each transmitter takes measurements of the ambient temperature, RX pulses / 500ms, reflected pulses of 500ms, and status of the 400v line. These values are collected, then sent to a central Sensor Hub over a WiFi connection through a TCP socket. The Sensor Hub collects data from all 18 transmitters and sends the information to the host computer over a USB serial connection. A python program (`superDARN_sensor/host_computer/main.py`) records this data and logs to a CSV file. 

---

## Setup for Use
1. Install one Sensor Unit in each transmitter. 
2. Connect Sensor Hub to the Host Computer using a USB-C cable. 
    - If both the Sensor Unit and Sensor Hub are receiving power, the will automatically connect and start exchanging data.
3. Determine the what port on the Host Computer the Sensor Hub is connected to (`/dev/ttyUSB0, /dev/ttyUSB1, ...`)
4. Run the python script `python3 main.py <serial_port> <log_file.csv>`. Use the `-v` command to display received data through stdout. Use the `-b` to run the program in a background process.
5. Now logging has begun! If the CSV file you specified already exist, the python program will just append new data to it.

---

## Code Organization

This project has three main codebases:

### `sensor_unit`
Software for the ESP32s placed inside each transmitter:
- `sensor_unit/main/main.cpp`: Initializes peripherals and starts RTOS tasks.
- `sensor_unit/main/sensors.*`: Functions for setting up and running ADC/GPIO registers and tasks.
- `sensor_unit/main/wifi.*`: Handles Wi-Fi connection and TCP server.

### `sensor_hub`
Software for the central ESP32 collecting data:
- `sensor_hub/main/main.cpp`: Initializes peripherals and starts RTOS tasks.
- `sensor_hub/main/sensor_net.*`: Manages all client connections and message routing.
- `sensor_hub/main/wifi.*`: Hosts the local Wi-Fi network.
- `sensor_hub/main/uart.*`: Sends data to the host computer via UART.

*Note: you must create a secrets.h file in the parent directory of this repo to set the SSID and password for the WiFi network. This file is used by both the Sensor Hub and Sensor Unit. An example secrets.h is shown below:
```c
// stored in superDARN_Sensor/secrets.h
#define WIFI_SSID       "secret_ssid"  
#define WIFI_PASS       "dhsd8g734ndsd9"
```

### `host_computer`
Python script for receiving and logging data via USB:
- `host_computer/main.py`: Connects to the Sensor Hub over serial, logs data to CSV.

---

## Installation & Setup

### ESP32 Code
Before building or running any components, make sure the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html) development environment is installed and configured.

### Host Computer Code
To setup the environment for `main.py`, run the following commands in the `host_computer` directory:
1. Create virtual environment
    ```bash
    python3 -m venv venv
    ```
2. Activate the virtual environment
    ```bash
    source venv/bin/activate
    ```
3. Install required dependencies
    ```bash
    pip3 install -r requirements.txt
    ```

---

## Running the Code

### ESP32 Components (`sensor_unit` and `sensor_hub`)
1. **Navigate to the project directory**:
    ```bash
    cd sensor_unit  # or sensor_hub
    ```

2. **Build the project**:
    ```bash
    idf.py build
    ```

3. **Flash to the device**:
    ```bash
    idf.py -p <serial port> flash
    ```

4. **Monitor serial output**:
    ```bash
    idf.py -p <serial port> monitor
    ```

---

### Host Computer Script (`host_computer`)

To run the logger script, first activate the venv with `source venv/bin/activate`, then run the script with: `python3 main.py <serial_port> <log_file.csv>`

If the .csv file does not exist, it will be created automatically.

## Contributors
- Tanner Beamer
- Alex Betz
- Corey Carpenter
- Wesley Flynn
- Dagmawi Theodros 
