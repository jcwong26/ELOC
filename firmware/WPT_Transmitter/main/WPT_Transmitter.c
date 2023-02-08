#include <stdio.h>
#include "driver/gpio.h"



// Pin defs

#define CURR_SENSE_PIN 5
#define V_INV_SENSE_PIN 6
#define V_INV_ENABLE_PIN 15
#define FAN_CTRL_PIN 8
#define BRIDGE_CTRL_PIN 10
#define NOT_SHUTDOWN_PIN 11
#define WPT_ACTIVE_LED_PIN 12

#define V_DIV_RATIO 0.0507614
#define MAX_ADC_OUTPUT 4095


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
}

void app_main(void)
{
    setup_gpio();

}
