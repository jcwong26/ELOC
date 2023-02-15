#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "inverter.h"

ledc_channel_config_t ledc_channel = {
    .speed_mode     = LEDC_MODE,
    .channel        = LEDC_CHANNEL_0,
    .timer_sel      = LEDC_TIMER,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = BRIDGE_CTRL_PIN,
    .duty           = LEDC_DUTY, // Set duty to 100%
    .hpoint         = 0
};

ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_MODE,
    .timer_num        = LEDC_TIMER,
    .duty_resolution  = LEDC_DUTY_RES,
    .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
    // .clk_cfg          = LEDC_AUTO_CLK,
    .clk_cfg          = LEDC_USE_APB_CLK
};

bool inv_pwr_status = false;

void init_inverter(void){
    // Setup PWM Timer
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    ESP_LOGI("PWM INIT", "Successfully Initialized PWM Timer");
}

void turn_on_inv_rail(void){
    gpio_set_level(V_INV_ENABLE_PIN, 1);
    inv_pwr_status = true;
}

void turn_off_inv_rail(void){
    gpio_set_level(V_INV_ENABLE_PIN, 0);
    inv_pwr_status = false;
}

void set_sw_freq(uint32_t f_sw){
    printf("Setting Frequency to %.1f kHz\n", (f_sw/1000.0));
    ledc_set_freq(LEDC_MODE, LEDC_CHANNEL_0, f_sw);
}

void enable_bridge(void){
    gpio_set_level(NOT_SHUTDOWN_PIN, 1);
    set_sw_freq(DEFAULT_SW_FREQ);       // Just set back to default switching freq
    // Set Status Pin:
    gpio_set_level(WPT_ACTIVE_LED_PIN, 0);
}

void disable_bridge(void){
    gpio_set_level(NOT_SHUTDOWN_PIN, 0);
    gpio_set_level(WPT_ACTIVE_LED_PIN, 1);
}

void turn_fan_on(void){
    gpio_set_level(FAN_CTRL_PIN, 1);
}

void turn_fan_off(void){
    gpio_set_level(FAN_CTRL_PIN, 0);
}

void flash_wpt_led(void*){
    static bool state = true;
    while(1){
        gpio_set_level(WPT_ACTIVE_LED_PIN, state);
        state = !state;
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}