#ifndef COMMANDS_H
#define COMMANDS_H

#include "stdlib.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define EN_GPIO GPIO_NUM_1
#define DI_GPIO GPIO_NUM_2
#define PWM_GPIO GPIO_NUM_3
#define DIR_GPIO GPIO_NUM_4

#define LEDC_GPIO PWM_GPIO
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0

int calc_bits_from_duty(int duty);

int lock_solenoid_comm(int argc, char **argv);
int unlock_solenoid_comm(int argc, char **argv);
int set_pwm(int argc, char **argv);
int set_mtr_dir(int argc, char **argv);
int enable_motor_output(int argc, char **argv);
int disable_motor_output(int argc, char **argv);

int sled_out_comm(int argc, char **argv);
int sled_in_comm(int argc, char **argv);
int stop_sled_comm(int argc, char **argv);

// State commands
int to_unlockedem_comm(int argc, char **argv);
int to_loading_comm(int argc, char **argv);
int to_closed_comm(int argc, char **argv);
int to_compvision_comm(int argc, char **argv);
int to_charging_comm(int argc, char **argv);
int to_unlocked_comm(int argc, char **argv);
int to_unloading_comm(int argc, char **argv);
int to_empty_comm(int argc, char **argv);

#endif