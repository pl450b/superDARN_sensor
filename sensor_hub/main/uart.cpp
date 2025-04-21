#include "driver/uart.h"
#include "esp_vfs_dev.h"
#include "esp_log.h"
#include <string.h>

#include "uart.h"
#define UART_DATA_NUM       UART_NUM_0

// const uart_port_t uart_num = UART_NUM_2;

extern QueueHandle_t dataQueue;

static const char *TAG = "UART TASK";
static char data[256];

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

    ESP_ERROR_CHECK(uart_driver_install(UART_DATA_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_DATA_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_DATA_NUM, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void uart_queue_task(void *pvParameters)
{
    while(1) {
        BaseType_t status = xQueueReceive(dataQueue, &data, 0); // Non-blocking

        if (status == pdPASS) {
            char send_buf[264];
            snprintf(send_buf, sizeof(send_buf), "<record>%s", data);
            uart_write_bytes(UART_DATA_NUM, (const char*)send_buf, strlen(send_buf));
            ESP_LOGI(TAG, "%s", data);  
            vTaskDelay(pdMS_TO_TICKS(20));
        } else {
            // ESP_LOGE(TAG, "dataQueue empty"); 
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}