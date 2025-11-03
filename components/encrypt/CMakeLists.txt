idf_component_register(
        SRCS "parse_json.c" "process_array.c" "encrypt_decrypt.c"
        INCLUDE_DIRS include
        REQUIRES
            driver
            freertos
            config
            mbedtls
            json

        PRIV_REQUIRES
            main  
            control_led  
            drivers
            mqtt_wifi
            ota_wifi
            control_relay

)
