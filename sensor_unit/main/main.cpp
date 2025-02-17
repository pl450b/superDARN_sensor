#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_adc/adc_oneshot.h"
// #include "esp_adc/adc_cali.h"
// #include "esp_adc/adc_cali_scheme.h"

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

extern "C" {
  void app_main(void);
}
void app_main(void)
{
    dataQueue = xQueueCreate(10, sizeof(double[4]));
    if(dataQueue == NULL) {
        ESP_LOGE(TAG, "Queue not created");
    }
    ESP_LOGI(TAG, "dataQueue created!!");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    adc_init();


    xTaskCreate(tcp_server_task, "TCP Server", 4096, NULL, 5, NULL);
    xTaskCreate(adc_to_queue_task, "ADC Read", 4096, NULL, 5, NULL);
}
