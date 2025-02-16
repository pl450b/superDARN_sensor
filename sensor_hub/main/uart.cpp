#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

#include "uart.h"

const uart_port_t uart_num = UART_NUM_2;

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
    // We won't use a buffer for sending data.
    uart_driver_install(uart_num, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(uart_num, &uart_config);
}

void uart_send_test(void *pvParameters)
{
    while(1) {
      // Write data to UART.
    char* test_str = "This is a test string.\n";
    uart_write_bytes(uart_num, (const char*)test_str, strlen(test_str));
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP_LOGI("UART2", "sent test string over 2");
    }
}
