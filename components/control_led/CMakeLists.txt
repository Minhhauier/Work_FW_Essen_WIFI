idf_component_register(
        SRCS "control_led.c" "led_init.c"
        INCLUDE_DIRS include
        REQUIRES
            driver
            freertos
            config
            led_strip
            control_relay
)
