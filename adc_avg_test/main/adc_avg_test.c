#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"
#include "esp_timer.h"

#define READ_LEN                  256
#define ADC_UNIT                  ADC_UNIT_1
#define ADC_CONV_MODE             ADC_CONV_SINGLE_UNIT_1
#define ADC_ATTEN                 ADC_ATTEN_DB_0
#define ADC_BIT_WIDTH             SOC_ADC_DIGI_MAX_BITWIDTH
#define ADC_OUTPUT_TYPE           ADC_DIGI_OUTPUT_FORMAT_TYPE1

#define GET_CHANNEL(p_data)       ((p_data)->type1.channel)
#define GET_DATA(p_data)          ((p_data)->type1.data)

static const adc_channel_t adc_channel = ADC_CHANNEL_6;
static TaskHandle_t adc_task_handle;
static const char *TAG = "ADC_AVG_SAMPLE";

static bool IRAM_ATTR adc_conversion_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    vTaskNotifyGiveFromISR(adc_task_handle, &mustYield);
    return mustYield == pdTRUE;
}

static void adc_continuous_init(adc_channel_t channel, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = 1024,
        .conv_frame_size = READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20000,
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN,
        .channel = channel & 0x7,
        .unit = ADC_UNIT,
        .bit_width = ADC_BIT_WIDTH,
    };

    dig_cfg.adc_pattern = &adc_pattern;
    dig_cfg.pattern_num = 1;

    ESP_LOGI(TAG, "ADC Channel Config - Atten: %"PRIx8", Channel: %"PRIx8", Unit: %"PRIx8,
             adc_pattern.atten, adc_pattern.channel, adc_pattern.unit);

    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));
    *out_handle = handle;
}

void app_main(void)
{
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[READ_LEN] = {0};

    adc_task_handle = xTaskGetCurrentTaskHandle();

    adc_continuous_handle_t adc_handle = NULL;
    adc_continuous_init(adc_channel, &adc_handle);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = adc_conversion_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

    int64_t last_print_time = esp_timer_get_time();
    uint32_t sum = 0;
    uint32_t count = 0;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (1) {
            ret = adc_continuous_read(adc_handle, result, READ_LEN, &ret_num, 0);
            if (ret == ESP_OK) {
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
                    uint32_t chan_num = GET_CHANNEL(p);
                    uint32_t data = GET_DATA(p);
                    if (chan_num == (adc_channel & 0x7)) {
                        sum += data;
                        count++;
                    }
                }

                // Check if 500ms has passed
                int64_t now = esp_timer_get_time();
                if ((now - last_print_time) >= 500000) { // 500ms = 500000us
                    if (count > 0) {
                        uint32_t avg = sum / count;
                        ESP_LOGI(TAG, "Average value over 500ms: %" PRIu32, avg);
                    } else {
                        ESP_LOGW(TAG, "No valid samples received in last 500ms");
                    }
                    sum = 0;
                    count = 0;
                    last_print_time = now;
                }
            } else if (ret == ESP_ERR_TIMEOUT) {
                break;
            }
        }
    }

    ESP_ERROR_CHECK(adc_continuous_stop(adc_handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(adc_handle));
}
