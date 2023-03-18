#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "pn532.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_system.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_vfs_cdcacm.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "cmd_nvs.h"
#include "cmd_system.h"

#include "lv_controller.h"
#include "commands.h"
#include "nfc_module.h"
#include "ring_light.h"
#include "limit_switches.h"
#include "state_machine.h"

/* MACROS */
#define SENDER_HOST SPI2_HOST
#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 7
#define PIN_NUM_CLK 6
#define PIN_NUM_CS 5

/* GLOBALS */
static const char *TAG = "main";
TaskHandle_t nfc_module_task_handle = NULL;
TaskHandle_t state_machine_task_handle = NULL;

/* INITS */
static void init_GPIO(void)
{
    // Set solenoid output
    gpio_set_direction(GPIO_NUM_38, GPIO_MODE_OUTPUT);

    // Set limit switch inputs
    gpio_set_direction(LIM1_GPIO, GPIO_MODE_INPUT); // LIM1
    gpio_pulldown_dis(LIM1_GPIO);
    gpio_pullup_en(LIM1_GPIO);
    gpio_set_intr_type(LIM1_GPIO, GPIO_INTR_NEGEDGE);

    gpio_set_direction(LIM2_GPIO, GPIO_MODE_INPUT); // LIM2
    gpio_pulldown_dis(LIM2_GPIO);
    gpio_pullup_en(LIM2_GPIO);
    gpio_set_intr_type(LIM2_GPIO, GPIO_INTR_NEGEDGE);

    gpio_set_direction(LIM3_GPIO, GPIO_MODE_INPUT); // LIM3
    gpio_pulldown_dis(LIM3_GPIO);
    gpio_pullup_en(LIM3_GPIO);
    gpio_set_intr_type(LIM3_GPIO, GPIO_INTR_NEGEDGE);

    gpio_set_direction(LIM4_GPIO, GPIO_MODE_INPUT); // LIM4
    gpio_pulldown_dis(LIM4_GPIO);
    gpio_pullup_en(LIM4_GPIO);
    gpio_set_intr_type(LIM4_GPIO, GPIO_INTR_NEGEDGE);

    // Set sled control outputs
    gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT); // EN
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT); // DI
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_OUTPUT); // PWM
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT); // DIR
}

static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 1000, // Set output frequency at 5 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_GPIO,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void initialize_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void initialize_console(void)
{
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_cdcacm_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_cdcacm_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Enable non-blocking mode on stdin and stdout */
    fcntl(fileno(stdout), F_SETFL, 0);
    fcntl(fileno(stdin), F_SETFL, 0);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = 8,
        .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *)&esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(10);
}

