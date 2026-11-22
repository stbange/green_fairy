#ifndef GF_DSP_AETHERIZER_H
#define GF_DSP_AETHERIZER_H

#include "core/prng.h"
#include <stdint.h>

#define GF_AETHER_BUF_SIZE 65536
#define GF_AETHER_BUF_MASK (GF_AETHER_BUF_SIZE - 1)
#define GF_AETHER_GRAINS   12

typedef struct GFAetherGrain {
    int active;
    float read_pos;
    float rate;
    float age;
    float dur;
    float pan;
} GFAetherGrain;

typedef struct GFAetherParams {
    float time_sec;    /* [0.02, 1.2] grain cloud delay window */
    float feedback;    /* [0.0, 0.92] regeneration */
    float mod_depth;   /* [0.0, 1.0] shimmer / spray */
    float damping;     /* [0.0, 0.95] HF tone damping */
    float wet_mix;     /* [0.0, 1.0] */
    float pitch_semi;  /* [-12.0, +24.0] granular crystal pitch shift */
    float reverse_prob;/* [0.0, 1.0] reverse grain probability */
} GFAetherParams;

typedef struct GFAetherizer {
    float buf_l[GF_AETHER_BUF_SIZE];
    float buf_r[GF_AETHER_BUF_SIZE];
    uint32_t write_pos;
    float damp_l;
    float damp_r;
    float mod_phase;
    float spawn_timer;
    GFAetherGrain grains[GF_AETHER_GRAINS];
    GFPrng rng;
} GFAetherizer;

void gf_aether_reset(GFAetherizer *ae);
void gf_aether_process(GFAetherizer *ae, const GFAetherParams *p,
                       float in_l, float in_r,
                       float pitch_cv, float mix_cv,
                       float *out_l, float *out_r,
                       float sample_rate);

#endif
