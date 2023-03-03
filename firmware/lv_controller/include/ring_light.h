#ifndef RING_LIGHT_H
#define RING_LIGHT_H

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM 37
#define RING_LIGHT_GPIO GPIO_NUM_37

#define NUM_LEDS 24
#define LED_CHASE_SPEED_MS 35

static void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
void init_ring_light(void);
int rainbow_chase_start(int argc, char **argv);
int rainbow_chase_stop(int argc, char **argv);
void rainbow_chase_inf(void *);
int set_white_leds(int argc, char **argv);
int leds_off(int argc, char **argv);

#endif