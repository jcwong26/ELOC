idf_component_register(
			SRCS "pn532.c"
			INCLUDE_DIRS "include"
			REQUIRES "driver"
)
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_DEBUG")
