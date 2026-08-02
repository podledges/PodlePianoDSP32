#pragma once
/**
 * board_pins.h — ESP32-S3 board map for the Podles DSP Piano reference app.
 *
 * The piezo ADC channel, attenuation, and sample rate are configured in the
 * capture_hal component (menuconfig → "Capture HAL"); the analog front-end
 * preconditions are documented in components/capture_hal/README.md.
 *
 * Forbidden pins (do NOT use for ADC/LED/UART/I2S):
 *   Strapping: GPIO0, GPIO3, GPIO45, GPIO46
 *   USB D-/D+: GPIO19, GPIO20
 *   Console UART0: GPIO43 (TX), GPIO44 (RX)
 *   Flash/PSRAM (octal modules): GPIO26-GPIO37
 */

/* ── Status LED ──────────────────────────────────────────────────────── */
/**
 * GPIO38 is safe on most ESP32-S3 devkits (no strapping / USB / flash conflict).
 * Some boards use GPIO48 for the RGB LED — override with -DBOARD_LED_GPIO=48
 * at compile time if needed.
 */
#ifndef BOARD_LED_GPIO
#  define BOARD_LED_GPIO  38
#endif

/* ── Future: Hardware envelope detector (DEFERRED — DO NOT IMPLEMENT NOW) ── */
/**
 * When the hardware envelope detector circuit is validated, wire it to a
 * second ADC1 channel (channel 1 = GPIO2, reserved) and add it as another
 * capture_hal source — the capture_source interface accepts it without
 * touching the streaming core.
 */

/* ── Future: External I2S audio codec (DEFERRED — DO NOT IMPLEMENT NOW) ── */
/**
 * If internal ADC SNR proves insufficient, swap in an I2S codec (e.g. INMP441,
 * ES8388, PCM1808) on these pins.  The capture_i2s stub uses these defines.
 */
#define BOARD_I2S_BCLK_GPIO   4
#define BOARD_I2S_WS_GPIO     5
#define BOARD_I2S_DIN_GPIO    6    /* data from codec to ESP */
