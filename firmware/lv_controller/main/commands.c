
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "commands.h"
#include "pn532.h"
#include "esp_log.h"
#include <driver/gpio.h>

static const char *TAG = "commands";
// pn532_t *nfc_reader = NULL;

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
int lock_solenoid(int argc, char **argv)
{
    // Set level low (0) to lock
    ESP_ERROR_CHECK(gpio_set_level(SOLENOID_GPIO, 0));
    printf("Set solenoid level low to lock\n");
    return 0;
}

int unlock_solenoid(int argc, char **argv)
{
    // Set level high (1) to unlock
    ESP_ERROR_CHECK(gpio_set_level(SOLENOID_GPIO, 1));
    printf("Set solenoid level high to unlock\n");
    return 0;
}

// int read_single_nfc_tag(int argc, char **argv)
// {
//     printf("Searching for tags...\n");
//     uint8_t uid[100] = {};
//     uint8_t uidLength;
//     if (nfc_reader == NULL)
//     {
//         printf("NFC Reader is NULL");
//     }

//     int res = pn532_Cards_and_return_data(nfc_reader, &uid[0], &uidLength);
//     while (res <= 0)
//     {
//         res = pn532_Cards_and_return_data(nfc_reader, &uid[0], &uidLength);
//         usleep(2000000);
//     }

//     char uid_str[16 * 4];
//     int index = 0;
//     for (uint8_t i = 0; i < uidLength; i++)
//     {
//         index += sprintf(&uid_str[index], "%d ", uid[i]);
//     }
//     printf("Detected NFC Tag with uid: %s\n", uid_str);
//     ESP_LOGI(TAG, "Detected NFC Tag with uid: %s", uid_str);
//     return 0;
// }

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