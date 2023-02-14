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
        .format = ADC_OUTPUT_TYPE
    };


    ///ADC_ATTEN_DB_6 for 24V_Sense
}


void current_monitoring_task(void*){
    // Setup queue or something here
    ESP_LOGI("ADC","test");
}

void supply_voltage_monitor_task(void*){

    int voltage = 10;
    while (1){
        ESP_LOGI("ADC", "24V Rail Voltage: %.2f", CONVERT_V24(voltage));
    }
    // Setup Queue or something here
    ESP_LOGI("ADC","test");
}