# Salvage Map — what to take into the cleanslate project

> **2026-08-02 update — firmware modularization DONE.** The four planned
> ESP-IDF packages (plus `wifi_sta` and `esp_hint`) now exist as
> self-contained components under `src/ESP_piano/components/`, with the
> day-one firmware fixes applied (ringbuf C11 atomics + pow2 enforcement,
> Wi-Fi Kconfig entries, header `_Static_assert`, persistent ADC DMA buffer,
> ws_streamer decoupled from the app via a status callback). Old
> `main/core|hal` paths referenced below are historical; see
> `src/ESP_piano/components/README.md` for the import guide.
> Python/JS packaging is still to-do.

Full-repo audit (2026-08-02). Verdicts: **STEAL** = lift nearly as-is,
**MAYBE** = take with fixes, **SKIP** = rewrite or drop.
Measured state: firmware host tests pass by design (plain `assert`, no Unity);
JS milestone sims pass (`node src/final_bar_matching/simulator_milestone4.js`);
Python suite is 35 pass / 1 fail (missing golden `.bin` fixture — see Contracts).

---

## 1. ESP32 firmware (`src/ESP_piano/main`) — the crown jewel

The v2 firmware is a textbook capture pipeline and the architecture itself is
the most valuable thing in the repo:

```
capture_task (core 1)          stream_task (core 0)           ws_tx_task (core 0)
ADC DMA -> DC-remove -> int16 ─-> ringbuf ─-> pcm_framer ────-> bounded queue ─-> WebSocket
              └-> esp_hint (optional on-board FFT hint, Kconfig-gated)
```

**STEAL — package as four ESP-IDF components:**

| Component | Files | Why it's worth taking |
|---|---|---|
| `asv1-contracts` | `src/contracts/audio_stream_v1.c`, `src/contracts/include/audio_stream_v1.h` | Frozen 26-byte binary frame encoder (see `docs/CONTRACTS.md`). Carries both `seq` and `sample_index` so the server can distinguish packet loss from clock drift. |
| `pcm-stream` (pure C, host-testable) | `core/ringbuf.{c,h}`, `core/pcm_framer.{c,h}`, `core/test_framing.c` | Zero ESP-IDF includes — compiles and tests on the host with plain `assert` (`test_framing.c` covers overflow, index wrap, seq wrap, timestamps). Framer emits 320-sample (20 ms) frames with timestamps derived from `sample_index`, not wall clock, so timing is sample-locked. |
| `capture-hal` | `hal/capture_source.h`, `hal/capture_adc.{c,h}`, `hal/capture_i2s.{c,h}` | Tiny vtable interface (`init/read/deinit`) so ADC can be swapped for an I2S codec without touching the core. `capture_adc.c` does continuous-DMA reads with per-frame mean-subtract DC removal and clamped int16 scaling, and transparently reconfigures when the frame size changes. |
| `ws-streamer` | `hal/ws_stream.c,h`, `hal/wifi_sta.{c,h}` | Dedicated TX task + bounded 8-frame queue isolates the audio path from network stalls (full queue = drop frame + count it, never block capture). Metrics (`sent/reconnects/dropped`) behind a portMUX spinlock; fresh random session id + HELLO on every reconnect. |

**Also take (app-level, not a package):**

- `main/board_pins.h` — the best-documented file in the repo: S3 pin map with
  the forbidden-pin list (strapping / USB / octal-PSRAM pins), analog
  front-end preconditions, and reserved pins for the future envelope detector
  and I2S codec.
- `main/main.cpp` — as the reference wiring of the four components
  (core-pinning, task priorities, 10 s metrics loop).
- `core/esp_hint.{c,h}` — optional on-board FFT hint channel (esp-dsp radix-2,
  Hann window, peak-pick, piano-range gate 27.5–4186 Hz, MIDI 21–108). The
  authoritative/non-authoritative split (server is truth, hint is instant
  feedback) is a keeper design idea even if you rewrite the code.

**Known defects to fix during extraction:**

1. `ringbuf.c` is used cross-core (push on core 1, pop on core 0) with no
   atomics/barriers. It survives because indices are free-running and capacity
   4096 is a power of two, but the module accepts arbitrary capacities —
   free-running index wraparound silently corrupts non-power-of-two sizes, and
   the overflow path mutates `tail` from the producer side (racy with a
   concurrent pop). Make it SPSC-safe (C11 atomics) and enforce pow2 capacity.
2. `capture_adc.c` mallocs/frees the raw DMA buffer on every read — hoist into
   the source struct.
3. `wifi_sta.c` consumes `CONFIG_WIFI_SSID` / `CONFIG_WIFI_PASSWORD` but no
   tracked Kconfig defines them (`Kconfig.projbuild` only has `SERVER_URI` and
   `ESP_HINT_ENABLED`) — a fresh clone doesn't build until you add those
   entries. No credentials are committed, which is correct; add the Kconfig
   entries in the new repo.
