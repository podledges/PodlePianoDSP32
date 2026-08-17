# esp_hint

Optional on-board "instant hint": a 1024-point Hann-windowed FFT (esp-dsp
radix-2) peak-pick that emits a `note_events_v1` JSON `note_on` with
`source:"esp_hint"` and `confidence:0.5` whenever a peak clears the threshold
inside the piano range (27.5–4186 Hz → MIDI 21–108).

**Non-authoritative by design** — the laptop transcriber is the source of
truth; this exists for low-latency local feedback (LEDs, debugging) only.

Disabled by default. Enable via menuconfig → "ESP Hint (on-board FFT)";
when disabled every function compiles to a no-op, so callers never need
`#ifdef`s. Costs ~1 ms of core-1 CPU per 1024-sample window when on.

```c
esp_hint_init(my_json_cb, ctx, 16000);
esp_hint_feed(samples, n);   /* call from the capture path */
```

Depends on the managed `espressif/esp-dsp` (declared in `idf_component.yml`).
Monophonic only — simultaneous notes yield the strongest peak.