/* REGISTER COMMANDS */
static void register_commands(void)
{
    /* Lock Solenoid */
    esp_console_cmd_t lock_solenoid_cmd = {
        .command = "lock_solenoid",
        .help = "Sets solenoid bolt to locked state",
        .hint = NULL,
        .func = lock_solenoid_comm,
        .argtable = NULL,
    };
    esp_err_t ret = esp_console_cmd_register(&lock_solenoid_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'lock_solenoid' command\n");
    }

    /* Unlock Solenoid */
    esp_console_cmd_t unlock_solenoid_cmd = {
        .command = "unlock_solenoid",
        .help = "Sets solenoid bolt to unlocked state",
        .hint = NULL,
        .func = unlock_solenoid_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&unlock_solenoid_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'unlock_solenoid' command\n");
    }

    /* Set Motor PWM */
    struct arg_int *duty;
    struct arg_end *end_pwm;
    void *set_pwm_argtable[] = {
        duty = arg_intn(NULL, NULL, "<duty>", 1, 2, "duty-cycle"),
        end_pwm = arg_end(10),
    };

    esp_console_cmd_t set_pwn_cmd = {
        .command = "set_pwm",
        .help = "Sets the duty cycle for PWM for the sled motor",
        .hint = NULL,
        .func = set_pwm,
        .argtable = set_pwm_argtable,
    };
    ret = esp_console_cmd_register(&set_pwn_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'set_pwn' command\n");
    }

    /* Set Motor DIR */
    struct arg_int *dir;
    struct arg_end *end_dir;
    void *set_dir_argtable[] = {
        dir = arg_intn(NULL, NULL, "<dir>", 1, 1, "direction of motor"),
        end_dir = arg_end(10),
    };

    esp_console_cmd_t set_mtr_dir_cmd = {
        .command = "set_mtr_dir",
        .help = "Sets the sled motor direction",
        .hint = NULL,
        .func = set_mtr_dir,
        .argtable = set_dir_argtable,
    };
    ret = esp_console_cmd_register(&set_mtr_dir_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'set_mtr_dir' command\n");
    }

    /* Enable Motor Output */
    esp_console_cmd_t en_mtr_output_cmd = {
        .command = "en_mtr_output",
        .help = "Enables the motor output",
        .hint = NULL,
        .func = enable_motor_output,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&en_mtr_output_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'en_mtr_output' command\n");
    }

    /* Disable Motor Output */
    esp_console_cmd_t dis_mtr_output_cmd = {
        .command = "dis_mtr_output",
        .help = "Disables the motor output",
        .hint = NULL,
        .func = disable_motor_output,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&dis_mtr_output_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'dis_mtr_output' command\n");
    }

    /* Start Rainbow Chase on Ring Light */
    esp_console_cmd_t rainbow_chase_start_cmd = {
        .command = "rainbow_chase_start",
        .help = "Start rRainbow chase on ring light",
        .hint = NULL,
        .func = rainbow_chase_start_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&rainbow_chase_start_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'rainbow_chase_start' command\n");
    }

    /* Stop Rainbow Chase on Ring Light */
    esp_console_cmd_t rainbow_chase_stop_cmd = {
        .command = "rainbow_chase_stop",
        .help = "Stop rainbow chase on ring light",
        .hint = NULL,
        .func = rainbow_chase_stop_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&rainbow_chase_stop_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'rainbow_chase_stop' command\n");
    }

    /* Set white LEDs on Ring Light */
    esp_console_cmd_t white_leds_cmd = {
        .command = "white_leds",
        .help = NULL,
        .hint = NULL,
        .func = set_white_leds,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&white_leds_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'white_leds' command\n");
    }

    /* Turn LEDs off on Ring Light */
    esp_console_cmd_t leds_off_cmd = {
        .command = "leds_off",
        .help = NULL,
        .hint = NULL,
        .func = leds_off_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&leds_off_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'leds_off' command\n");
    }

    /* sled_out */
    esp_console_cmd_t sled_out_cmd = {
        .command = "sled_out",
        .help = NULL,
        .hint = NULL,
        .func = sled_out_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&sled_out_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'sled_out' state command\n");
    }

    /* sled_in */
    esp_console_cmd_t sled_in_cmd = {
        .command = "sled_in",
        .help = NULL,
        .hint = NULL,
        .func = sled_in_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&sled_in_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'sled_in' state command\n");
    }

    /* stop_sled */
    esp_console_cmd_t stop_sled_cmd = {
        .command = "stop_sled",
        .help = NULL,
        .hint = NULL,
        .func = stop_sled_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&stop_sled_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'stop_sled' state command\n");
    }

    /* ------------------STATE COMMANDS------------------ */
    /* to_loading */
    esp_console_cmd_t to_loading_cmd = {
        .command = "to_loading",
        .help = NULL,
        .hint = NULL,
        .func = to_loading_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&to_loading_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'to_loading' state command\n");
    }

    /* to_closed */
    esp_console_cmd_t to_closed_cmd = {
        .command = "to_closed",
        .help = NULL,
        .hint = NULL,
        .func = to_closed_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&to_closed_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'to_closed' state command\n");
    }

    /* to_compvision */
    esp_console_cmd_t to_compvision_cmd = {
        .command = "to_compvision",
        .help = NULL,
        .hint = NULL,
        .func = to_compvision_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&to_compvision_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'to_compvision' state command\n");
    }

    /* to_charging */
    esp_console_cmd_t to_charging_cmd = {
        .command = "to_charging",
        .help = NULL,
        .hint = NULL,
        .func = to_charging_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&to_charging_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'to_charging' state command\n");
    }

    /* to_unlocked */
    esp_console_cmd_t to_unlocked_cmd = {
        .command = "to_unlocked",
        .help = NULL,
        .hint = NULL,
        .func = to_unlocked_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&to_unlocked_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'to_unlocked' state command\n");
    }

    /* to_unloading */
    esp_console_cmd_t to_unloading_cmd = {
        .command = "to_unloading",
        .help = NULL,
        .hint = NULL,
        .func = to_unloading_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&to_unloading_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'to_unloading' state command\n");
    }

    /* to_empty */
    esp_console_cmd_t to_empty_cmd = {
        .command = "to_empty",
        .help = NULL,
        .hint = NULL,
        .func = to_empty_comm,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&to_empty_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'to_empty' state command\n");
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "LV Controller Starting");

    /* GPIO Init */
    init_GPIO();

    /* Ring Light Init */
    init_ring_light();

    /* PWM Init */
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT); // for debug PWM LED only
    ledc_init();

    /* USB CONSOLE */
    initialize_nvs();
    initialize_console();

    /* Register commands */
    esp_console_register_help_command();
    register_system_common();
    register_system_sleep();
    register_nvs();
    register_commands();

    /* L9958 Motor Driver SPI bus setup */ // TODO
    esp_err_t err;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1};

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char *prompt = LOG_COLOR_I CONFIG_IDF_TARGET "> " LOG_RESET_COLOR;

    printf("\n ==============================================================\n");
    printf(" |                                                            |\n");
    printf(" |                ELOC LV CONTROLLER INTERFACE                |\n");
    printf(" |                                                            |\n");
    printf(" ==============================================================\n\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status)
    { /* zero indicates success */
        // printf("\n"
        //        "Your terminal application does not support escape sequences.\n"
        //        "Line editing and history features are disabled.\n"
        //        "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = CONFIG_IDF_TARGET "> ";
#endif // CONFIG_LOG_COLORS
    }

    /* Turn LEDs off on startup */
    leds_off_comm(0, NULL);

    /* NFC Module Task */
    xTaskCreate(read_single_nfc_tag, "read_single_nfc_tag", 4096, NULL, 10, &nfc_module_task_handle);

    /* Limit Switches Task */
    init_limit_switches();

    /* State Machine Task */
    // xTaskCreate(state_machine, "state_machine", 4096, NULL, 10, &state_machine_task_handle);

    /* Main loop */
    while (true)
    {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char *line = linenoise(prompt);
        if (line == NULL)
        { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);

        /* Try to run the command */
        int ret;
        err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
        {
            printf("Unrecognized command\n");
        }
        else if (err == ESP_ERR_INVALID_ARG)
        {
            // command was empty
        }
        else if (err == ESP_OK && ret != ESP_OK)
        {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        }
        else if (err != ESP_OK)
        {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

    printf(TAG, "Error or end-of-input, terminating console\n");
    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
}
