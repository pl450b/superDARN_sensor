#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_INPUT              35
#define GPIO_INPUT_PIN_SEL      ((1ULL<<GPIO_INPUT))
#define ESP_INTR_FLAG_DEFAULT   0

typedef struct {
    uint32_t gpio_pin;
    int value;
} gpio_stat_t;

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    int level = gpio_get_level(gpio_num);
    gpio_stat_t temp_stat = {
        .gpio_pin = gpio_num,
        .value = level,
    };
    if(level == 0) xQueueSendFromISR(gpio_evt_queue, &temp_stat, NULL);
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
    gpio_stat_t pin_value;
    while(1) {
        int pulse_ct = 0;
        while(xQueueReceive(gpio_evt_queue, &pin_value, 5) != pdFALSE) {
            printf("GPIO[%"PRIu32"] intr, val: %d\n", pin_value.gpio_pin, pin_value.value);
            pulse_ct++;
        }
        printf("Pulse count = %i\n", pulse_ct);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void)
{
    gpio_init();
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(gpio_stat_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    printf("Minimum free heap size: %"PRIu32" bytes\n", esp_get_minimum_free_heap_size());
}
