#pragma once
/**
 * ringbuf.h - Lock-free single-producer/single-consumer int16 ring buffer.
 *
 * Exactly one task may call ringbuf_push (producer) and exactly one task may
 * call ringbuf_pop (consumer); with that contract the buffer is safe across
 * cores without locks. Indices are free-running and rely on unsigned
 * wraparound, which is only correct when capacity is a power of two —
 * ringbuf_init enforces this and fails otherwise.
 *
 * A full buffer drops the INCOMING sample (drop-newest) and counts it in
 * `dropped`. Drop-oldest would require the producer to advance `tail`, which
 * races the consumer.
 *
 * C11 <stdatomic.h> — include from C translation units only.
 */
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int16_t      *buf;
    size_t        mask;    /* capacity - 1; capacity must be a power of two */
    atomic_size_t head;    /* producer-owned write index (free-running) */
    atomic_size_t tail;    /* consumer-owned read index (free-running) */
    atomic_size_t dropped; /* samples dropped because the buffer was full */
} ringbuf_t;

/** Returns false (and leaves the buffer unusable) unless capacity is a nonzero power of two. */
bool   ringbuf_init(ringbuf_t *rb, int16_t *storage, size_t capacity);
bool   ringbuf_push(ringbuf_t *rb, int16_t sample);   /* returns false if full; drops + increments dropped */
bool   ringbuf_pop(ringbuf_t *rb, int16_t *out);      /* returns false if empty */
size_t ringbuf_available(ringbuf_t *rb);              /* samples ready to read */
size_t ringbuf_dropped(ringbuf_t *rb);                /* total dropped count */
void   ringbuf_reset_dropped(ringbuf_t *rb);
