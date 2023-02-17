idf_component_register(SRCS "commands.c" "lv_controller.c"
                    INCLUDE_DIRS "../include"
                    REQUIRES "pn532" "driver" "console" "nvs_flash" "cmd_nvs" "cmd_system")
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_DEBUG")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
