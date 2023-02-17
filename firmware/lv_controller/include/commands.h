#ifndef COMMANDS_H
#define COMMANDS_H

#include "stdlib.h"
#include "pn532.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define EN_GPIO GPIO_NUM_1
#define DI_GPIO GPIO_NUM_2
#define PWM_GPIO GPIO_NUM_3
#define DIR_GPIO GPIO_NUM_4

#define LEDC_GPIO PWM_GPIO
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0

#define SOLENOID_GPIO GPIO_NUM_38

// NFC reader global pointer
extern pn532_t *nfc_reader;

int calc_bits_from_duty(int duty);

int lock_solenoid(int argc, char **argv);
int unlock_solenoid(int argc, char **argv);
int read_single_nfc_tag(int argc, char **argv);
int set_pwm(int argc, char **argv);
int set_mtr_dir(int argc, char **argv);
int enable_motor_output(int argc, char **argv);
int disable_motor_output(int argc, char **argv);

#endif