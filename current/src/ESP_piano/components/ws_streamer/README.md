# ws_streamer

WebSocket transmitter that isolates the real-time audio path from network
stalls: `ws_stream_send_frame` copies the frame into a bounded 8-deep queue
and returns immediately — a dedicated TX task does the actual sends. When the
network is slow the queue fills and frames are **dropped and counted**, never
blocking capture.

- Sends an `audio_stream_v1` HELLO with a fresh random session id
  (`esp-%08x-%02x`) on every (re)connect, so the server can reset seq tracking.
- Auto-reconnects (1 s timeout) via esp_websocket_client.
- Metrics (`frames_sent / reconnects / dropped_frames`) behind a portMUX
  spinlock; read them with `ws_stream_get_metrics()`.
- Lazy client start: the connection is only opened once the first frame is
  queued.

Depends on `asv1_contracts` and the managed `espressif/esp_websocket_client`
(declared in `idf_component.yml` — ESP-IDF v6 removed it from core).

## Wiring

```c
ws_stream_config_t cfg = {
    .server_uri  = CONFIG_SERVER_URI,   /* supplied by the app */
    .sample_rate = 16000,
    .on_status   = my_status_cb,        /* optional: connect/disconnect */
};
ws_stream_init(&cfg);
ws_stream_send_frame(frame.data, frame.len);
```

The server URI is deliberately a config-struct parameter, not component
Kconfig — the app decides where config comes from. Connection status flows
out through the optional `on_status` callback (the hackathon version called
straight into the app's startup module; that coupling is gone).
