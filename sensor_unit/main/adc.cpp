#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"

#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include "adc.h"
#include "driver/gpio.h"

extern QueueHandle_t dataQueue;

extern bool wifi_connected;
extern bool l_sock_connected;
extern bool c_sock_connected;

adc_oneshot_unit_handle_t adc1_handle;
adc_channel_t channels[ADC_COUNT] = {ADC_CHANNEL_6, ADC_CHANNEL_4, ADC_CHANNEL_7, ADC_CHANNEL_5}; // 34, 32, 35, 33 
adc_cali_handle_t adc1_cali_handle[ADC_COUNT];


static const char *TAG = "ADC";
static int adc_raw[ADC_COUNT];
static double sensors[ADC_COUNT];

static QueueHandle_t gpio_evt_queue = NULL;

// ADC Calibration stuff
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

static void IRAM_ATTR digital_input_isr_handler(void* arg)
{
    gpio_num_t gpio_num = (gpio_num_t)(intptr_t)arg;
    int level = gpio_get_level(gpio_num);
    gpio_stat_t temp_stat = {
        .gpio_pin = gpio_num,
        .value = level,
    };
    if(level == 0) xQueueSendFromISR(gpio_evt_queue, &temp_stat, NULL);
}   

void digital_input_init(void) {
    gpio_config_t io_conf = {};
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // Set pullup/pulldowns
    io_conf.pull_up_en = (gpio_pullup_t)0;
    io_conf.pull_down_en = (gpio_pulldown_t)0;

    gpio_config(&io_conf);
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_0, digital_input_isr_handler, (void*) GPIO_INPUT_0);
    gpio_isr_handler_add(GPIO_INPUT_0, digital_input_isr_handler, (void*) GPIO_INPUT_1);
}

void adc_oneshot_init(void)
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
	
  int numChannels = sizeof(channels)/sizeof(adc_channel_t);
	for (int i=0; i<numChannels; i++)
	{
		ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, channels[i], &channelConfig));
        bool cali_check = adc_calibration_init(ADC_UNIT_1, channels[i], ADC_ATTEN_DB_12, &adc1_cali_handle[i]);
        if(cali_check) {
            ESP_LOGI(TAG, "Channel %i calibrated!", channels[i]);
        } else {
            ESP_LOGE(TAG, "Channel %i failed to calibrate", channels[i]);
        }
	}
}

void adc_to_queue_task(void* pvParameters) {
    while (1) { 
        // 1. Count items from queue (by popping?)
        // 2. Reset queue
        // 3. Send count to another queue
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void digital_input_to_queue_task(void* pvParameters) {
    while (1) { 
        std::ostringstream data_oss;
        for(int i = 0; i < ADC_COUNT; i++) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channels[i], &adc_raw[i]));
            sensors[i] = 3.3*adc_raw[i]/4095;
            data_oss << sensors[i];
            if (i != ADC_COUNT - 1) data_oss << ",";
        }
        
        std::string data_str = data_oss.str();
        if(c_sock_connected) {
            BaseType_t tx_result = xQueueSend(dataQueue, data_str.c_str(), (TickType_t)0);
            if(tx_result != pdPASS) {
                ESP_LOGE(TAG, "Push to queue failed with error: %i, queue reset", tx_result);
                xQueueReset(dataQueue);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
