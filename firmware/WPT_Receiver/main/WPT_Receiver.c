#include <stdio.h>

#include "esp_log.h"

void app_main(void)
{
    printf("Hello ESP!\n");
    ESP_LOGI("main", "Hello from the logger");
    printf("Hello ESP!\n");
    ESP_LOGI("main", "Hello from the logger");
    printf("Hello ESP!\n");
    ESP_LOGI("main", "Hello from the logger");
}
