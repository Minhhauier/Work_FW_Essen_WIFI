file(GLOB source "*.c")

idf_component_register(
    SRCS ${source}
    INCLUDE_DIRS include
    REQUIRES
        config
        esp_https_ota
        esp_http_client
        app_update
        #mqtt
        # espressif_mqtt
    PRIV_REQUIRES
        main
        #encrypt
)