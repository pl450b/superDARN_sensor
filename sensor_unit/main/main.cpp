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

void app_main(void) {
    nvs_flash_init(); // Required for WiFi

    dataQueue = xQueueCreate(100, sizeof(char[100]));
    adc_init();
    ESP_LOGI("SYSTEM", "Init ADC complete");   
    wifi_init_sta();
    ESP_LOGI("SYSTEM", "Init Wifi complete");

    //Start TCP Client task
    xTaskCreate(tcp_server_task, "TCP Client Task", 4096, NULL, 5, NULL);
    // xTaskCreate(uart_send_test, "uart test", 4096, NULL, 3, NULL);
}
