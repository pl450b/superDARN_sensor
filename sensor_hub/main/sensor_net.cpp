#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include <string>

#include "sensor_net.h"

static const char *TAG = "NETWORK CLASS";

SensorNetwork::SensorNetwork(void) {
    unit_map[1] = {.tx_num = 1, .mac = TX1_MAC};
    unit_map[2] = {.tx_num = 2, .mac = TX2_MAC};
    unit_map[3] = {.tx_num = 3, .mac = TX3_MAC};
    unit_map[4] = {.tx_num = 4, .mac = TX4_MAC};
    unit_map[5] = {.tx_num = 5, .mac = TX5_MAC};
}

int SensorNetwork::unit_connected(esp_ip4_addr_t ip, std::string mac) {
    for(int i = 0; i < UNIT_COUNT; i++) {
        if(unit_map[i].mac == mac) {
            unit_map[i].ip = ip;
            ESP_LOGI(TAG, "Transmitter %i connected with IP %f", i, ip);
            return i;
        }
    }
    ESP_LOGE(TAG, "Transmitter %s not in map", mac.c_str());
    return -1;
}

int SensorNetwork::unit_disconnected(std::string mac) {
    for(int i = 0; i < UNIT_COUNT; i++) {
        if(unit_map[i].mac == mac) {
            if(unit_map[i].ip.addr == NULL) return 0;
            else return i; 
        }
    }
    return -1;
}