idf_component_register(
    SRCS "capture_adc.c" "capture_i2s.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_adc
)
