#ifndef SOLENOID_H
#define SOLENOID_H

#include "driver/gpio.h"

#define SOLENOID_GPIO GPIO_NUM_38

void lock_solenoid(void);
void unlock_solenoid(void);

#endif