#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "limit_switches.h"

/* GLOBALS */
int state = 0;
QueueHandle_t interuptQueue;

void init_limit_switches(void)
{
    interuptQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(lim_switch_read, "lim_switch_read", 2048, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(LIM1_GPIO, gpio_interrupt_handler, (void *)LIM1_GPIO);
    gpio_isr_handler_add(LIM2_GPIO, gpio_interrupt_handler, (void *)LIM2_GPIO);
    gpio_isr_handler_add(LIM3_GPIO, gpio_interrupt_handler, (void *)LIM3_GPIO);
    gpio_isr_handler_add(LIM4_GPIO, gpio_interrupt_handler, (void *)LIM4_GPIO);
}

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    xQueueSendFromISR(interuptQueue, &pinNumber, NULL);
}

void lim_switch_read(void *params)
{
    int pinNumber, temp_count = 0;
    int count1 = 1;
    int count2 = 1;
    int count3 = 1;
    int count4 = 1;
    while (true)
    {
        if (xQueueReceive(interuptQueue, &pinNumber, portMAX_DELAY))
        {
            if (pinNumber == LIM1_GPIO)
                temp_count = count1++;
            else if (pinNumber == LIM2_GPIO)
                temp_count = count2++;
            else if (pinNumber == LIM3_GPIO)
                temp_count = count3++;
            else if (pinNumber == LIM4_GPIO)
                temp_count = count4++;
            printf("GPIO %d was pressed %d times. The state is %d\n", pinNumber, temp_count, gpio_get_level(pinNumber));
        }
    }
}