4. `audio_stream_v1.h` has no `_Static_assert(sizeof(asv1_header_t) == ASV1_HEADER_SIZE)`
   — add it; the Python side has the equivalent tripwire, the C side doesn't.
5. `esp_hint.c` re-inits `dsps_fft2r` tables on every `esp_hint_init` call and
   uses a fixed magnitude threshold (200.0) — fine for a hint, not for truth.

**Legacy firmware (`src/ESP_piano/legacy`) — mine for constants, then drop.**
`audio_dsp.c` holds the v1 on-device pitch detector: EMA DC-bias tracking
(`bias = 0.999*bias + 0.001*sample`), 200 ms peak-hold window, SNR gate
(peak/average >= 5), 3-frame note debounce, and a commented-out Harmonic
Product Spectrum (5 harmonics) implementation. If you ever bring pitch
detection back on-device, these tuned thresholds are the hard-won part.

**`src/ESP_lights` — SKIP** (boilerplate ESP-NOW receiver that blinks a GPIO
through a transistor). The only reusable bit is the pattern: ISR-safe
`vTaskNotifyGiveFromISR` from the ESP-NOW receive callback into a blink task,
and the 12-byte `PianoDataPacket` broadcast struct. Ten minutes to rewrite.

---

## 2. Circuit / hardware knowledge

- **KiCad (`KiCad/firstTime`)** — draft piezo analog front-end: piezo →
  TL072 (or MCP6002 at 3.3 V single-supply) op-amp stage, 1N5817 Schottky
  clamp pair, 1 MΩ bias resistors, 100/150 nF caps, 330 Ω series. Drawn for
  a classic-ESP32 Feather; `board_pins.h` explicitly supersedes it for the
  S3. Keep the schematic as the AFE reference, ignore the PCB.
- **`board_pins.h` header comment** — the AFE contract in three lines:
  mid-rail bias ~1.65 V, clamp/attenuate into 0–3.1 V (12 dB atten), RC
  anti-alias cutoff < 8 kHz for the 16 kHz sample rate.
- **`docs/PHYSICAL_TEST_GUIDE.md`** — keep whole: piezo placement/attachment
  on the soundboard, tabletop signal-quality smoke test (RMS sanity
  thresholds), field-test script T1–T9, and a failure-recovery section
  (no-signal, wrong pitches, high latency) that encodes real bring-up
  experience.
- **Future-hardware ideas already reserved in firmware:** hardware envelope
  detector on a second ADC channel (GPIO2), I2S codec swap-in
  (INMP441/ES8388/PCM1808 on GPIO4/5/6) — both land behind the
  `capture_source` seam with zero core changes.

---

## 3. Protocol contracts (`src/contracts`) — STEAL

C encoder + Python codec + JSON schema for `audio_stream_v1` (26-byte header:
magic/version/type/seq/sample_index/timestamp_ms/rate/channels/pcm_len) and
`note_events_v1`. Python decoder fails closed on every malformed-input class
and has an import-time `struct.calcsize` tripwire.

**Caveat — the C↔Python parity is asserted, not tested.** No C code runs in
any test; the golden `.bin` fixture was never committed (root `.gitignore`
has a blanket `*.bin`), which is the one Python test failure. Day one in the
new repo: commit the fixture under a different extension, add a host-side
test that encodes with the C encoder and decodes with Python, and add the
missing `_Static_assert`. Also: `TYPE_GOODBYE` is defined but has no encoder
on either side.

Package: pip `audio-contracts` with the C files shipped as package data.

---

## 4. Score following (`src/final_bar_matching`) — STEAL, highest software value

`full_score_progress_tracker.js` is a real online score-follower: ranked
candidate matching (next → bar restart → page restart → bounded backward
recovery), idempotent boundary events (`BAR_COMPLETED` / `PAGE_END_REACHED` /
`SCORE_COMPLETED` fire exactly once even across recovery rewinds), page-turn
suppression on the final page. Supporting cast worth taking:
`score_normalizer.js` (accepts five score shapes, pathed warnings),
`score_preprocessor.js` (chord grouping against a fixed anchor time — avoids
tolerance-window drift), `chord_compare.js` (Jaccard), `frequency_event_adapter.js`
(keeps higher-magnitude peak on MIDI collision), `frequency_to_midi.js`,
`frequency_score_pipeline.js` (the wiring). The four
`simulator_milestone*.js` files are the actual test suite (plain
`node:assert`, cascading, all passing) — port them to `node:test`.

**Fix before trusting (found in audit):**
- Two consecutive unmatched notes reset the whole score
  (`DEFAULT_MAX_CONSECUTIVE_UNRELATED = 1` + full `resetProgress()`), and any
  5 s silence does the same — deadly with a real transcriber. Make both
  bounded re-anchors with saner defaults.
