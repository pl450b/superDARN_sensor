#ifndef ADC_H
#define ADC_H

#include "driver/gpio.h"

#define ADC_COUNT               4
#define RX_INPUT                (gpio_num_t)35
#define RF_INPUT                (gpio_num_t)33
#define GPIO_INPUT_PIN_SEL      ((1ULL<<RX_INPUT) | (1ULL<<RF_INPUT))
#define ESP_INTR_FLAG_DEFAULT   0

#include "esp_adc/adc_cali.h"

typedef struct {
    gpio_num_t gpio_pin;
    int value;
} gpio_stat_t;

void adc_oneshot_init(void);

void digital_input_init(void);

void input_to_queue_task(void* pvParameters);

#endif // ADC_H
