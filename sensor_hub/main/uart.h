#ifndef UART_H
#define UART_H

void init_uart(void);

void uart_send_test(void *pvParameters);

void uart_queue_task(void *pvParameters);

#endif // UART_H
