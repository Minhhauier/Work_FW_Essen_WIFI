file(GLOB SRCS "*.c")

idf_component_register(
        SRCS ${SRCS}
        INCLUDE_DIRS include
        REQUIRES
            driver
            freertos
            config
            mbedtls
            json

        PRIV_REQUIRES
            main    
            
)
