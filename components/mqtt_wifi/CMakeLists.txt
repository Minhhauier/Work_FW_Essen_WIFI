file(GLOB source "*.c")

idf_component_register(
    SRCS ${source}
    INCLUDE_DIRS include
    REQUIRES
        config
        mqtt
        esp_wifi
        # espressif_mqtt
    PRIV_REQUIRES
        main
        encrypt
        setup_wifi
        drivers

)