#include "v_sense.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_err.h"

#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static adc_continuous_handle_t handle = NULL;
static adc_cali_handle_t calib_handle = NULL;

static QueueHandle_t current_queue = NULL;
static QueueHandle_t voltage_queue = NULL;

static bool check_valid_data(const adc_digi_output_data_t *data){
    if (data->type1.channel >= SOC_ADC_CHANNEL_NUM(ADC_UNIT_1)) {
        return false;
    }
    return true;
}

void init_adc(void){
    
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 2048,
        .conv_frame_size = FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 80 * 1000, // 20K is lowest sampling freq
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = 2;   // 2 ADC Channels will be used

    // Config I_Sense Input
    adc_pattern[0].atten = ADC1_ATTEN;     // 0-2400mV
    adc_pattern[0].channel = I_SENSE_CHANNEL;
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    // Config 24V Sense Input
    adc_pattern[1].atten = ADC1_ATTEN;      //ADC_ATTEN_DB_6 is 0 - 1300mV
    adc_pattern[1].channel = V_24V_CHANNEL;
    adc_pattern[1].unit = ADC_UNIT_1;
    adc_pattern[1].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));  
    ESP_ERROR_CHECK(adc_continuous_start(handle));
    ESP_LOGI("ADC", "ADC Started Successfully!");
}

void calibrate_adc(void){
    //Characterize ADC at particular atten
    esp_err_t ret;
    bool cali_enable = false;

    ESP_LOGI("ADC", "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC1_ATTEN,
        .bitwidth = SOC_ADC_DIGI_MAX_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &calib_handle));
    }

int convert_to_cal_mV(uint16_t raw_value){
    int result_mV = 0;
    adc_cali_raw_to_voltage(calib_handle, raw_value, &result_mV);
    return result_mV;
}

void power_monitoring_task(void*){
    // Setup queue or something here
    ESP_LOGI("ADC","test");
}

void poll_adc_task(void*){
    // Setup Queue or something here
    uint8_t result[FRAME_SIZE] = {0};
    uint32_t ret_num = 0;
    uint32_t I_sum = 0;
    uint16_t I_count = 0;

    uint32_t V_sum = 0;
    uint16_t V_count = 0;
    esp_err_t ret;
    while (1){
        ret = adc_continuous_read(handle, result, FRAME_SIZE, &ret_num, 0);
        if (ret == ESP_OK) {
        // Just get last 2 conversion results
            // for (int i = ret_num-(2*SOC_ADC_DIGI_RESULT_BYTES); i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
            for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                adc_digi_output_data_t *p = (void*)&result[i];
                if (check_valid_data(p)) {
                    switch (p->type1.channel)
                    {
                    case 4:     // Current Shunt Sensor
                        I_sum += p->type1.data;
                        I_count++;
                        
                        break;
                    
                    case 5:     // 24V bus sensor
                        V_sum += p->type1.data;
                        V_count++;
                        break; 

                    default:
                        break;
                    }
                    
                } else {
                    ESP_LOGI("ADC", "Invalid data");
                }
            }
            // Print out avg of results
            float I_avg = 1000*VAL_TO_VOLTS(I_sum/(float)(I_count));
            float V_avg =  CONVERT_V24(VAL_TO_VOLTS(V_sum/(float)(V_count)));
            ESP_LOGI("ADC", "DIff Voltage: %.2fmV 12V Input Current: %.2fA", I_avg, V_TO_I_SHUNT(I_avg/1000.0));
            ESP_LOGI("ADC", "24V Bus Voltage: %.2fV", V_avg);

            // Reset sum + counters
            I_sum = 0;
            I_count =0;

            V_sum = 0;
            V_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}