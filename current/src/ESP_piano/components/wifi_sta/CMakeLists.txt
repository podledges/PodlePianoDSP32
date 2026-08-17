idf_component_register(
    SRCS "wifi_sta.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_wifi esp_event esp_netif
)
