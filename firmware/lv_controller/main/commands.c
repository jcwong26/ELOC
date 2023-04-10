
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "esp_log.h"
#include <driver/gpio.h>

#include "commands.h"
#include "solenoid.h"
#include "state_machine.h"
#include "motor.h"

static const char *TAG = "commands";

/* Helpers */
int calc_bits_from_duty(int duty)
{
    if (duty > 100 || duty < 0)
    {
        printf("Error setting duty cycle. Duty value invalid: %d\n", duty);
        ESP_LOGE(TAG, "Error setting duty cycle. Duty value invalid: %d\n", duty);
    }
    printf("Setting duty to: %d%%\n", duty);
    // ESP_LOGI(TAG, "Setting duty to: %d%%\n", duty);
    int calc_duty = (int)((pow(2, 10) - 1) * (duty / 100.0));
    // printf("Duty bits: %d\n", calc_duty);
    // ESP_LOGI(TAG, "Duty bits: %d\n", calc_duty);
    return calc_duty; // 10 bit res
}

/* Commands */
int lock_solenoid_comm(int argc, char **argv)
{
    // Set level low (0) to lock
    ESP_ERROR_CHECK(gpio_set_level(SOLENOID_GPIO, 0));
    printf("Set solenoid level low to lock\n");
    return 0;
}

int unlock_solenoid_comm(int argc, char **argv)
{
    // Set level high (1) to unlock
    ESP_ERROR_CHECK(gpio_set_level(SOLENOID_GPIO, 1));
    printf("Set solenoid level high to unlock\n");
    return 0;
}

int set_pwm(int argc, char **argv)
{
    // Expecting 2 arguments for this function
    if (argc != 2)
    {
        printf("Incorrect number of arguments %d\n", argc);
        ESP_LOGE(TAG, "Incorrect number of arguments %d\n", argc);
        return 0;
    }
    char *duty_str = argv[1]; // 1st index is command, second is arg
    int duty = atoi(duty_str);

    // Set duty
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, calc_bits_from_duty(duty))); // Set duty to XX%. ((2 ** 10) - 1) * XX% = # bits
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    // ESP_ERROR_CHECK(gpio_set_level(PWM_GPIO, 1));
    // printf("Set motor PWM to 1\n");
    return 0;
}

int set_mtr_dir(int argc, char **argv)
{
    // Expecting 2 arguments for this function
    if (argc != 2)
    {
        printf("Incorrect number of arguments: %d\n", argc);
        ESP_LOGE(TAG, "Incorrect number of arguments: %d\n", argc);
        return 0;
    }
    char *arg_str = argv[1]; // 1st index is command, second is arg
    int arg = atoi(arg_str);

    if (arg != 0 && arg != 1)
    {
        printf("Incorrect direction passed: %d\n", arg);
        ESP_LOGE(TAG, "Incorrect direction passed: %d\n", arg);
        return 0;
    }

    if (arg)
    {
        ESP_ERROR_CHECK(gpio_set_level(DIR_GPIO, 1));
        printf("Set motor direction to Forward (1)\n");
    }
    else
    {
        ESP_ERROR_CHECK(gpio_set_level(DIR_GPIO, 0));
        printf("Set motor direction to Reverse (0)\n");
    }

    return 0;
}

int enable_motor_output(int argc, char **argv)
{
    // Set EN high and DI low to enable output
    ESP_ERROR_CHECK(gpio_set_level(EN_GPIO, 1));
    ESP_ERROR_CHECK(gpio_set_level(DI_GPIO, 0));
    printf("Enabling motor output...\n");
    return 0;
}

int disable_motor_output(int argc, char **argv)
{
    // Set EN low and DI high to disable output
    ESP_ERROR_CHECK(gpio_set_level(EN_GPIO, 0));
    ESP_ERROR_CHECK(gpio_set_level(DI_GPIO, 1));
    printf("Disabling motor output...\n");
    return 0;
}

int sled_out_comm(int argc, char **argv)
{
    sled_out();
    return 0;
}

int sled_in_comm(int argc, char **argv)
{
    sled_in();
    return 0;
}

int stop_sled_comm(int argc, char **argv)
{
    stop_sled();
    return 0;
}

int to_unlockedem_comm(int argc, char **argv)
{
    return to_unlockedem();
}

int to_loading_comm(int argc, char **argv)
{
    return to_loading();
}
int to_closed_comm(int argc, char **argv)
{
    return to_closed();
}
int to_compvision_comm(int argc, char **argv)
{
    return to_compvision();
}
int to_charging_comm(int argc, char **argv)
{
    return to_charging();
}
int to_unlocked_comm(int argc, char **argv)
{
    return to_unlocked();
}
int to_unloading_comm(int argc, char **argv)
{
    return to_unloading();
}
int to_empty_comm(int argc, char **argv)
{
    return to_empty();
}