#ifndef GF_DSP_LFO_H
#define GF_DSP_LFO_H

#include "core/prng.h"

typedef enum GFLfoWave {
    GF_LFO_SINE = 0,
    GF_LFO_TRI  = 1,
    GF_LFO_SAW  = 2,
    GF_LFO_SQUARE = 3,
    GF_LFO_SAMPLE_HOLD = 4
} GFLfoWave;

typedef enum GFModTarget {
    GF_MOD_NONE = 0,
    GF_MOD_PITCH = 1,
    GF_MOD_FILTER_CUTOFF = 2,
    GF_MOD_AMP = 3,
    GF_MOD_FM_INDEX = 4,
    GF_MOD_PAN = 5
} GFModTarget;

typedef struct GFLfoParams {
    GFLfoWave wave;
    GFModTarget target;
    float rate_hz;
    float depth;
    float phase_offset;
} GFLfoParams;

typedef struct GFLfoState {
    float phase;
    float sh_value;
    GFPrng rng;
} GFLfoState;

void gf_lfo_reset(GFLfoState *st, float phase_offset, uint32_t seed);
float gf_lfo_step(GFLfoState *st, const GFLfoParams *p, float sample_rate);

#endif
