#ifndef ADC_H
#define ADC_H

#include "driver/gpio.h"

#define ADC_COUNT               4
#define GPIO_INPUT_0            GPIO_NUM_12
#define GPIO_INPUT_1            GPIO_NUM_13
#define GPIO_INPUT_PIN_SEL      ((1ULL<<GPIO_INPUT_0) | (1ULL<<GPIO_INPUT_1))
#define ESP_INTR_FLAG_DEFAULT   0

#include "esp_adc/adc_cali.h"

typedef struct {
    gpio_num_t gpio_pin;
    int value;
} gpio_stat_t;

void adc_oneshot_init(void);

void digital_input_init(void);

void adc_to_queue_task(void* pvParameters);

void digital_input_to_queue_task(void* pvParameters);

#endif // ADC_H
