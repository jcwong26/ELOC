#include "v_sense.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_adc/adc_continuous.h"


void init_adcs(void){
    adc_continuous_handle_t handle = NULL;
    
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = 100,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20E3, // 20K is lowest sampling freq
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE, 
        .pattern_num = 2,   // 2 ADC Channels will be used
    };

    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_continuous.html
    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc_oneshot.html#_CPPv4N25adc_digi_pattern_config_t4unitE

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};

    // Config I_Sense Input
    adc_pattern[0].atten = ADC_ATTEN_DB_11;     // 0-2400mV
    adc_pattern[0].channel = I_SENSE_CHANNEL;
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    // Config 24V Sense Input
    adc_pattern[1].atten = ADC_ATTEN_DB_6;      // 0 - 1300mV
    adc_pattern[1].channel = V_24V_CHANNEL;
    adc_pattern[1].unit = ADC_UNIT_1;
    adc_pattern[1].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    dig_cfg.adc_pattern = adc_pattern;

    // ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    adc_continuous_config(handle, &dig_cfg);        // Fails here it looks like

    ESP_LOGI("ADC", "ADC Configured Successfully!");
}


void current_monitoring_task(void*){
    // Setup queue or something here
    ESP_LOGI("ADC","test");
}

void supply_voltage_monitor_task(void*){
    // Setup Queue or something here
    int voltage = 1.21;
    while (1){
        ESP_LOGI("ADC", "24V Rail Voltage: %.2fV", CONVERT_V24(voltage));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}