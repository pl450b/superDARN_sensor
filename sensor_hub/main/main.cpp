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
#include "driver/uart.h"
#include "driver/gpio.h"

#include "uart.h"
#include "wifi.h"
#include "sensor_net.h"

extern "C" {
  void app_main(void);
}

SensorNetwork sensorNet;
QueueHandle_t dataQueue;

void app_main(void)
{
    dataQueue = xQueueCreate(100, sizeof(char[100]));
    init_uart();
    ESP_LOGI("SYSTEM", "Init UART complete");
    init_wifi_ap();
    ESP_LOGI("SYSTEM", "Init Wifi complete");
    
    //Start TCP Client task
    std::string ipAddr = "192.168.4.2";
    const char* ipAddrConst = ipAddr.c_str();
    // xTaskCreate(tcp_client_task, "TCP Client Task", 4096, (void*)ipAddrConst, 5, NULL);

    // xTaskCreate(uart_send_test, "uart test", 4096, NULL, 3, NULL);
    // for(int i = 0, )
    //     xTaskCreate(tcp_client_task, "Socket Task", 4096, (void*)1, 5, NULL);
}
