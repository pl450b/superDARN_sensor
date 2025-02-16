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

static const char* TAG = "MAIN";

extern "C" {
  void app_main(void);
}
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
    // xTaskCreate(tcp_server_task, "TCP Server Task", 4096, NULL, 5, NULL);
}
