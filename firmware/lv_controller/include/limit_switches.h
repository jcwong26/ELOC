#ifndef LIMIT_SWITCHES_H
#define LIMIT_SWITCHES_H

#include "driver/gpio.h"

#define LIM1_GPIO GPIO_NUM_13
#define LIM2_GPIO GPIO_NUM_14
#define LIM3_GPIO GPIO_NUM_15
#define LIM4_GPIO GPIO_NUM_16

extern int LIM1_state;
extern int LIM2_state;
extern int LIM3_state;
extern int LIM4_state;

void init_limit_switches(void);
static void IRAM_ATTR gpio_interrupt_handler(void *args);
void lim_switch_read(void *params);
int get_lim_switch_curr_value(int pinNumber);

#endif