idf_component_register(
    SRCS "mqtt_wifi.c"
    INCLUDE_DIRS include
    REQUIRES
        config
        mqtt
        driver
        esp_wifi
        # espressif_mqtt
    PRIV_REQUIRES
        main
        encrypt
        drivers
        setup_wifi
)