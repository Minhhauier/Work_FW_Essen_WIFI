file(GLOB SRCS "*.c")

idf_component_register(
        SRCS ${SRCS}
        INCLUDE_DIRS "."
        REQUIRES
            driver
            freertos
            config
            esp_timer
        # PRIV_REQUIRES
            control_relay
            setup_wifi
            esp_wifi
            
)

