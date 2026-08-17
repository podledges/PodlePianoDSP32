# asv1_contracts

C encoder for the **frozen** `audio_stream_v1` wire protocol: a packed 26-byte
little-endian header (magic `0xAD51`) followed by int16 PCM. Carries both `seq`
(loss/duplicate detection) and `sample_index` (drift detection) per frame.
See `docs/CONTRACTS.md` at the repo root for the field table.

Pure C, zero dependencies, compiles on host and target. A `_Static_assert`
guarantees the packed struct is exactly `ASV1_HEADER_SIZE` bytes.

## API

```c
size_t asv1_encode_audio(buf, buf_size, seq, sample_index,
                         timestamp_ms, sample_rate, pcm, pcm_len);
size_t asv1_encode_hello(buf, buf_size, seq, session_id); /* 42 bytes */
```

Both return bytes written, or 0 on invalid input / insufficient buffer.

## Rules

- FROZEN: do not change field order or meaning without updating the Python
  mirror (`src/contracts/audio_stream_v1.py`), the golden fixtures, and
  `docs/CONTRACTS.md` together.
- Little-endian only (ESP32-S3 and x86 both are; the encoder memcpys the
  packed struct).

## Known gap

There is no C↔Python golden-fixture parity test yet — the referenced
`golden_audio_frame.bin` was never committed (blocked by a blanket `*.bin`
gitignore). Write that test on day one in cleanslate.
