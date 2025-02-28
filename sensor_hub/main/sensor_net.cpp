#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <array>

#include "sensor_net.h"
#include "wifi.h"

extern QueueHandle_t dataQueue;

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

// ----- Public Functions -----
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
            unit_map[i].wifi = true;
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
                unit_map[i].wifi = false;
                ESP_LOGE(TAG, "Transmitter %i disconnected", i);
                return i;
            } 
        }
    }
    return -1;
}

bool SensorNetwork::check_unit_connected(int unit_num) {
    return(unit_map[unit_num].wifi);
}

void SensorNetwork::unit_task(int unit_num) {
    // Initial setup
    // int unit_num = (int)pvParameters;
    // Ensure unit_num is within bounds
    if (unit_num < 0 || unit_num >= UNIT_COUNT) {
        ESP_LOGE("ERROR", "Invalid unit number: %d", unit_num);
        vTaskDelete(NULL);
        return;
    }

    const char* ipAddrStr = unit_map[unit_num].ip.c_str();
    std::string unitTag = "UNIT_" + std::to_string(unit_num) + " TASK";
    const char* UNIT_TAG = unitTag.c_str();  // Get C-style string

    struct sockaddr_in server_addr;
    int sock = -1;
    int len;
    char queue_buffer[128];

    // Task loop started
    while(1) {
        ESP_LOGI(UNIT_TAG, "Task started!");

        // Wifi loop, doesn't pass until wifi is connected
        while(!unit_map[unit_num].wifi) {
            // ESP_LOGE(UNIT_TAG, "Not connected to network");
            std::string temp_msg = "Unit" + std::to_string(unit_num) + " not connected to WIFI, retrying...";
            strncpy(queue_buffer, temp_msg.c_str(), sizeof(queue_buffer) - 1);
            queue_buffer[sizeof(queue_buffer) - 1] = '\0';  // Ensure null termination
            BaseType_t rx_result = xQueueSend(dataQueue, &queue_buffer, (TickType_t)0);
            if(rx_result != pdPASS) {
                ESP_LOGE(UNIT_TAG, "Push to queue failed with error: %i", rx_result);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        // Socket loop, doesn't pass until wifi and socket are connected
        while(unit_map[unit_num].wifi && !unit_map[unit_num].socket) {
            // Configure server address
            server_addr.sin_addr.s_addr = inet_addr(ipAddrStr);
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(PORT);

            // Create socket
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                ESP_LOGE(UNIT_TAG, "Unable to create socket");
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            // Connect to socket
            int err = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
            if (err != 0) {
                ESP_LOGE(UNIT_TAG, "Socket unable to connect, error: %d", errno);
                close(sock);
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            unit_map[unit_num].socket = true;
            ESP_LOGI(UNIT_TAG, "Socket connection established, ready to receive!");
        }

        // Receive loop, continues unless wifi or socket disconnects    
        while(unit_map[unit_num].wifi && unit_map[unit_num].socket) {
            len = recv(sock, queue_buffer, sizeof(queue_buffer)-1, 0);
            if(len < 0) { 
                ESP_LOGE(UNIT_TAG, "Error occurred when receiving: error %d", errno);
                break;  // receive error, just try again without resetting the connection
            } else if (len == 0) {
                ESP_LOGW(UNIT_TAG, "Socket connection closed");
                unit_map[unit_num].socket = false;
                break;
            } else {
                queue_buffer[len] = 0;
                ESP_LOGI(UNIT_TAG, "Socket received: %s", queue_buffer);
                BaseType_t rx_result = xQueueSend(dataQueue, &queue_buffer, (TickType_t)0);
                if(rx_result != pdPASS) {
                    ESP_LOGE(UNIT_TAG, "Push to queue failed with error: %i", rx_result);
                }
            }
            vTaskDelay(pdMS_TO_TICKS(400));
        }
        close(sock);
    }
}