idf_component_register(
    SRCS "main.c" "startup.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        pcm_stream capture_hal ws_streamer wifi_sta esp_hint
        nvs_flash esp_driver_gpio esp_timer
)
