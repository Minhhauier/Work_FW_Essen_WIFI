file(GLOB source "*.c")

idf_component_register(
    SRCS ${source}
    INCLUDE_DIRS include 
    # EMBED_TXTFILES "htm_code.html"
    #REQUIRES
    PRIV_REQUIRES
        esp_wifi
        esp_event
        nvs_flash
        esp_http_server
        drivers

        mqtt_wifi
    
)
#message(STATUS "Source files: ${source}")