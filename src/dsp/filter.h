#ifndef GF_DSP_FILTER_H
#define GF_DSP_FILTER_H

#include <stdint.h>

#define GF_COMB_MAX_DELAY 4096
#define GF_COMB_MASK      (GF_COMB_MAX_DELAY - 1)

typedef enum GFFilterMode {
    GF_FILTER_BYPASS      = 0,
    GF_FILTER_LP          = 1,
    GF_FILTER_HP          = 2,
    GF_FILTER_BP          = 3,
    GF_FILTER_NOTCH       = 4,
    GF_FILTER_LADDER_LP24 = 5,
    GF_FILTER_LADDER_HP24 = 6,
    GF_FILTER_LADDER_BP24 = 7,
    GF_FILTER_ALLPASS     = 8,
    GF_FILTER_COMB_POS    = 9,
    GF_FILTER_COMB_NEG    = 10,
    GF_FILTER_FORMANT     = 11,
    GF_FILTER_CLOUD       = 12,
    GF_FILTER_PEAK        = 13
} GFFilterMode;

typedef struct GFFilterParams {
    GFFilterMode mode;
    float cutoff_hz; /* [20, 18000] */
    float resonance; /* [0.0, 0.98] */
    float morph;     /* [0.0, 1.0] vowel morph / comb damping / cloud shimmer */
} GFFilterParams;

typedef struct GFFilterState {
    float ic1eq;
    float ic2eq;
    float stg[4];      /* 4-pole ladder / allpass states */
    float f_ic1[3];    /* 3-band parallel formant states */
    float f_ic2[3];
    float comb_buf[GF_COMB_MAX_DELAY];
    uint32_t comb_pos;
    float comb_damp;
    float cloud_phase;
} GFFilterState;

void gf_filter_reset(GFFilterState *st);
float gf_filter_step(GFFilterState *st, const GFFilterParams *p,
                     float input, float cutoff_mod_octaves, float res_mod,
                     float morph_mod, float sample_rate);

#endif
