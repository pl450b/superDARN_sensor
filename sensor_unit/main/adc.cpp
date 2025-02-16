#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "adc.h"

static const char *TAG = "ADC";

static int adc_raw[4];
static int voltage[2];
adc_oneshot_unit_handle_t adc1_handle;
adc_channel_t channels[4] = {ADC_CHANNEL_0, ADC_CHANNEL_3, ADC_CHANNEL_6, ADC_CHANNEL_7};
adc_cali_handle_t adc1_cali_handle[4] = {NULL, NULL, NULL, NULL};

/*---------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

void adc_init(adc_channel_t *channel_array, adc_cali_handle_t *adc1_cali_handle, uint8_t numChannels)
{
    //-------------ADC1 Init---------------//
	adc_oneshot_unit_init_cfg_t unitConfig = {
		.unit_id = ADC_UNIT_1,  // ADC1
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&unitConfig, &adc1_handle));
	
    //-------------ADC1 Channel(s) Config---------------//
	adc_oneshot_chan_cfg_t channelConfig = {
		.atten = ADC_ATTEN_DB_12,  // 150mV - 2450mV
		.bitwidth = ADC_BITWIDTH_12,  // resolution 12 bit
	};
	
	for (int i=0; i<numChannels; i++)
	{
		ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channel_array[i], &channelConfig));
        bool cali_check = adc_calibration_init(ADC_UNIT_1, channel_array[i], ADC_ATTEN_DB_12, &adc1_cali_handle[i]);
        if(cali_check) {
            ESP_LOGI(TAG, "Channel %i calibrated!", channel_array[i]);
        } else {
            ESP_LOGE(TAG, "Channel %i failed to calibrate", channel_array[i]);
        }
	}
}
