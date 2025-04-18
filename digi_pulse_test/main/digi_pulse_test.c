#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define RX_INPUT                32
#define RF_INPUT               33
#define GPIO_INPUT_PIN_SEL      ((1ULL<<GPIO_INPUT))
#define ESP_INTR_FLAG_DEFAULT   0

static int rx_pulse_ct = 0;
static int rf_pulse_ct = 0;
static portMUX_TYPE input_mutex;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    int pin = (int)arg;
    if(pin == RX_INPUT) {
        rx_pulse_ct++;
    }
    if(pin == RF_INPUT) {
        rf_pulse_ct++;
    }
}

void gpio_init(void) {
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // Set pullup/pulldowns
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;

    gpio_config(&io_conf);
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT, gpio_isr_handler, (void*) GPIO_INPUT);
}

static void gpio_task_example(void* arg)
{
    printf("Task Started");

    while(1) {
        portENTER_CRITICAL(&input_mutex);
        printf("Pulse count = %i\n", pulse_ct);
        pulse_ct = 0;
        portEXIT_CRITICAL(&input_mutex);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    gpio_init();

    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());
}
