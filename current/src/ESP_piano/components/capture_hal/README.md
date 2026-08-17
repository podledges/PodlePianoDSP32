# capture_hal

Swappable audio-capture interface plus the ADC-DMA implementation used for the
piezo pickup. Consumers call three functions (`init/read/deinit`) through
`capture_source_t` and never learn what hardware is behind them — that's the
seam for swapping in an I2S codec (stub included) or an envelope detector
without touching the streaming core.

`read()` returns DC-removed, zero-centered, signed int16 mono.

## capture_adc

- ADC1 continuous (DMA) driver, reconfigures transparently if the requested
  frame size changes.
- Per-frame mean-subtract DC removal, then scale/clamp to int16.
- The DMA read buffer is allocated once per configuration (the hackathon
  version malloc'd/free'd on every 4 ms read).

Channel, attenuation, and sample rate come from Kconfig (menuconfig →
"Capture HAL"). `CAPTURE_SAMPLE_RATE_HZ` in `capture_source.h` exposes the
configured rate so app code and the framer share one source of truth.

## Analog front-end preconditions (hardware, not firmware)

The ADC expects a conditioned piezo signal:

- Piezo output biased to mid-rail (~1.65 V) before the ADC pin
  (KiCad draft: 1 MΩ divider + op-amp buffer, TL072/MCP6002).
- Clamped into ADC range — 0–3.1 V at the default 12 dB attenuation
  (Schottky clamp, 1N5817).
- RC low-pass anti-alias, cutoff < half the sample rate (< 8 kHz @ 16 kHz).

Field bring-up and placement procedure: `docs/PHYSICAL_TEST_GUIDE.md`.

## Forbidden pins (ESP32-S3)

Strapping GPIO0/3/45/46 · USB GPIO19/20 · UART0 GPIO43/44 ·
flash/PSRAM GPIO26–37.
