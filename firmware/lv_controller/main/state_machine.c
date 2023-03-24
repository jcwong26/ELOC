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
    UnlockedEm,
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

int to_unlockedem(void)
{
    // Start a heartbeat on the ring light in prep for loading
    heartbeat_start();

    // Unlock door
    unlock_solenoid();

    // Poll until limit switch is de-pressed for DOOR is OPEN (LIM4)
    LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
    while (!LIM4_state)
    {
        LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Lock solenoid to prevent overheating
    printf("Locking solenoid...\n");
    lock_solenoid();

    // Set new state
    printf("UNLOCKEDEM_state\n");
    curr_state = UnlockedEm;
    return 0;
}

int to_loading(void)
{
    // Move sled out
    sled_out();

    printf("Waiting for LIM1 (SLED OUT)...\n");
    // Poll until limit switch is hit for SLED OUT (LIM1), then stop the sled (LIM1)
    LIM1_state = get_lim_switch_curr_value(LIM1_GPIO);
    while (LIM1_state)
    {
        LIM1_state = get_lim_switch_curr_value(LIM1_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    printf("Stopping sled...\n");
    // Stop sled
    stop_sled();

    // Set new state
    printf("LOADING_state\n");
    curr_state = Loading;
    return 0;
}

int to_closed(void)
{
    // Move sled in
    printf("Moving sled in...\n");
    sled_in();

    // Poll until limit switch is hit for SLED IN, then stop the sled (LIM3)
    LIM3_state = get_lim_switch_curr_value(LIM3_GPIO);
    while (LIM3_state)
    {
        LIM3_state = get_lim_switch_curr_value(LIM3_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    printf("Stopping sled...\n");
    stop_sled();

    // Poll until limit switch is hit for DOOR CLOSED
    LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
    while (LIM4_state)
    {
        LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Set new state
    printf("CLOSED_state\n");
    curr_state = Closed;
    return 0;
}

int to_compvision(void)
{
    // Turn off heartbeat from loading
    heartbeat_stop();
    vTaskDelay(pdMS_TO_TICKS(100)); // give the ring light some time to settle

    // Turn LEDs on white for CV
    white_leds();

    // Set new state
    printf("COMPVISION_state\n");
    curr_state = CompVision;
    return 0;
}

int to_charging(void)
{
    // Turn on rainbow chase for charging
    rainbow_chase_start();

    // Set new state
    printf("CHARGING_state\n");
    curr_state = Charging;
    return 0;
}

int to_unlocked(void)
{
    // Stop rainbow chase
    rainbow_chase_stop();

    // Turn LEDs off
    leds_off();

    // Start a heartbeat on the ring light for unloading
    heartbeat_start();

    // Unlock door
    unlock_solenoid();

    // Poll until limit switch is de-pressed for DOOR is OPEN (LIM4)
    LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
    while (!LIM4_state)
    {
        LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Lock solenoid to prevent overheating
    lock_solenoid();

    // Set new state
    printf("UNLOCKED_state\n");
    curr_state = Unlocked;
    return 0;
}

int to_unloading(void)
{
    // Move sled out
    sled_out();

    // Poll until limit switch is hit for SLED OUT, then stop the sled (LIM1)
    LIM1_state = get_lim_switch_curr_value(LIM1_GPIO);
    while (LIM1_state)
    {
        LIM1_state = get_lim_switch_curr_value(LIM1_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Stop sled
    stop_sled();

    // Set new state
    printf("UNLOADING_state\n");
    curr_state = Unloading;
    return 0;
}

int to_empty(void)
{
    // Move sled in
    sled_in();

    // Poll until limit switch is hit for SLED IN, then stop the sled (LIM3)
    LIM3_state = get_lim_switch_curr_value(LIM3_GPIO);
    while (LIM3_state)
    {
        LIM3_state = get_lim_switch_curr_value(LIM3_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Stop the sled
    stop_sled();

    // Poll until limit switch is pressed for DOOR is CLOSED (LIM4)
    LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
    while (LIM4_state)
    {
        LIM4_state = get_lim_switch_curr_value(LIM4_GPIO);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Turn off heartbeat from unloading
    heartbeat_stop();

    // Set new state
    printf("EMPTY_state\n");
    curr_state = Empty;
    return 0;
}