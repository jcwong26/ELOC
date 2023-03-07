idf_component_register(SRCS "motor.c" "state_machine.c" "limit_switches.c" "nfc_module.c" "ring_light.c" "commands.c" "lv_controller.c"
                    INCLUDE_DIRS "../include"
                    REQUIRES "pn532" "driver" "console" "nvs_flash" "cmd_nvs" "cmd_system" "led_strip")
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_DEBUG")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
