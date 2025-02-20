#ifndef SENSORNET_H
#define SENSORNET_H

#include <string>

#define UNIT_COUNT      5                 // Number of Sensor Units to be connected
#define TX1_MAC         "a8:42:e3:91:45:5c"
#define TX2_MAC         "00:00:00:00:00:00"
#define TX3_MAC         "00:00:00:00:00:00"
#define TX4_MAC         "00:00:00:00:00:00"
#define TX5_MAC         "00:00:00:00:00:00"

#include <string>

// Struct to hold networking info for each Sensor Unit
typedef struct {
    int tx_num;
    esp_ip4_addr_t ip;
    std::string mac;
} sensor_unit;

class SensorNetwork {
private:
    sensor_unit unit_map[UNIT_COUNT]; 

public:
    SensorNetwork();

    // Updates the `unit_map` when a new Sensor_Unit connects
    // Returns the unit number if MAC address is recognize, -1 if not
    int unit_connected(esp_ip4_addr_t ip, std::string mac);

    // Updates the `unit_map` when a Sensor Unit disconnects
    // Return map:
    //      >0: MAC recognized, IP of returned tx removed from map
    //      0: MAC recognized, IP never in map
    //     -1: MAC not recognized 
    int unit_disconnected(std::string mac);
};

#endif // SENSORNET_H