#include "esp_log.h"
#include <driver/gpio.h>

#include "solenoid.h"

void lock_solenoid(void)
{
    // Set level low (0) to lock
    ESP_ERROR_CHECK(gpio_set_level(SOLENOID_GPIO, 0));
    printf("Set solenoid level low to lock\n");
}

void unlock_solenoid(void)
{ // Set level high (1) to unlock
    ESP_ERROR_CHECK(gpio_set_level(SOLENOID_GPIO, 1));
    printf("Set solenoid level high to unlock\n");
}