#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
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

#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"

#include "wifi.h"

#define MAX_STA_CONN                 18
#define PORT                         3333                    // TCP port number for the server
#define KEEPALIVE_IDLE               240
#define KEEPALIVE_INTERVAL           10
#define KEEPALIVE_COUNT              5

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

extern QueueHandle_t dataQueue;

static const char *TAG = "WIFI";
static char connected_ips[MAX_STA_CONN][16];  // Stores connected IPs

extern bool wifi_status;
extern bool listen_socket_status;
extern bool conn_socket_status;

void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    double msg_buffer[4];
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    int sock = -1;
    int listen_sock = -1;
    struct sockaddr_storage dest_addr;

    // Main loop
    while(1) {
        // Doesn't attempt socket hosting if not connected to wifi
        ESP_LOGI(TAG, "Starting TCP server loop");
        if(!wifi_status) {
            listen_socket_status = false;
            conn_socket_status = false;
            ESP_LOGE(TAG, "WIFI NOT HITTING");
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        while(wifi_status && !listen_socket_status) {
            conn_socket_status = false;

            if (addr_family == AF_INET) {
                struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
                dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
                dest_addr_ip4->sin_family = AF_INET;
                dest_addr_ip4->sin_port = htons(PORT);
                ip_protocol = IPPROTO_IP;
            }

            int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
            if (listen_sock < 0) {
                ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            int opt = 1;
            setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            ESP_LOGI(TAG, "Socket created");

            int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err != 0) {
                ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
                ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
                close(listen_sock);
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            ESP_LOGI(TAG, "Socket bound, port %d", PORT);

            err = listen(listen_sock, 1);
            if (err != 0) {
                ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
                close(listen_sock);
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            listen_socket_status = true;
            ESP_LOGI(TAG, "TCP socket listening");
        }

        // Loop to connect socket to outside device and move to another port 
        while (wifi_status && listen_socket_status && !conn_socket_status) {
            ESP_LOGI(TAG, "Socket listening");

            struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
            socklen_t addr_len = sizeof(source_addr);
            sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            if (sock < 0) {
                ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
                close(sock);
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }

            // Set tcp keepalive option
            setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
            setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
            // Convert ip address to string
            if (source_addr.ss_family == PF_INET) {
                inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            }

            ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);
            conn_socket_status = true;
        }

        // Loop to send data to the connected client
        while(wifi_status && listen_socket_status && conn_socket_status) {
            // Send message to AP from dataQueue
            xQueueReceive(dataQueue, &msg_buffer, (TickType_t)0);
            int written = send(sock, msg_buffer, sizeof(msg_buffer), 0);
            if(written < 0) {
                ESP_LOGE(TAG, "Transmit failed, resetting socket connection");
                conn_socket_status = false;
                shutdown(sock, 0);
                close(sock);
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            ESP_LOGI(TAG, "Message transmitted over WIFI: %f", msg_buffer[0]);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }   
    vTaskDelay(pdMS_TO_TICKS(500));
}
