idf_component_register(SRCS "main.c" "system_manage.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        mqtt
                        esp_wifi
                        esp_event
                        nvs_flash 
                        esp_http_server
                    PRIV_REQUIRES
                        setup_wifi
                        mqtt_wifi
                        config
                        encrypt
                        drivers
                        control_led
                        control_relay
)
