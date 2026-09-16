# ESP agent status display

`ESP_agent_status` is the expandable ESP-IDF home for showing PodlESP agent state on the captain's ESP32-S3-Touch-LCD-1.9-class board. It intentionally does not implement piezo sampling or a live Firstmate bridge yet.

The 170 x 320 display changes only when the status changes:

| Command value | Screen | Meaning |
| --- | --- | --- |
| `working` | calm blue, three-dot mark, `WORKING` | agent is working |
| `decision` or `needs-decision` | amber, alert mark, `DECISION` | captain must decide |
| `finished` or `done` | green, check mark, `DONE` | finished / idle-complete |

There is no animation or periodic repaint. The initial state is `working`.

## Build and offline check

Use an ESP-IDF 5.x shell from this folder:

```sh
idf.py set-target esp32s3
idf.py build
```

This repository does not currently provide a project-local ESP-IDF environment. PodlESP's Nix patterns may be reused as an environment reference, but this app remains owned here.

The parser has a dependency-free host check:

```sh
cd tests
./run_host_check.sh
```

This task does not authorize flashing, resetting, probing, or opening a hardware serial session.

## Setting status

After an operator with separate hardware authority has flashed the app, send one newline-terminated command over the ESP32-S3 USB Serial/JTAG console:

```text
STATUS working
STATUS decision
STATUS finished
```

Commands and values are case-insensitive. `needs-decision` and `done` are accepted aliases. Invalid lines do not change the screen.

`status_model.c` is deliberately independent of the display and transport. A later live PodlESP feed should translate its messages into `agent_status_t`, then call the same event-driven rendering path. Networking, Firstmate integration, SSH, websockets, and broker work are out of scope here.

## Hardware assumptions and evidence

Target: Waveshare `ESP32-S3-Touch-LCD-1.9` product class, SKU 30939. The assigned lab unit's exact manufacturer and hardware revision are still unconfirmed, so a successful build is not proof that these settings match that physical unit.

| Setting used here | Value | Evidence / qualification |
| --- | --- | --- |
| LCD panel | 170 x 320 SPI, ST7789V2-class | Waveshare's primary product documentation lists 170 x 320, SPI, and ST7789V2. The task also explicitly targets the ST7789-class unit. |
| ESP-IDF panel driver | built-in `esp_lcd_new_panel_st7789` | Software choice matching the product table, not a physical-unit observation. |
| Visible-column offset | x = 35 | Waveshare's primary Arduino ST7789 demo configures 170 x 320 with 35-pixel left/right offsets. |
| LCD reset | GPIO9 | Waveshare Arduino and ESP-IDF display demos at vendor commit `12b8fc74434103d0c5721906b69d866c789e4c41`. |
| LCD clock | GPIO10 | Same vendor demos. |
| LCD data/command | GPIO11 | Same vendor demos. |
| LCD chip select | GPIO12 | Same vendor demos. |
| LCD MOSI | GPIO13 | Same vendor demos. |
| Backlight | GPIO14, active low | Waveshare Arduino demo defines GPIO14 and drives it low after LCD initialization. |
| SPI host / clock | SPI3 at 20 MHz | Waveshare ESP-IDF display demo. |
| USB console | ESP32-S3 USB Serial/JTAG | Project build choice; it does not consume or claim an exposed board GPIO. |

Primary sources:

- [Waveshare ESP32-S3-LCD-1.9 product documentation](https://docs.waveshare.com/ESP32-S3-LCD-1.9)
- [Waveshare official example repository](https://github.com/waveshareteam/ESP32-S3-LCD-1.9/tree/12b8fc74434103d0c5721906b69d866c789e4c41)
- Sibling fleet note: `PodlESP/boards/esp32-s3-touch-lcd-1.9/README.md`

Important vendor discrepancy: at the cited commit, Waveshare's Arduino demo uses `Arduino_ST7789`, while its ESP-IDF LVGL demo uses a custom SH8601 panel path. Both use GPIO9 through GPIO14. This app follows the task's ST7789-class target and the product table. **TODO before any authorized flash:** identify the physical unit and revision, then confirm the controller against its matching schematic or known-good factory firmware. If it is the SH8601 revision, replace only the panel creation/init command table; do not guess a new pin map.

No piezo or ADC GPIO is selected here. The ESP32-S3 silicon ADC map does not establish which board header net is free, and LCD, touch, IMU, USB, or storage may already use candidate pins.

## Piezo expansion seam

Keep sampling separate from UI and transport:

1. Confirm the exact board revision and an exposed ADC1-capable net from the matching schematic.
2. Add a `piezo_capture` component for ADC continuous/I2S sampling and signal conditioning assumptions.
3. Send only reduced events or health state to the app task. Never repaint per sample; status rendering remains event-driven.
4. Add host checks for threshold/onset logic before any authorized hardware test.

Full piezo acquisition, protection circuitry, calibration, onset detection, and note analysis remain future work.
