#include "core/ringbuf.h"

#include <string.h>

static bool is_power_of_two(size_t n) {
    return n >= 2 && (n & (n - 1)) == 0;
}

bool gf_ringbuf_init(GFRingBuf *rb, void *backing_storage, size_t elem_size, size_t capacity_pow2) {
    if (!rb || !backing_storage || elem_size == 0 || !is_power_of_two(capacity_pow2)) {
        return false;
    }
    rb->storage = (uint8_t *)backing_storage;
    rb->elem_size = elem_size;
    rb->capacity = capacity_pow2;
    rb->mask = capacity_pow2 - 1;
    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);
    return true;
}

bool gf_ringbuf_push(GFRingBuf *rb, const void *item) {
    if (!rb || !item) return false;
    size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    if ((head - tail) >= rb->capacity) {
        return false;
    }
    size_t slot = head & rb->mask;
    memcpy(rb->storage + slot * rb->elem_size, item, rb->elem_size);
    atomic_store_explicit(&rb->head, head + 1, memory_order_release);
    return true;
}

bool gf_ringbuf_pop(GFRingBuf *rb, void *out_item) {
    if (!rb || !out_item) return false;
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    if (tail == head) {
        return false;
    }
    size_t slot = tail & rb->mask;
    memcpy(out_item, rb->storage + slot * rb->elem_size, rb->elem_size);
    atomic_store_explicit(&rb->tail, tail + 1, memory_order_release);
    return true;
}

size_t gf_ringbuf_count(const GFRingBuf *rb) {
    if (!rb) return 0;
    size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    return head - tail;
}

void gf_ringbuf_reset(GFRingBuf *rb) {
    if (!rb) return;
    atomic_store_explicit(&rb->head, 0, memory_order_release);
    atomic_store_explicit(&rb->tail, 0, memory_order_release);
}
