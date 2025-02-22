#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>

#include "sensor_net.h"
#include "wifi.h"

static const char *TAG = "NETWORK CLASS";
// ----- Private Functions -----


bool SensorNetwork::macStringToBytes(const std::string& mac, uint8_t macBytes[6]) {
    if (mac.length() != 17) return false; // Ensure correct length "XX:XX:XX:XX:XX:XX"
    
    std::istringstream iss(mac);
    std::string byte;
    int index = 0;
    
    while (std::getline(iss, byte, ':')) {
        if (byte.length() != 2 || index >= 6) return false;
        std::istringstream hexByte(byte);
        int value;
        
        if (!(hexByte >> std::hex >> value)) return false;
        macBytes[index++] = static_cast<uint8_t>(value);
    }
    
    return index == 6; // Ensure exactly 6 bytes were parsed
}

std::string SensorNetwork::macBytesToString(const uint8_t macBytes[6]) {
    std::ostringstream oss;
    for (int i = 0; i < 6; ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(macBytes[i]);
    }
    return oss.str();
}

bool SensorNetwork::ipStringToBytes(const std::string& ip, uint32_t& ipAddress) {
    std::istringstream iss(ip);
    std::string byte;
    uint32_t result = 0;
    int index = 0;
    
    while (std::getline(iss, byte, '.')) {
        if (index >= 4) return false;
        int value = std::stoi(byte);
        if (value < 0 || value > 255) return false;
        result = (result << 8) | value;
        index++;
    }
    
    if (index != 4) return false;
    ipAddress = result;
    return true;
}

std::string SensorNetwork::ipBytesToString(uint32_t ipAddress) {
    std::ostringstream oss;
    for (int i = 0; i <= 3; ++i) {
        if (i > 0) oss << ".";
        oss << ((ipAddress >> (8 * i)) & 0xFF);
    }
    return oss.str();
}

SensorNetwork::SensorNetwork(void) {
    // Assign hard-coded MAC addresses
    unit_map[1].mac = TX1_MAC;
    unit_map[2].mac = TX2_MAC;
    unit_map[3].mac = TX3_MAC;
    unit_map[4].mac = TX4_MAC;
    unit_map[5].mac = TX5_MAC;
}

// ----- Public Functions -----
int SensorNetwork::unit_connected(uint32_t ip, uint8_t mac[6]) {
    std::string macStr = macBytesToString(mac);
    std::string ipStr = ipBytesToString(ip);

    for(int i = 0; i <= UNIT_COUNT; i++) {
        if(unit_map[i].mac == macStr) {
            unit_map[i].ip = ipStr;
            ESP_LOGI(TAG, "Transmitter %i connected with IP %s", i, ipStr.c_str());
            return i;
        }
    }
    ESP_LOGE(TAG, "Transmitter %s not in map", macStr.c_str());
    return -1;
}

int SensorNetwork::unit_disconnected(uint8_t mac[6]) {
    std::string macStr = macBytesToString(mac);
    for(int i = 0; i <= UNIT_COUNT; i++) {
        if(unit_map[i].mac == macStr) {
            if(unit_map[i].ip == "") return 0;
            else {
                unit_map[i].ip = "";
                vTaskDelete(netHandle[i]);
                netHandle[i] = NULL;
                ESP_LOGE(TAG, "Transmitter %i disconnected", i);
                return i;
            } 
        }
    }
    return -1;
}

void SensorNetwork::unit_task(void *pvParameters) {
    int unit_num = (int)pvParameters;
    bool unit_status = false;

    if(unit_map[unit_num] != "") {
        unit_status = true;
        xTaskCreate(tcp_client_task, "Socket Task", 4096, (void*)ipStr.c_str(), 5, &netHandle[unit_num]);
        
    }
}