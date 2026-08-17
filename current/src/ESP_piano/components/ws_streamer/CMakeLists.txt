idf_component_register(
    SRCS "ws_stream.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES asv1_contracts esp_event esp_websocket_client
)
