idf_component_register(
    SRCS "main.c" "status_model.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio esp_driver_spi esp_lcd
)
