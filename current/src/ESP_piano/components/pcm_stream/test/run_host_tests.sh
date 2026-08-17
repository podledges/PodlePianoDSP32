#!/usr/bin/env sh
# Host-side unit tests: no ESP-IDF, no Unity — plain C11 + assert.
# Needs a C11 compiler with <stdatomic.h> (gcc or clang).
set -e
cd "$(dirname "$0")"
${CC:-cc} -std=c11 -Wall -Wextra -Werror \
    -I../include -I../../asv1_contracts/include \
    ../ringbuf.c ../pcm_framer.c ../../asv1_contracts/audio_stream_v1.c \
    test_framing.c -o test_framing.out
./test_framing.out
