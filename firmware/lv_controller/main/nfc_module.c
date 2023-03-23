#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "nfc_module.h"
#include "state_machine.h"
#include "pn532.h"

enum nfc_states
{
    Vacant,
    UnlockEm,
    Load,
    Close,
    WaitForBBBFin,
    Unlock,
    Unload,
    Empty
};

static const char *TAG = "nfc_module";
pn532_t *nfc_reader = NULL;
static enum nfc_states next_state = Vacant;
static uint8_t curr_uid[100] = {'\0'};
static uint8_t curr_uidLength = 0;

int init_nfc_reader(void)
{
    nfc_reader = pn532_init(0, 43, 44, 0); // passing UART0, tx=43, rx=44, 0 for output bits rn

    // 5 retries before giving up
    int count = 0;
    int MAX_NUM_RETRIES = 5;
    while (nfc_reader == NULL)
    {
        nfc_reader = pn532_init(0, 43, 44, 0);
        count++;
        if (count > MAX_NUM_RETRIES)
        {
            break;
        }
    }
    if (nfc_reader != NULL)
    {
        ESP_LOGI(TAG, "NFC Module Initialized");
        return 1;
    }
    ESP_LOGE(TAG, "NFC Module NOT Initialized...");
    return 0;
}

void read_single_nfc_tag(void *)
{
    int ret = init_nfc_reader();
    if (ret)
    {
        printf("Searching for tags...\n");
    }

    while (ret)
    {
        uint8_t uid[100] = {};
        uint8_t uidLength;
        if (nfc_reader == NULL)
        {
            printf("NFC Reader is NULL");
        }

        int res = pn532_Cards_and_return_data(nfc_reader, &uid[0], &uidLength);
        while (res <= 0)
        {
            res = pn532_Cards_and_return_data(nfc_reader, &uid[0], &uidLength);
            // usleep(2000000);
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        char uid_str[16 * 4];
        int index = 0;
        for (uint8_t i = 0; i < uidLength; i++)
        {
            index += sprintf(&uid_str[index], "%d ", uid[i]);
        }
        printf("Detected NFC Tag with uid: %s\n", uid_str);
        ESP_LOGI(TAG, "Detected NFC Tag with uid: %s", uid_str);
        nfc_state_machine(uid, uidLength);
        vTaskDelay(pdMS_TO_TICKS(5000)); // delay 5s to ensure states are properly handled
    }
    vTaskDelete(NULL); // if module wasn't initialized, delete this task
}

void nfc_state_machine(uint8_t uid[], uint8_t uidLength)
{
    int ret = 0;
    switch (next_state)
    {
    case Vacant:
        ret = new_tag(uid, uidLength);
        if (ret)
        {
            ESP_LOGE(TAG, "ERROR: Could not register new tag...");
            break;
        }
        ret = to_unlockedem();
        if (ret)
        {
            ESP_LOGE(TAG, "ERROR: Could not transistion to unlocked (empty) state...");
            break;
        }
        next_state = UnlockEm;
        break;
    case UnlockEm:
        ret = check_tag(uid, uidLength);
        if (ret)
        {
            ESP_LOGI(TAG, "INFO: Detected unsaved tag...");
            break;
        }
        ret = to_loading();
        if (ret)
        {
            ESP_LOGE(TAG, "ERROR: Could not transistion to loading state...");
            break;
        }
        next_state = Load;
        break;
    case Load:
        ret = check_tag(uid, uidLength);
        if (ret)
        {
            ESP_LOGI(TAG, "INFO: Detected unsaved tag...");
            break;
        }
        ret = to_closed();
        if (ret)
        {
            ESP_LOGE(TAG, "ERROR: Could not transistion to closed state...");
            break;
        }
        // "Close" NFC module state skipped for

        ret = to_compvision(); // send msg to BBB that bike is now in and door is closed/locked
        if (ret)
        {
            ESP_LOGE(TAG, "ERROR: Could not transistion to comp. vision state...");
            break;
        }
        // Transition to charging state happens from BBB automatically, no need for NFC input

        next_state = WaitForBBBFin;
        break;
        // case Close:
        //     ret = check_tag(uid, uidLength);
        //     if (ret)
        //     {
        //         ESP_LOGI(TAG, "INFO: Detected unsaved tag...");
        //         break;
        //     }
        //     ret = to_compvision();
        //     if (ret)
        //     {
        //         ESP_LOGE(TAG, "ERROR: Could not transistion to comp. vision state...");
        //         break;
        //     }
        //     // Transition to charging state happens from BBB automatically, no need for NFC input

        // next_state = Unlock;
        // break;
    case WaitForBBBFin:
        ret = check_tag(uid, uidLength);
        if (ret)
        {
            ESP_LOGI(TAG, "INFO: Detected unsaved tag...");
            break;
        }
        printf("WaitForBBBFin_state\n");
        next_state = Unload;
        break;
    // case Unlock:
    //     ret = check_tag(uid, uidLength);
    //     if (ret)
    //     {
    //         ESP_LOGI(TAG, "INFO: Detected unsaved tag...");
    //         break;
    //     }
    //     ret = to_unlocked();
    //     if (ret)
    //     {
    //         ESP_LOGE(TAG, "ERROR: Could not transistion to unlocked state...");
    //         break;
    //     }
    //     next_state = Unload;
    //     break;
    case Unload:
        ret = check_tag(uid, uidLength);
        if (ret)
        {
            ESP_LOGI(TAG, "INFO: Detected unsaved tag...");
            break;
        }
        ret = to_unloading();
        if (ret)
        {
            ESP_LOGE(TAG, "ERROR: Could not transistion to unloaded state...");
            break;
        }
        next_state = Empty;
        break;
    case Empty:
        ret = check_tag(uid, uidLength);
        if (ret)
        {
            ESP_LOGI(TAG, "INFO: Detected unsaved tag...");
            break;
        }
        ret = to_empty();
        if (ret)
        {
            ESP_LOGE(TAG, "ERROR: Could not transistion to empty state...");
            break;
        }
        delete_tag();
        next_state = Vacant;
        break;
    default:
        break;
    }
}

int new_tag(uint8_t uid[], uint8_t uidLength)
{
    if (curr_uid[0] == '\0')
    {
        // No uid is set currently, so set the uid detected
        for (int i = 0; i < uidLength; i++)
        {
            curr_uid[i] = uid[i];
        }
        curr_uidLength = uidLength;
        return 0;
    }
    return 1;
}

int check_tag(uint8_t uid[], uint8_t uidLength)
{
    if (curr_uidLength == uidLength)
    {
        // Compare uids for further processing
        for (int i = 0; i < uidLength; i++)
        {
            if (curr_uid[i] != uid[i])
                return 0; // not matching id, return false
        }
        return 0; // id matched! obv don't need to update
    }
    return 1;
}

void delete_tag(void)
{
    memset(curr_uid, '\0', sizeof(curr_uid)); // reset uid
    curr_uidLength = 0;                       // reset uid length
}