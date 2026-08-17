# wifi_sta

Minimal Wi-Fi station bring-up: connect to the AP configured via menuconfig
(`WIFI_SSID` / `WIFI_PASSWORD`), block up to 30 s for an IP, auto-reconnect on
every disconnect thereafter.

```c
ESP_ERROR_CHECK(wifi_sta_init());   /* ESP_ERR_TIMEOUT if no IP in 30 s */
wifi_sta_is_connected();
```

The Kconfig entries live in this component (the hackathon tree referenced
`CONFIG_WIFI_SSID` without defining it anywhere, so a fresh clone couldn't
build — fixed here). Defaults are empty; set real credentials per venue via
menuconfig. `sdkconfig` is gitignored, so they never land in git.

NVS must be initialized before calling `wifi_sta_init()` (the reference app's
`main.c` does this).
