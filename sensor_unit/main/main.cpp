#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_adc/adc_oneshot.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "wifi.h"
#include "adc.h"

#define KEEPALIVE_IDLE              10
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3
#define PORT                        3333                 // Port of the server

QueueHandle_t dataQueue;



adc_oneshot_unit_handle_t adc1_handle;
adc_channel_t channels[4] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7};
adc_cali_handle_t adc1_cali_handle[4] = {NULL, NULL, NULL, NULL};

static const char* TAG = "MAIN";
bool wifi_status = false;
bool conn_socket_status = false;
bool listen_socket_status = false;

/* FreeRTOS event group to signal when we are connected/disconnected */
static EventGroupHandle_t s_wifi_event_group;
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
        wifi_status = true;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d, reason:%d", MAC2STR(event->mac), event->aid, event->reason);
        wifi_status = false;
    }
}

extern "C" {
  void app_main(void);
}

void app_main(void)
{
    dataQueue = xQueueCreate(10, sizeof(const char[100]));
    if(dataQueue == NULL) {
        ESP_LOGE(TAG, "Queue not created");
    }
    ESP_LOGI(TAG, "dataQueue created!!");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize event group */
    s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                    &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    adc_init();

    xTaskCreate(tcp_server_task, "TCP Server", 4096, NULL, 5, NULL);
    xTaskCreate(adc_to_queue_task, "ADC Read", 4096, NULL, 5, NULL);
}
