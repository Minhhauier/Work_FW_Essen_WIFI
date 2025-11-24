file(GLOB SRCS "*.c")

idf_component_register(
        SRCS ${SRCS}
        INCLUDE_DIRS "."
        REQUIRES
            driver
            freertos
            config
            esp_timer
            esp_wifi

            control_relay
            setup_wifi
        PRIV_REQUIRES
            control_led
)

