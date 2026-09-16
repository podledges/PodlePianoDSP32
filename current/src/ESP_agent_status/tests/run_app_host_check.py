#!/usr/bin/env python3
"""Capture actual firmware frame bytes with host-only IDF boundary doubles.

Usage: python3 run_app_host_check.py /absolute/evidence/directory
Requires only Python's standard library and a C compiler. This is not a board
emulator or an ESP-IDF build; complete lines are supplied through host stdin.
"""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib


def png(path, pixels, width, height):
    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data)))
    scanlines = b"".join(b"\0" + pixels[y * width * 3:(y + 1) * width * 3]
                         for y in range(height))
    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(scanlines)) + chunk(b"IEND", b""))


def main():
    evidence = Path(sys.argv[1]).resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    tests = Path(__file__).resolve().parent
    headers = ["driver/gpio.h", "driver/spi_master.h", "esp_err.h", "esp_heap_caps.h",
               "esp_lcd_panel_io.h", "esp_lcd_panel_ops.h", "esp_lcd_panel_vendor.h",
               "esp_log.h", "freertos/FreeRTOS.h", "freertos/semphr.h", "freertos/task.h"]
    commands = ["STATUS working", "STATUS decision", " status NEEDS-DECISION ",
                "STATUS unknown", "STATUS working extra", "STATUS " + "x" * 80,
                "STATUS finished", "STATUS done", "sTaTuS working"]
    with tempfile.TemporaryDirectory(prefix="agent-status-host-") as directory:
        build = Path(directory)
        for header in headers:
            path = build / header
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('#include "host_idf.h"\n')
        binary = build / "app_host_test"
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-I" + str(build), "-I" + str(tests), str(tests / "app_host_test.c"),
                        str(tests / "../main/status_model.c"), "-o", str(binary)], check=True)
        result = subprocess.run([str(binary), str(build)], input="\n".join(commands) + "\n",
                                text=True, capture_output=True, check=True)
        assert result.stdout.count("Invalid command.") == 2
        assert result.stdout.count("Command too long.") == 1
        frames = []
        for index in range(4):
            data = (build / f"frame-{index}.ppm").read_bytes()
            assert data.startswith(b"P6\n170 320\n255\n")
            frames.append(data.split(b"\n", 3)[3])
        assert frames[0] == frames[3], "Returning to working must restore identical pixels"
        # Side-by-side, unscaled actual LCD payloads, not a reimplementation of the UI.
        pixels = b"".join(frame[y * 510:(y + 1) * 510]
                          for y in range(320) for frame in frames[:3])
        png(evidence / "status-frames.png", pixels, 510, 320)
        transcript = ("OFFLINE HOST HARNESS: unchanged app_main, mocked IDF/LCD boundaries.\n"
                      "No hardware or USB timing validation.\n\nINPUT LINES:\n"
                      + "\n".join(commands) + "\n\nAPP OUTPUT:\n" + result.stdout)
        (evidence / "command-transcript.txt").write_text(transcript)
        print(transcript)


if __name__ == "__main__":
    main()
