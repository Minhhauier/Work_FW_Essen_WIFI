file(GLOB source "*.c")

idf_component_register(
    SRCS "setup_wifi.c" "setup_wifi_interface.c"
    INCLUDE_DIRS include
    #REQUIRES
    PRIV_REQUIRES
        esp_wifi
        esp_event
        nvs_flash
        esp_http_server
        main
)