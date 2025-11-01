file(GLOB SRCS "*.c")

idf_component_register(
        SRCS ${SRCS}
        INCLUDE_DIRS include
        REQUIRES
            driver
            drivers
            freertos
            config
            esp_timer
            
            mqtt_wifi
            control_led
            control_relay
)
