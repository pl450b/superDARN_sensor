#ifndef ADC_H
#define ADC_H

#include "esp_adc/adc_cali.h"

// Function from https://controllerstech.com/esp32-8-how-to-use-adc-part1/
// Things that have to defined first:
// 
void adc_init(adc_channel_t *channel_array, adc_oneshot_unit_handle_t *adc1_handle,
    adc_cali_handle_t *adc1_cali_handle, uint8_t numChannels);

#endif // ADC_H
