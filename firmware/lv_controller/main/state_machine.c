#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "state_machine.h"

enum to_state
{
    ToLoading,
    ToClosed,
    ToCharging,
    ToUnlocked,
    ToUnloading,
    ToEmpty,
    ToVacant,
    BADKEY = -1
};

static const char *TAG = "state_machine";

typedef struct
{
    char *key;
    int val;
} lookupstruct_t;

static lookupstruct_t lookuptable[] = {
    {"ToLoading", ToLoading}, {"ToClosed", ToClosed}, {"ToCharging", ToCharging}, {"ToUnlocked", ToUnlocked}, {"ToUnloading", ToUnloading}, {"ToEmpty", ToEmpty}, {"ToVacant", ToVacant}};

#define NKEYS (sizeof(lookuptable) / sizeof(lookupstruct_t))

int keyfromstring(char *key)
{
    for (int i = 0; i < NKEYS; i++)
    {
        lookupstruct_t *table = &lookuptable[i];
        if (strcmp(table->key, key) == 0)
            return table->val;
    }
    return BADKEY;
}

void state_machine(void)
{
    int ret = 0;
    char *command = NULL;
    while (1)
    {
        // wait for usb command
        switch (keyfromstring(command))
        {
        case ToLoading:
            ret = loading();
            if (ret)
                ESP_LOGE(TAG, "ERROR: Could not transition to loading state...");
            break;
        case ToClosed:
            ret = closed();
            if (ret)
                ESP_LOGE(TAG, "ERROR: Could not transition to closed state...");
            break;
        case ToCharging:
            ret = charging();
            if (ret)
                ESP_LOGE(TAG, "ERROR: Could not transition to charging state...");
            break;
        case ToUnlocked:
            ret = unlocked();
            if (ret)
                ESP_LOGE(TAG, "ERROR: Could not transition to unlocked state...");
            break;
        case ToUnloading:
            ret = unloading();
            if (ret)
                ESP_LOGE(TAG, "ERROR: Could not transition to unloading state...");
            break;
        case ToEmpty:
            ret = empty();
            if (ret)
                ESP_LOGE(TAG, "ERROR: Could not transition to empty state...");
            break;
        case ToVacant:
            ret = vacant();
            if (ret)
                ESP_LOGE(TAG, "ERROR: Could not transition to vacant state...");
            break;
        case BADKEY: /* handle failed lookup */
            ESP_LOGE(TAG, "ERROR: BADKEY detect -> invalid command received...");
            break;
        default:
            break;
        }
    }
}

int loading(void)
{
    return 0;
}

int closed(void)
{
    return 0;
}

int charging(void)
{
    return 0;
}

int unlocked(void)
{
    return 0;
}

int unloading(void)
{
    return 0;
}

int empty(void)
{
    return 0;
}

int vacant(void)
{
    return 0;
}