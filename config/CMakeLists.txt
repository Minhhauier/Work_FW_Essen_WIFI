# file(GLOB source "*.c")

idf_component_register(
    # SRCS ${source}
    INCLUDE_DIRS "."
    REQUIRES
        
    # PRIV_REQUIRES
    #     main
    #     config
)