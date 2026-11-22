#include "core/prng.h"

void gf_prng_seed(GFPrng *rng, uint32_t seed) {
    if (!rng) return;
    rng->state = (seed == 0u) ? 0x6D2B79F5u : seed;
}

uint32_t gf_prng_u32(GFPrng *rng) {
    uint32_t x = rng->state;
    if (x == 0u) x = 0x6D2B79F5u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng->state = x;
    return x;
}

float gf_prng_f32(GFPrng *rng) {
    return (float)(gf_prng_u32(rng) >> 8) * (1.0f / 16777216.0f);
}

float gf_prng_bipolar(GFPrng *rng) {
    return gf_prng_f32(rng) * 2.0f - 1.0f;
}
