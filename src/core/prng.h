#ifndef GF_CORE_PRNG_H
#define GF_CORE_PRNG_H

#include <stdint.h>

typedef struct GFPrng {
    uint32_t state;
} GFPrng;

void gf_prng_seed(GFPrng *rng, uint32_t seed);
uint32_t gf_prng_u32(GFPrng *rng);
float gf_prng_f32(GFPrng *rng);      /* [0.0f, 1.0f) */
float gf_prng_bipolar(GFPrng *rng);  /* [-1.0f, 1.0f) */

#endif
