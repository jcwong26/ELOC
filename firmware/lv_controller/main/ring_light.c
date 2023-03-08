#include <stdint.h>
#include <string.h>
#include "ring_light.h"
#include "led_strip_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"

static const char *TAG = "ring_light";

static uint8_t led_strip_pixels[NUM_LEDS * 3];
static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static TaskHandle_t rainbow_chase_task_handle = NULL;
static bool go_rainbow_chase = false;

static void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i)
    {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void init_ring_light(void)
{
    ESP_LOGI(TAG, "Create RMT TX channel");
    printf("Create RMT TX channel");

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    printf("Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    printf("Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));
}

void rainbow_chase_start(void)
{
    if (go_rainbow_chase)
    {
        // chase already started, do nothing
        ESP_LOGI(TAG, "Rainbow chase already started");
        printf("Rainbow chase already started");
    }
    go_rainbow_chase = true;
    xTaskCreate(rainbow_chase_inf, "rainbow_chase_inf", 4096, NULL, 10, &rainbow_chase_task_handle);
}

int rainbow_chase_start_comm(int argc, char **argv)
{
    rainbow_chase_start();
    return 0;
}

void rainbow_chase_stop(void)
{
    go_rainbow_chase = false;
}

int rainbow_chase_stop_comm(int argc, char **argv)
{
    rainbow_chase_stop();
    return 0;
}

void rainbow_chase_inf(void *)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;

    ESP_LOGI(TAG, "Start LED rainbow chase");
    printf("Start LED rainbow chase");
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    while (go_rainbow_chase)
    {
        for (int i = 0; i < 3; i++)
        {
            for (int j = i; j < NUM_LEDS; j += 3)
            {
                // Build RGB pixels
                hue = j * 360 / NUM_LEDS + start_rgb;
                led_strip_hsv2rgb(hue, 100, 30, &red, &green, &blue);
                led_strip_pixels[j * 3 + 0] = green;
                led_strip_pixels[j * 3 + 1] = blue;
                led_strip_pixels[j * 3 + 2] = red;
            }
            // Flush RGB values to LEDs
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            vTaskDelay(pdMS_TO_TICKS(LED_CHASE_SPEED_MS));
            // memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
            // ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
            // vTaskDelay(pdMS_TO_TICKS(LED_CHASE_SPEED_MS));
        }

        memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
        ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
        start_rgb += 60;
    }
    vTaskDelete(NULL);
}

void white_leds(void)
{
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    ESP_LOGI(TAG, "Setting LEDs to white\n");
    printf("Setting LEDs to white\n");
    memset(led_strip_pixels, 255, sizeof(led_strip_pixels));
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
}

int set_white_leds(int argc, char **argv)
{
    white_leds();
    return 0;
}

void leds_off(void)
{
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    ESP_LOGI(TAG, "Turning LEDs off");
    printf("Turning LEDs off");
    memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config));
}

int leds_off_comm(int argc, char **argv)
{
    leds_off();
    return 0;
}