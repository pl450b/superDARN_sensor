#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "wifi.h"
#include "adc.h"

static const char *TAG = "wifi_station";
bool wifi_status;
bool listen_socket_status;
bool conn_socket_status;
adc_oneshot_unit_handle_t adc1_handle;
adc_channel_t channels[4];
adc_cali_handle_t adc1_cali_handle[4];
QueueHandle_t dataQueue;

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
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_sta();

    adc_init();


    xTaskCreate(tcp_server_task, "TCP Server", 4096, NULL, 5, NULL);
    xTaskCreate(adc_to_queue_task, "ADC Read", 4096, NULL, 5, NULL);
}
