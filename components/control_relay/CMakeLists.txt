idf_component_register(
        SRCS "control_relay.c"
        INCLUDE_DIRS include
        REQUIRES
            driver
            freertos
            config

        PRIV_REQUIRES    
            drivers
)
