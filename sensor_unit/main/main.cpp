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
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

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

static int adc_raw[4];
static double sensors[4];
adc_oneshot_unit_handle_t adc1_handle;
adc_channel_t channels[4] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7};
adc_cali_handle_t adc1_cali_handle[4] = {NULL, NULL, NULL, NULL};

static const char* TAG = "MAIN";

extern "C" {
  void app_main(void);
}
void app_main(void)
{
    dataQueue = xQueueCreate(10, sizeof(sensors));
    if(dataQueue == NULL) {
        ESP_LOGE(TAG, "Queue not created");
    }
    ESP_LOGI(TAG, "dataQueue created!!");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    adc_init(channels, &adc1_handle, adc1_cali_handle, 4);


    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
    // xTaskCreate(tcp_server_task, "TCP Server Task", 4096, NULL, 5, NULL);

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &adc_raw[0]));
        sensors[0] = 3.3*adc_raw[0]/4095;
        ESP_LOGI(TAG, "Raw Data: %f", sensors[0]);
        vTaskDelay(pdMS_TO_TICKS(500));
        
        BaseType_t tx_result = xQueueSend(dataQueue, &sensors, (TickType_t)0);
        if(tx_result != pdPASS) {
            ESP_LOGE(TAG, "Push to queue failed with error: %i", tx_result);
        }
    }
}
