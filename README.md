# S25-25 Senior Design Project

**SuperDARN Transmitter Monitor System**  
_A real-time embedded monitoring and logging system for SuperDARN radar transmitters._

---

## Table of Contents
- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Code Organization](#code-organization)
- [Installation & Setup](#installation--setup)
- [Running the Code](#running-the-code)
- [Usage](#usage)
- [Contributors](#contributors)

---

## Overview
_This section should briefly describe the problem the system solves, and its major components._

> _TODO: Add project summary here._

---

## System Architecture
_Describe the system-level layout of components, data flow, and communication._

> _TODO: Add diagram and explanation._

---

## Code Organization

This project has three main codebases:

### `sensor_unit`
Software for the ESP32s placed inside each transmitter:
- `sensor_unit/main/main.cpp`: Initializes peripherals and starts RTOS tasks.
- `sensor_unit/main/adc.*`: Functions for setting up and running ADC/GPIO registers and tasks.
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

Before building or running any components, make sure the [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html) development environment is installed and configured.

> _TODO: Add version numbers and dependencies._

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

Run the logger script:
```bash
python3 main.py <serial_port> <log_file.csv>
```
- If the .csv file does not exist, it will be created automatically.

## Usage
## Contributors
- Alex - TL
- Wesley - software lead
- Corey - 
- Dagu - 
- Tanner -

