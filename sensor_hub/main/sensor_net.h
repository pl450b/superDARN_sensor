#ifndef SENSORNET_H
#define SENSORNET_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>

#define UNIT_COUNT      5                 // Number of Sensor Units to be connected

#define TX1_MAC         "cc:7b:5c:e2:f4:f4"     
#define TX2_MAC         "cc:7b:5c:e3:07:e4"     
#define TX3_MAC         "cc:7b:5c:e3:13:e8"
#define TX4_MAC         "88:13:bf:04:65:90"     // MAC of host
#define TX5_MAC         "00:00:00:00:00:00"


// Struct to hold networking info for each Sensor Unit
typedef struct {
    bool wifi = false;
    bool socket = false;
    std::string ip = "";
    std::string mac = "";
} sensor_unit;

class SensorNetwork {
private:
    sensor_unit unit_map[UNIT_COUNT+1]; 
    wifi_sta_list_t conn_units;

    TaskHandle_t netHandle[UNIT_COUNT+1] = {NULL};

    bool macStringToBytes(const std::string& mac, uint8_t macBytes[6]);

    std::string macBytesToString(const uint8_t macBytes[6]);

    bool ipStringToBytes(const std::string& ip, uint32_t& ipAddress);

    std::string ipBytesToString(uint32_t ipAddress);

public:
    SensorNetwork();

    // Updates the `unit_map` when a new Sensor_Unit connects
    // Returns the unit number if MAC address is recognize, -1 if not
    int unit_connected(uint32_t ip, uint8_t mac[6]);

    // Updates the `unit_map` when a Sensor Unit disconnects
    // Return map:
    //      >0: MAC recognized, IP of returned tx removed from map
    //      0: MAC recognized, IP never in map
    //     -1: MAC not recognized 
    int unit_disconnected(uint8_t mac[6]);

    bool check_unit_connected(int unit_num);

    void unit_task(int unit_num);

    void update_conn_unit(void);
};

#endif // SENSORNET_H