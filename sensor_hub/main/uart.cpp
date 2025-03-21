#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#include "uart.h"

const uart_port_t uart_num = UART_NUM_2;

extern QueueHandle_t dataQueue;

static const char *TAG = "UART TASK";
static char data[128];

#define RX_BUF_SIZE           1024

void init_uart(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

    ESP_ERROR_CHECK(uart_driver_install(uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void uart_send_test(void *pvParameters)
{
    while(1) {
      // Write data to UART.
    char* test_str = "This is a test string.\n";
    uart_write_bytes(uart_num, (const char*)test_str, strlen(test_str));
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "sent test string over 2");
    }
}

void uart_queue_task(void *pvParameters)
{
    while(1) {
        BaseType_t status = xQueueReceive(dataQueue, &data, 0); // Non-blocking

        if (status == pdPASS) {
            uart_write_bytes(uart_num, (const char*)data, strlen(data));
            ESP_LOGI(TAG, "%s", data);  
            vTaskDelay(pdMS_TO_TICKS(20));
        } else {
            // ESP_LOGE(TAG, "dataQueue empty"); 
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}