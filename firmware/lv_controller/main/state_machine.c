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
#include "commands.h"

enum states
{
    Loading,
    Closed,
    Charging,
    Unlocked,
    Unloading,
    Empty,
    Vacant,
    BADKEY = -1
};

static const char *TAG = "state_machine";
static enum states curr_state = Vacant;

/*
    DON'T NEED SWITCH CASE, ALREADY HANDLING COMMAND PARSING THROUGH THE CONSOLE APP
    -> INSTEAD, MAKE STATE FUNCS CALLABLE FROM CONSOLE
 */

// typedef struct
// {
//     char *key;
//     int val;
// } lookupstruct_t;

// static lookupstruct_t lookuptable[] = {
//     {"ToLoading", ToLoading}, {"ToClosed", ToClosed}, {"ToCharging", ToCharging}, {"ToUnlocked", ToUnlocked}, {"ToUnloading", ToUnloading}, {"ToEmpty", ToEmpty}, {"ToVacant", ToVacant}};

// #define NKEYS (sizeof(lookuptable) / sizeof(lookupstruct_t))

// int keyfromstring(char *key)
// {
//     for (int i = 0; i < NKEYS; i++)
//     {
//         lookupstruct_t *table = &lookuptable[i];
//         if (strcmp(table->key, key) == 0)
//             return table->val;
//     }
//     return BADKEY;
// }

// void state_machine(void)
// {
//     int ret = 0;
//     char *command = NULL;
//     while (1)
//     {
//         // wait for usb command
//         switch (keyfromstring(command))
//         {
//         case ToLoading:
//             ret = loading();
//             if (ret)
//                 ESP_LOGE(TAG, "ERROR: Could not transition to loading state...");
//             break;
//         case ToClosed:
//             ret = closed();
//             if (ret)
//                 ESP_LOGE(TAG, "ERROR: Could not transition to closed state...");
//             break;
//         case ToCharging:
//             ret = charging();
//             if (ret)
//                 ESP_LOGE(TAG, "ERROR: Could not transition to charging state...");
//             break;
//         case ToUnlocked:
//             ret = unlocked();
//             if (ret)
//                 ESP_LOGE(TAG, "ERROR: Could not transition to unlocked state...");
//             break;
//         case ToUnloading:
//             ret = unloading();
//             if (ret)
//                 ESP_LOGE(TAG, "ERROR: Could not transition to unloading state...");
//             break;
//         case ToEmpty:
//             ret = empty();
//             if (ret)
//                 ESP_LOGE(TAG, "ERROR: Could not transition to empty state...");
//             break;
//         case ToVacant:
//             ret = vacant();
//             if (ret)
//                 ESP_LOGE(TAG, "ERROR: Could not transition to vacant state...");
//             break;
//         case BADKEY: /* handle failed lookup */
//             ESP_LOGE(TAG, "ERROR: BADKEY detect -> invalid command received...");
//             break;
//         default:
//             break;
//         }
//     }
// }

int to_loading(void)
{
    // Move sled out
    sled_out();

    // Poll until limit switch is hit for SLED OUT, then stop the sled (LIM1)
    while (LIM1_state)
    {
    }

    // Stop sled
    stop_sled();

    // Set new state
    curr_state = Loading;
    return 0;
}

int to_closed(void)
{
    // Move sled in
    sled_in();

    // Poll until limit switch is hit for SLED IN, then stop the sled (LIM3)
    while (LIM3_state)
    {
    }

    // Poll until limit switch is hit for DOOR CLOSED, then stop the sled (LIM4)
    while (LIM4_state)
    {
    }

    // Lock door
    lock_solenoid();

    // Set new state
    curr_state = Closed;
    return 0;
}

int to_charging(void)
{
    // Turn LEDs on white for CV
    white_leds();

    // Set new state
    curr_state = Charging;
    return 0;
}

int to_unlocked(void)
{
    // Turn LEDs off
    leds_off();

    // Unlock door
    unlock_solenoid();

    // Set new state
    curr_state = Unlocked;
    return 0;
}

int to_unloading(void)
{
    // Poll until limit switch is hit for DOOR OPEN (LIM4)
    while (!LIM4_state)
    {
    }

    // Move sled out
    sled_out();

    // Poll until limit switch is hit for SLED OUT, then stop the sled (LIM1)
    while (LIM1_state)
    {
    }

    // Stop sled
    stop_sled();

    // Set new state
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
    }

    // Poll until limit switch is hit for DOOR CLOSED, then stop the sled (LIM4)
    while (LIM4_state)
    {
    }

    // Set new state
    curr_state = Empty;
    return 0;
}