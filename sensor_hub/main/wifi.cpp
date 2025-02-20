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
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "lwip/err.h"
#include "lwip/sys.h"
#include "driver/gpio.h"

#include "wifi.h"

/* AP Configuration */  
#define WIFI_AP_SSID                "WesleyMiniNetwork"
#define WIFI_AP_PASSWD              "WesleyMiniNetwork"
#define WIFI_CHANNEL                 6
#define MAX_STA_CONN                 18
#define PORT                         3333                    // TCP port number for the server
#define KEEPALIVE_IDLE               240
#define KEEPALIVE_INTERVAL           10
#define KEEPALIVE_COUNT              5
#define SERVER_IP                   "192.168.4.2"  
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

extern QueueHandle_t sensorQueue;

static const char *TAG = "UNIT WIFI";
static char connected_ips[MAX_STA_CONN][16];  // Stores connected IPs

/* FreeRTOS event group to signal when we are connected/disconnected */
static EventGroupHandle_t s_wifi_event_group;

static void update_connected_ips() {
    // PAss
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
        update_connected_ips();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d, reason:%d", MAC2STR(event->mac), event->aid, event->reason);
        update_connected_ips();
    } else if(event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *) event_data;
        ESP_LOGI(TAG, "IP event hit!!!!");
    }
}

/* Initialize soft AP */
static esp_netif_t *wifi_init_softap(void)
{
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .password = WIFI_AP_PASSWD,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_CHANNEL,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .max_connection = MAX_STA_CONN,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             WIFI_AP_SSID, WIFI_AP_PASSWD, WIFI_CHANNEL);

    return esp_netif_ap;
}


void tcp_client_task(void *pvParameters) {
    struct sockaddr_in server_addr;
    int sock;
    int len;
    char rx_buffer[128];

    while (1) {
        // Configure server address
        server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);

        // Create socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Socket created");

        // Connect to socket
        int err = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect, error: %d", errno);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Connected to server, ready to receive!");
        
        while(1) {
            len = recv(sock, rx_buffer, sizeof(rx_buffer)-1, 0);
            if(len < 0) { 
                ESP_LOGE(TAG, "Error occurred when receiving: error %d", errno);
                break;
            } else if (len == 0) {
                ESP_LOGW(TAG, "Connection closed");
                break;
            } else {
                rx_buffer[len] = 0;
                ESP_LOGI(TAG, "Received: %s", rx_buffer);
            }
        }
    close(sock);
    }
}

void init_wifi_ap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize event group */
    s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    /* Initialize AP */
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    esp_netif_t *esp_netif_ap = wifi_init_softap();

    /* Start WiFi */
    ESP_ERROR_CHECK(esp_wifi_start());
}
