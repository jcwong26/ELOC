

#include "motor.h"
#include "commands.h"

#define MTR_FWD 1
#define MTR_REV 0
#define STD_DUTY_CYCLE 100

void sled_out(void)
{
    // Set motor direction
    ESP_ERROR_CHECK(gpio_set_level(DIR_GPIO, MTR_FWD));
    printf("Set motor direction to Forward (1)\n");

    // Set duty cycle for PWM
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, calc_bits_from_duty(STD_DUTY_CYCLE)));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

    // Set EN high and DI low to enable output
    ESP_ERROR_CHECK(gpio_set_level(EN_GPIO, 1));
    ESP_ERROR_CHECK(gpio_set_level(DI_GPIO, 0));
    printf("Enabling motor output...\n");
}

void sled_in(void)
{
    ESP_ERROR_CHECK(gpio_set_level(DIR_GPIO, MTR_REV));
    printf("Set motor direction to Reverse (0)\n");

    // Set duty cycle for PWM
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, calc_bits_from_duty(STD_DUTY_CYCLE)));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

    // Set EN high and DI low to enable output
    ESP_ERROR_CHECK(gpio_set_level(EN_GPIO, 1));
    ESP_ERROR_CHECK(gpio_set_level(DI_GPIO, 0));
    printf("Enabling motor output...\n");
}

void stop_sled(void)
{
    // Set EN low and DI high to disable output
    ESP_ERROR_CHECK(gpio_set_level(EN_GPIO, 0));
    ESP_ERROR_CHECK(gpio_set_level(DI_GPIO, 1));
    printf("Disabling motor output...\n");
}
