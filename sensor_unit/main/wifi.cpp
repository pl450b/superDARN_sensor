#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "driver/gpio.h"

#include "wifi.h"

#define WIFI_SSID                    "WesleyMiniNetwork"
#define WIFI_PASSWD                  "WesleyMiniNetwork"

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
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

extern bool wifi_status;
extern bool listen_socket_status;
extern bool conn_socket_status;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            wifi_status = false;
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            wifi_status = false;
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_status = true;
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler,
                                                        NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler,
                                                        NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, pdFALSE, portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASSWD);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASSWD);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// void tcp_server_task(void *pvParameters)
// {
//     char addr_str[128];
//     int addr_family = AF_INET;
//     int ip_protocol = 0;
//     int keepAlive = 1;
//     double msg_buffer[4];
//     int keepIdle = KEEPALIVE_IDLE;
//     int keepInterval = KEEPALIVE_INTERVAL;
//     int keepCount = KEEPALIVE_COUNT;
//     int sock = -1;
//     int listen_sock = -1;
//     struct sockaddr_storage dest_addr;

//     // Main loop
//     while(1) {
//         // Doesn't attempt socket hosting if not connected to wifi  
//         if(!wifi_status) {
//             listen_socket_status = false;
//             conn_socket_status = false;
//             ESP_LOGE(TAG, "WIFI NOT HITTING");
//             vTaskDelay(pdMS_TO_TICKS(500));
//             continue;
//         }

//         while(wifi_status && !listen_socket_status) {
//             conn_socket_status = false;

//             if (addr_family == AF_INET) {
//                 struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
//                 dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
//                 dest_addr_ip4->sin_family = AF_INET;
//                 dest_addr_ip4->sin_port = htons(PORT);
//                 ip_protocol = IPPROTO_IP;
//             }

//             int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
//             if (listen_sock < 0) {
//                 ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
//                 vTaskDelay(pdMS_TO_TICKS(500));
//                 continue;
//             }

//             int opt = 1;
//             setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

//             ESP_LOGI(TAG, "Socket created");

//             int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
//             if (err != 0) {
//                 ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
//                 ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
//                 close(listen_sock);
//                 vTaskDelay(pdMS_TO_TICKS(500));
//                 continue;
//             }
//             ESP_LOGI(TAG, "Socket bound, port %d", PORT);

//             err = listen(listen_sock, 1);
//             if (err != 0) {
//                 ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
//                 close(listen_sock);
//                 vTaskDelay(pdMS_TO_TICKS(500));
//                 continue;
//             }
//             listen_socket_status = true;
//             ESP_LOGI(TAG, "TCP socket listening");
//         }

//         // Loop to connect socket to outside device and move to another port 
//         while (wifi_status && listen_socket_status && !conn_socket_status) {
//             ESP_LOGI(TAG, "Socket listening");

//             struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
//             socklen_t addr_len = sizeof(source_addr);
//             sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
//             if (sock < 0) {
//                 ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
//                 close(sock);
//                 listen_socket_status = false;
//                 vTaskDelay(pdMS_TO_TICKS(500));
//                 continue;
//             }

//             // Set tcp keepalive option
//             setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
//             setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
//             setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
//             setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
//             // Convert ip address to string
//             if (source_addr.ss_family == PF_INET) {
//                 inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
//             }

//             ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);
//             conn_socket_status = true;
//         }

//         // Loop to send data to the connected client
//         while(wifi_status && listen_socket_status && conn_socket_status) {
//             // Send message to AP from dataQueue
//             xQueueReceive(dataQueue, &msg_buffer, (TickType_t)0);
//             int written = send(sock, msg_buffer, sizeof(msg_buffer), 0);
//             if(written < 0) {
//                 ESP_LOGE(TAG, "Transmit failed, resetting socket connection");
//                 conn_socket_status = false;
//                 shutdown(sock, 0);
//                 close(sock);
//                 vTaskDelay(pdMS_TO_TICKS(500));
//                 continue;
//             }
//             ESP_LOGI(TAG, "Message transmitted over WIFI: %f", msg_buffer[0]);
//             vTaskDelay(pdMS_TO_TICKS(500));
//         }
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }   
// }

void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;
    double msg_buffer[4];

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
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
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

        // Loop to send data to the connected client
        while(1) {
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

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
}

