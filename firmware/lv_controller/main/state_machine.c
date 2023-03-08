#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "state_machine.h"
#include "motor.h"
#include "limit_switches.h"
#include "ring_light.h"
#include "solenoid.h"

enum states
{
    Loading,
    Closed,
    CompVision,
    Charging,
    Unlocked,
    Unloading,
    Empty,
    Vacant,
    BADKEY = -1
};

static const char *TAG = "state_machine";
static enum states curr_state = Vacant;

int to_loading(void)
{
    // Unlock door
    unlock_solenoid();

    // Poll until limit switch is de-pressed for DOOR is OPEN (LIM4)
    while (!LIM4_state)
    {
        LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Lock door to prevent overheating
    printf("Locking solenoid...\n");
    lock_solenoid();

    // Move sled out
    sled_out();

    printf("Waiting for LIM1 (SLED OUT)...\n");
    // Poll until limit switch is hit for SLED OUT (LIM1), then stop the sled (LIM1)
    while (LIM1_state)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    printf("Stopping sled...\n");
    // Stop sled
    stop_sled();

    // Set new state
    printf("Now in LOADING state!\n");
    curr_state = Loading;
    return 0;
}

int to_closed(void)
{
    // Move sled in
    printf("Moving sled in...\n");
    sled_in();

    // Poll until limit switch is hit for SLED IN, then stop the sled (LIM3)
    while (LIM3_state)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    printf("Stopping sled...\n");
    stop_sled();

    // Poll until limit switch is hit for DOOR CLOSED
    while (LIM4_state)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Set new state
    printf("Now in CLOSED state!\n");
    curr_state = Closed;
    return 0;
}

int to_compvision(void)
{
    // Turn LEDs on white for CV
    white_leds();

    // Set new state
    printf("Now in COMPVISION state!\n");
    curr_state = CompVision;
    return 0;
}

int to_charging(void)
{
    // Turn on rainbow chase for charging
    rainbow_chase_start();

    // Set new state
    printf("Now in CHARGING state!\n");
    curr_state = Charging;
    return 0;
}

int to_unlocked(void)
{
    // Stop rainbow chase
    rainbow_chase_stop();

    // Turn LEDs off
    leds_off();

    // Unlock door
    unlock_solenoid();

    // Poll until limit switch is de-pressed for DOOR is OPEN (LIM4)
    while (!LIM4_state)
    {
        LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Lock solenoid to prevent overheating
    lock_solenoid();

    // Set new state
    printf("Now in UNLOCKED state!\n");
    curr_state = Unlocked;
    return 0;
}

int to_unloading(void)
{
    // Move sled out
    sled_out();

    // Poll until limit switch is hit for SLED OUT, then stop the sled (LIM1)
    while (LIM1_state)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Stop sled
    stop_sled();

    // Set new state
    printf("Now in UNLOADING state!\n");
    curr_state = Unloading;
    return 0;
}

int to_empty(void)
{
    // Move sled in
    sled_in();

    // Poll until limit switch is hit for SLED IN, then stop the sled (LIM3)
    while (LIM3_state)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Stop the sled
    stop_sled();

    // Set new state
    printf("Now in EMPTY/VACANT state!\n");
    curr_state = Empty;
    return 0;
}