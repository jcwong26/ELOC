#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "nfc_module.h"
#include "pn532.h"

static const char *TAG = "nfc_module";
pn532_t *nfc_reader = NULL;

void init_nfc_reader(void)
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
    }
    else
    {
        ESP_LOGE(TAG, "NFC Module NOT Initialized...");
    }
}

void read_single_nfc_tag(void *)
{
    init_nfc_reader();
    printf("Searching for tags...\n");
    while (1)
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
    }
}