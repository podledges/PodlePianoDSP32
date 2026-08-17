#include "ringbuf.h"

bool ringbuf_init(ringbuf_t *rb, int16_t *storage, size_t capacity)
{
    if (rb == NULL) {
        return false;
    }

    const bool valid = storage != NULL && capacity != 0 && (capacity & (capacity - 1)) == 0;

    rb->buf = valid ? storage : NULL;
    rb->mask = valid ? capacity - 1 : 0;
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
    atomic_init(&rb->dropped, 0);

    return valid;
}

bool ringbuf_push(ringbuf_t *rb, int16_t sample)
{
    if (rb == NULL || rb->buf == NULL) {
        if (rb != NULL) {
            atomic_fetch_add_explicit(&rb->dropped, 1, memory_order_relaxed);
        }
        return false;
    }

    const size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);

    if (head - tail > rb->mask) { /* full: capacity samples in flight */
        atomic_fetch_add_explicit(&rb->dropped, 1, memory_order_relaxed);
        return false;
    }

    rb->buf[head & rb->mask] = sample;
    atomic_store_explicit(&rb->head, head + 1, memory_order_release);

    return true;
}

bool ringbuf_pop(ringbuf_t *rb, int16_t *out)
{
    if (rb == NULL || out == NULL || rb->buf == NULL) {
        return false;
    }

    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    const size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);

    if (head == tail) {
        return false;
    }

    *out = rb->buf[tail & rb->mask];
    atomic_store_explicit(&rb->tail, tail + 1, memory_order_release);

    return true;
}

size_t ringbuf_available(ringbuf_t *rb)
{
    if (rb == NULL) {
        return 0;
    }

    const size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);

    return head - tail;
}

size_t ringbuf_dropped(ringbuf_t *rb)
{
    if (rb == NULL) {
        return 0;
    }

    return atomic_load_explicit(&rb->dropped, memory_order_relaxed);
}

void ringbuf_reset_dropped(ringbuf_t *rb)
{
    if (rb == NULL) {
        return;
    }

    atomic_store_explicit(&rb->dropped, 0, memory_order_relaxed);
}
