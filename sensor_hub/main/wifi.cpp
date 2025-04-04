#include <string>
#include <cstdio>

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
#include "sensor_net.h"

extern SensorNetwork sensorNet;
extern QueueHandle_t dataQueue;

static const char *TAG = "HUB WIFI";

/* FreeRTOS event group to signal when we are connected/disconnected */
static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        // ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        sensorNet.unit_disconnected(event->mac);

    } else if(event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *) event_data;
        sensorNet.unit_connected(event->ip.addr, event->mac);
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
            .ssid_hidden = 1,
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

    /* Register WIFI handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    
    /* Register IP handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, 
                    &wifi_event_handler, NULL, NULL));

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
