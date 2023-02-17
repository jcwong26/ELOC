idf_component_register(
			SRCS "led_strip.c"
			INCLUDE_DIRS "include"
			REQUIRES "driver"
)
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_DEBUG")
