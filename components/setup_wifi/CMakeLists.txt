file(GLOB source "*.c")

idf_component_register(
    SRCS ${source}
    INCLUDE_DIRS include
    #REQUIRES
    PRIV_REQUIRES
        esp_wifi
        esp_event
        nvs_flash
        esp_wifi esp_event 
        nvs_flash 
        esp_http_server
        drivers
    
)