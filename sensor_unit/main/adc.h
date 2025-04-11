#ifndef ADC_H
#define ADC_H

#include "esp_adc/adc_cali.h"

typedef struct {
    uint32_t gpio_pin;
    int value;
} gpio_stat_t;

void adc_init(void);

void adc_to_queue_task(void* pvParameters);

#endif // ADC_H
