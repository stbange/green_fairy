#ifndef GF_CORE_RINGBUF_H
#define GF_CORE_RINGBUF_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct GFRingBuf {
    uint8_t *storage;
    size_t elem_size;
    size_t capacity; /* Must be a power of two >= 2 */
    size_t mask;
    atomic_size_t head; /* Producer write index */
    atomic_size_t tail; /* Consumer read index */
} GFRingBuf;

bool gf_ringbuf_init(GFRingBuf *rb, void *backing_storage, size_t elem_size, size_t capacity_pow2);
bool gf_ringbuf_push(GFRingBuf *rb, const void *item);
bool gf_ringbuf_pop(GFRingBuf *rb, void *out_item);
size_t gf_ringbuf_count(const GFRingBuf *rb);
void gf_ringbuf_reset(GFRingBuf *rb);

#endif
