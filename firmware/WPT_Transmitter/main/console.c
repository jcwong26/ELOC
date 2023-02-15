#include "console.h"

#include "inverter.h"
#include "v_sense.h"

static int cmd_set_freq(int argc, char **argv) {
    if(argc == 2) {
        set_sw_freq(atof(argv[1])*1000);
    } else {
        printf("Invalid Usage. Usage: setFreq <freq_kHz>\n");
        // return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static int cmd_enable_bridge(int argc, char **argv) {
    enable_bridge();
    printf("Inverter Output Enabled\n");
    return ESP_OK;
}

static int cmd_disable_bridge(int argc, char **argv) {
    disable_bridge();
    printf("Inverter Output Disabled\n");
    return ESP_OK;
}

static int cmd_turn_fan_on(int argc, char **argv) {
    turn_fan_on();
    printf("Fan On\n");
    return ESP_OK;
}

static int cmd_turn_fan_off(int argc, char **argv) {
    turn_fan_off();
    printf("Fan Off\n");
    return ESP_OK;
}


void register_commands(void){
    // Set Switching frequency Command
    const esp_console_cmd_t set_freq_cmd = {
        .command = "setFreq",
        .help = "Sets Inverter Switching Frequency in kHz",
        .hint = "<freq_kHz>",
        .func = cmd_set_freq,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_freq_cmd));

    const esp_console_cmd_t enable_bridge_cmd = {
        .command = "enableBridge",
        .help = "Enabled the Inverter Output",
        .hint = NULL,
        .func = cmd_enable_bridge,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&enable_bridge_cmd));

    const esp_console_cmd_t disable_bridge_cmd = {
        .command = "disableBridge",
        .help = "Disable the Inverter Output",
        .hint = NULL,
        .func = cmd_disable_bridge,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&disable_bridge_cmd));

    const esp_console_cmd_t turn_fan_on_cmd = {
        .command = "turnFanOn",
        .help = "Turn Cooling Fan On",
        .hint = NULL,
        .func = cmd_turn_fan_on,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&turn_fan_on_cmd));

    const esp_console_cmd_t turn_fan_off_cmd = {
        .command = "turnFanOff",
        .help = "Turn Cooling Fan Off",
        .hint = NULL,
        .func = cmd_turn_fan_off,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&turn_fan_off_cmd));


}


void initialize_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void initialize_console(void)
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
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    register_commands();

    esp_console_register_help_command();


    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(10);

    printf("\n"
        "----------------------------------------------\n"
        "| ELOC Wireless Power Transmitter Interface. |\n"
        "| Type 'help' to get the list of commands.   |\n"
        "----------------------------------------------\n");
}

void run_console(void){
    const char* prompt = LOG_COLOR_I CONFIG_IDF_TARGET "> " LOG_RESET_COLOR;
    
    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
            // printf("\n"
            //     "Your terminal application does not support escape sequences.\n"
            //     "Line editing and history features are disabled.\n"
            //     "On Windows, try using Putty instead.\n");
            linenoiseSetDumbMode(1);
    #if CONFIG_LOG_COLORS
            /* Since the terminal doesn't support escape sequences,
            * don't use color codes in the prompt.
            */
            prompt = CONFIG_IDF_TARGET "> ";
    #endif //CONFIG_LOG_COLORS
        }


    /* Main loop */
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }
}