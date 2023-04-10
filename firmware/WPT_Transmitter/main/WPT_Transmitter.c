#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "inverter.h"
#include "v_sense.h"
#include "console.h"


void setup_gpio(void){
    gpio_reset_pin(V_INV_ENABLE_PIN);
    gpio_reset_pin(FAN_CTRL_PIN);
    gpio_reset_pin(BRIDGE_CTRL_PIN);
    gpio_reset_pin(NOT_SHUTDOWN_PIN);
    gpio_reset_pin(WPT_ACTIVE_LED_PIN);

    gpio_set_direction(V_INV_ENABLE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(FAN_CTRL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(BRIDGE_CTRL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(NOT_SHUTDOWN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(WPT_ACTIVE_LED_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(WPT_ACTIVE_LED_PIN, 1); // This turns off the LED
}

void app_main(void)
{
    setup_gpio();
    init_inverter();
    calibrate_adc();
    init_adc();
    
    turn_on_inv_rail();
    // turn_off_inv_rail();
    // printf("Setup Completed. Inverter Should be on");

    xTaskCreate(flash_wpt_led, "LED_blink_Task", 2500, NULL, 2, NULL);
    xTaskCreate(poll_adc_task, "V_monitor_task", 8000, NULL, 2, NULL);

    // Setup and start Console for user interaction
    initialize_nvs();
    initialize_console();
    run_console();
}
