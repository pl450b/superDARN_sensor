#ifndef ADC_H
#define ADC_H

#include "esp_adc/adc_cali.h"

// Function from https://controllerstech.com/esp32-8-how-to-use-adc-part1/
// Things that have to defined first:
// 
void adc_init(void);

void adc_to_queue_task(void* pvParameters);

#endif // ADC_H
