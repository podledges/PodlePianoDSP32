# Yoinkable ESP-IDF components

Each directory here is a self-contained ESP-IDF component. To use one in
another project (e.g. cleanslate), either:

1. **Copy the folder** into that project's `components/` directory, or
2. **Point at it in place**: `set(EXTRA_COMPONENT_DIRS "path/to/this/components")`
   in the project's top-level `CMakeLists.txt`.

Managed dependencies (esp-dsp, esp_websocket_client) are declared per-component
in `idf_component.yml`, so the component manager fetches them automatically
wherever the component lands.

## Dependency graph

```
asv1_contracts        (pure C, no deps — wire protocol encoder)
   ▲
pcm_stream            (pure C — SPSC ring buffer + 20ms framer; host-testable)
capture_hal           (ESP-IDF — swappable capture interface + ADC-DMA impl)
ws_streamer           (ESP-IDF — bounded-queue WebSocket TX, needs asv1_contracts)
wifi_sta              (ESP-IDF — STA connect w/ auto-reconnect; standalone)
esp_hint              (ESP-IDF — optional on-board FFT hint; standalone)
```

Only `pcm_stream → asv1_contracts` is a hard inter-component dependency.
Everything else composes through the app (see `../main/main.c` for the
reference wiring: capture task → ring buffer → framer → ws_streamer).

## Bring-your-own pieces

The seams where new code plugs in:

- **New audio source** (I2S codec, envelope detector): implement the 3-function
  vtable in `capture_hal/include/capture_source.h`. Nothing downstream changes.
- **New transport** (BLE, UDP, ESP-NOW): consume `framer_frame_t` from
  `pcm_stream` — frames are fully encoded bytes, transport-agnostic.
- **New connection supervisor / UI**: register `on_status` in
  `ws_stream_config_t`; the streamer has no idea what an app or LED is.
- **On-device DSP**: feed the same samples to `esp_hint`-style modules in the
  capture task; the streaming path doesn't care.

## Host tests

`pcm_stream/test/run_host_tests.sh` builds and runs the pure-C core tests with
plain gcc/clang — no ESP-IDF toolchain needed.