- No forward-skip candidates: a missed note stalls the tracker permanently.
- Null-guard bug: `findCurrentBarStartIndex` crashes on scores mixing
  bar-less and bar-based pages (its sibling `findBarEndIndex` guards
  correctly).
- `score_normalizer.js` can mutate caller input via `rawNotes.push` aliasing.
- Jaccard punishes extra observed notes (pedal ring, harmonics — the
  transcriber's most common failure) as hard as missing ones; consider
  recall-biased scoring. No tempo/time handling exists anywhere.
- `final_bar_matcher.js` is a superseded predecessor — drop it. The
  hand-rolled browser bundler (`src/scripts/build_final_bar_matching_browser_bundle.js`)
  is competent but is 171 lines that `esbuild --bundle` replaces.

Package: npm `score-follower`.

---

## 5. Server-side Python (`src/server`, `src/tools`) — STEAL the core, rewrite the shell

- **`ingest.py`** — best Python file in the repo: tracks dups/gaps/resyncs
  and cumulative sample-index drift independently, handles uint32 seq
  wraparound (tested), treats `seq==0` mid-stream as a device reboot →
  resync, and invokes the PCM callback outside its lock so a slow consumer
  can't block the socket reader. Lift as-is → pip `audio-ingest`.
- **`transcriber/base.py` + `fake.py`** — clean 4-method ABC
  (`start/feed/poll/stop`; poll-and-clear, no asyncio coupling) plus a
  deterministic RMS-threshold fake that makes the whole pipeline CI-testable
  with no ML model. `basic_pitch.py` is MAYBE: sliding 2 s window with 1 s
  overlap **double-emits every note in the overlap** (no dedup), writes a
  temp WAV per inference window, and accumulates onset rounding drift — but
  `_resolve_model_path()` (ONNX → TFLite → TF fallback) encodes a real
  compatibility landmine worth keeping.
- **`src/tools`** — the closed measurement loop almost nobody builds:
  `generate_fixtures.py` → `stream_synthetic.py` (wall-clock-rate replay) →
  `check_golden.py` (mir_eval F1 vs golden MIDI, exits non-zero under 0.50 —
  CI-ready accuracy gate) → `latency_harness.py` (wire-to-wire p50/p95 for
  the specific frame containing a known onset). Dedup the three copies of
  `read_wav`/`iter_audio_frames` into one module. → pip `stream-harness`.
- **`app.py` — SKIP/rewrite:** runs model inference synchronously on the
  asyncio event loop (freezes every endpoint), decodes every frame twice,
  has a dead lock in the Node bridge and unbounded reconnect recursion.
  `index.js` (the second, Node server) overlaps it — pick one language in
  the new repo. `broadcast.py` (bounded per-client queues, drop-don't-stall)
  is a keeper snippet.

---

## 6. Frontends (`src/web`, `src/web_app`, `src/mobile`) — SKIP; strip three snippets

1. Reconnecting WebSocket client (`src/server/static/index.html:766-808`) —
   capped exponential backoff, re-entrancy-guarded timer, sync-throw-safe
   `new WebSocket()`. The only client in the repo with reconnect logic.
2. Waterfall / piano-roll canvas renderer (`src/web_app/app.js:927-971`) —
   61 lanes, viewport culling, clocked off `audioCtx.currentTime` so it
   stays sample-locked.
3. Gemini OMR prompt + JSON response schema
   (`src/web_app/app.js`, `callGeminiTranscriptionForCurrentPage`) — extract
   the prompt text, discard the file.

`src/web` is a marketing landing page with hardcoded fake logs; `src/mobile`
is a 1,133-line single-component Expo app with no reconnect logic. Take
nothing else.

---

## 7. Repo hygiene notes

- `src/server/node_modules` (~900 files) is committed — don't carry that
  habit forward.
- `esp32_technical_reference_manual_en.pdf` (10 MB) is Espressif's public
  manual — link it, don't commit it, in the new repo.
- Root `.gitignore`'s blanket `*.bin` is what ate the golden fixture.
- `run_app.py` is a trivial static-file launcher that collides with the
  server's default port — drop.

## Suggested package split for cleanslate

| Package | Source | Effort |
|---|---|---|
| ESP-IDF components: `asv1-contracts`, `pcm-stream`, `capture-hal`, `ws-streamer` | `src/ESP_piano/main` + `src/contracts` C side | Low; fix ringbuf atomics + add Kconfig entries |
| pip `audio-contracts` | `src/contracts` | Low; write the real C↔Python parity test |
| pip `audio-ingest` | `src/server/ingest.py` (+tests) | Trivial |
| pip `transcriber` | `transcriber/` (+tests), basic-pitch as optional extra | Low; fix overlap double-emit |
| pip `stream-harness` | `src/tools` | Low; dedup WAV I/O |
| npm `score-follower` | `src/final_bar_matching` core + milestone sims as tests | Medium; fix reset/skip/null-guard/mutation first |
| snippets | reconnect WS client, waterfall renderer, Gemini OMR prompt | Trivial |
