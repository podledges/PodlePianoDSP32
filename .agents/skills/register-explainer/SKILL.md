---
name: register-explainer
description: >
  Register explainer. Use when asked to show, draw, or decode an MCU/ESP32
  register or bit field; configure a register; or trace a peripheral clock.
---

# Register explainer

Emit a **card** (default) or a **walkthrough** (one checkpoint, then one question) for a **locked** target. Every hardware-facing claim is **cited**.

## 1. Target lock

Done when all four are named from evidence:

1. Chip variant (`esp32`, `esp32s3`, …)
2. SoC revision, or a cite that this claim is rev-agnostic
3. ESP-IDF version, or the exact `*_reg.h` / `*_struct.h` path including revision
4. Peripheral

Search in order; the first sufficient cite wins:

1. Project-owned — `sdkconfig`, `sdkconfig.defaults`, CMakeCache `IDF_TARGET`, `docs/TOOLCHAIN.md`, board/Kconfig headers in the firmware you are actually building
2. ESP-IDF for that variant — `$IDF_PATH/components/soc/<target>/`, `*_reg.h`, `*_struct.h`, `*_periph.h`
3. The TRM whose title matches the locked variant — section and page

A classic *ESP32 Technical Reference Manual* covers ESP32. It is the wrong book for S3/C3/C6/P4. A header whose path is another `soc/<target>/` is the wrong book.

Incomplete lock → ask **one** question that fills the highest-leverage missing item (variant, then IDF, then revision), then wait. Sibling variants are not stand-ins.

## 2. Object class

Name the asked token as exactly one of: **register**, **field**, **signal**, **clock**, **alias**, **derived**.

| Class | Response |
|---|---|
| register / field | **card** |
| clock | **clock path**; a mux/divider **field** may also get a card |
| signal / alias / derived | classify, cite the related register if any, skip the card |

Done when the class is stated and the rest of the reply matches that row.

## 3. Card

Compact unless the user asked to reason it themselves. Fill every slot a cite supports; mark the rest `unverified` and ask.

```
Target: <variant> · rev <n or agnostic> · IDF <ver> · <peripheral>
Register: <full name>
Address: <absolute and/or peripheral offset>
Width / access / reset: <n-bit> / <RO|WO|R/W|R/W1C|…> / <value>
Cite: <TRM §… p.…> · <header::macro>

 31          24 23          16 15           8 7            0
┌──────────────┬──────────────┬─────────────┬──────────────┐
│              │              │             │              │
└──────────────┴──────────────┴─────────────┴──────────────┘
 <name or reserved [msb:lsb]> …every bit covered…

Fields
  NAME [msb:lsb]  access  reset  encoding (legal values only)

Requested value: unshifted <n> → shifted 0x… (bits …)
  before: 0b…
  after:  0b…   (only the named field changes)

RMW
  uint32_t v = REG_READ(REG);
  v = (v & ~FIELD_M) | ((unshifted << FIELD_S) & FIELD_M);
  REG_WRITE(REG, v);

Interactions
Clock path (clock questions only)
```

Cover every named field and every reserved range. Leave reserved bits at reset unless a cite says otherwise. Prefer the project's ESP-IDF macros when those headers are the cite.

### Masks

For each `_M` / `_V` / `_S` (or equivalent) trio, state all three:

- unshifted field value
- shifted register value (`value << shift`)
- before and after bit pattern of the full register

The C snippet is mask-based RMW so unrelated bits stay put.

## 4. Clock path

Trace left to right, units on every number:

**source → divider(s) → mux → peripheral → peripheral prescaler/count**

Name each mux by the field value that selects it. Show the arithmetic (Hz, kHz, MHz). A derived clock or pin is not a register — classify it, then keep tracing.

## 5. Walkthrough

When the user wants to do the reasoning: one checkpoint, then one question. Otherwise the compact card.

## Done

The lock is stated, or a single blocking question was asked. Object class is named. Every hardware-facing sentence has a cite or is marked `unverified`. The card, clock path, or question matches the request. Configuring uses RMW that preserves unrelated bits.
