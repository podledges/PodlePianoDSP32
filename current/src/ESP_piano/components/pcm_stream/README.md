# pcm_stream

The pure-C streaming core: a lock-free SPSC ring buffer plus a framer that
chunks samples into 320-sample (20 ms @ 16 kHz) `audio_stream_v1` frames.
Zero ESP-IDF includes — builds and tests on the host.

Depends on `asv1_contracts` (frame encoding).

## ringbuf

Single-producer/single-consumer int16 ring buffer using C11 atomics with
acquire/release ordering — safe across cores as long as exactly one task
pushes and one pops. Capacity **must be a power of two** (free-running indices
rely on unsigned wraparound); `ringbuf_init` returns false otherwise.
A full buffer drops the incoming sample and counts it (`ringbuf_dropped`).

Changes from the hackathon version (both were latent cross-core races):

- `volatile size_t` indices → C11 atomics with explicit memory ordering.
- Overflow used to advance `tail` from the producer (drop-oldest), racing the
  consumer's pop. Now drop-newest: only the consumer ever writes `tail`.
- Non-power-of-two capacities used to silently corrupt on index wraparound;
  now rejected at init.

`<stdatomic.h>` means the header is for C translation units; call it through
a C wrapper if you need it from C++.

## pcm_framer

Feed one sample at a time; every 320th sample it encodes a complete frame and
hands back a pointer valid until the next push. `timestamp_ms` is derived from
`sample_index`, **not** wall clock, so timing is sample-locked and immune to
scheduling jitter. `seq` wraps at `UINT32_MAX` (covered by a test);
`pcm_framer_reset` starts a new session.

## Tests

```sh
test/run_host_tests.sh
```

Plain assert, no framework: round-trip, overflow accounting, index wraparound,
pow2 rejection, frame boundaries, seq/sample_index/timestamp, seq wrap.
