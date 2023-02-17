#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#include "lv_controller.h"
#include "commands.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "pn532.h"
#include <driver/gpio.h>
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

#define SENDER_HOST SPI2_HOST
#define PIN_NUM_MISO 12
#define PIN_NUM_MOSI 7
#define PIN_NUM_CLK 6
#define PIN_NUM_CS 5

static const char *TAG = "main";

static void init_GPIO(void)
{
    // Set solenoid output
    gpio_set_direction(GPIO_NUM_38, GPIO_MODE_OUTPUT);

    // Set limit switch inputs

    // Set sled control outputs
    gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT); // EN
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT); // DI
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_OUTPUT); // PWM
    gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT); // DIR
}

static void init_nfc_reader(void)
{
    nfc_reader = pn532_init(0, 43, 44, 0); // passing UART0, tx=43, rx=44, 0 for output bits rn

    // 5 retries before giving up
    int count = 0;
    while (nfc_reader == NULL)
    {
        nfc_reader = pn532_init(0, 43, 44, 0);
        count++;
        if (count > 12)
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

static void register_commands(void)
{
    /* Lock Solenoid */
    esp_console_cmd_t lock_solenoid_cmd = {
        .command = "lock_solenoid",
        .help = "Sets solenoid bolt to locked state",
        .hint = NULL,
        .func = lock_solenoid,
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
        .func = unlock_solenoid,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&unlock_solenoid_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'unlock_solenoid' command\n");
    }

    /* Read NFC Tag */
    esp_console_cmd_t read_nfc_tag_cmd = {
        .command = "read_nfc_tag",
        .help = "Polls NFC reader to return a detected NFC tag",
        .hint = NULL,
        .func = read_single_nfc_tag,
        .argtable = NULL,
    };
    ret = esp_console_cmd_register(&read_nfc_tag_cmd);
    if (ret != ESP_OK)
    {
        printf("Error resgistering 'read_nfc_tag' command\n");
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
}

void app_main(void)
{
    ESP_LOGI(TAG, "LV Controller Starting");

    /* LV Controller Inits */
    init_GPIO();
    // init_nfc_reader();

    /* PWM Init */
    gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT);
    ledc_init();

    /* USB CONSOLE */
    initialize_nvs();

    initialize_console();

    /* Register commands */
    // register_commands();
    esp_console_register_help_command();
    register_system_common();
    register_system_sleep();
    register_nvs();
    register_commands();
    // register_wifi();

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

    //     /* NFC READER */

    //     // pn532_t *nfcreader = pn532_init(0, 43, 44, 0); // passing UART0, tx=43, rx=44, 0 for output bits rn
    //     // // pn532_t *nfcreader = pn532_init(1, 17, 18, 3); // passing UART0, tx=37, rx=36, 0 for output bits rn
    //     // int count = 0;
    //     // while (nfcreader == NULL)
    //     // {
    //     //     // nfcreader = pn532_init(1, 17, 18, 3);
    //     //     nfcreader = pn532_init(0, 43, 44, 0);
    //     //     count++;
    //     //     if (count > 12)
    //     //     {
    //     //         break;
    //     //     }
    //     // }
    //     // if (nfcreader != NULL)
    //     // {
    //     //     ESP_LOGI(TAG, "NFC Module Initialized");
    //     // }

    //     // usleep(100000);
    //     // while (true)
    //     // {
    //     //     uint8_t uid[100] = {};
    //     //     uint8_t uidLength;
    //     //     int res = pn532_Cards_and_return_data(nfcreader, &uid[0], &uidLength);

    //     //     ESP_LOGI(TAG, "Doing first check for tags");
    //     //     while (res <= 0)
    //     //     {
    //     //         res = pn532_Cards_and_return_data(nfcreader, &uid[0], &uidLength);
    //     //         usleep(2000000);
    //     //         // still checking
    //     //         ESP_LOGI(TAG, "still checking for tags");
    //     //     }
    //     //     // ESP_LOGI("main", "res: %d", res);
    //     //     // char text[21] = {};
    //     //     // uint8_t *nfcId = pn532_nfcid(nfcreader, text);
    //     //     // ESP_LOGI("main", "nfcId: %d", *nfcId);
    //     //     // ESP_LOGI("main", "uidLength: %d", uidLength);

    //     //     char uid_str[16 * 4];
    //     //     int index = 0;
    //     //     for (uint8_t i = 0; i < uidLength; i++)
    //     //     {
    //     //         index += sprintf(&uid_str[index], "%d ", uid[i]);
    //     //     }
    //     //     ESP_LOGI(TAG, "Detected NFC Tag with uid: %s", uid_str);
    //     // }

    //     // for (uint8_t i = 0; i < uidLength; i++)
    //     // {
    //     //     ESP_LOGI("main", "uid[%d]: %d", i, uid[i]);
    //     // }

    //     // uint8_t buf[64] = {};
    //     // uint8_t uid[64] = {};
    //     // uint8_t uidLength;
    //     // count = 0;
    //     // bool success = readPassiveTargetID2(nfcreader, &uid[0], &uidLength);
    //     // while (!success)
    //     // {
    //     //     success = readPassiveTargetID2(nfcreader, &uid[0], &uidLength);
    //     //     // ESP_LOGI("main", "count: %d", count);
    //     //     count++;
    //     // }

    //     // ESP_LOGI("main", "uidLength: %d", uidLength);
    //     // // char text[21] = {};
    //     // // uint8_t *nfcId = pn532_nfcid(nfcreader, text);
    //     // // ESP_LOGI("main", "nfcId: %d", *nfcId);
}
