file(GLOB SRCS "*.c")

idf_component_register(
        SRCS ${SRCS}
        INCLUDE_DIRS include
        REQUIRES
            driver
            freertos
            config
            led_strip
            control_relay
)
