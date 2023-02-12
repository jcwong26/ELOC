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
}

void current_monitoring_task(void*){
    // Setup queue or something here
    printf("test");
}

void supply_voltage_monitor_task(void*){
    // Setup Queue or something here
    printf("test");
}