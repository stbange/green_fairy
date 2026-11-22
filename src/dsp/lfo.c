#include "dsp/lfo.h"

#include "core/math3d.h"

#include <math.h>

void gf_lfo_reset(GFLfoState *st, float phase_offset, uint32_t seed) {
    if (!st) return;
    st->phase = phase_offset - floorf(phase_offset);
    if (st->phase < 0.0f) st->phase += 1.0f;
    gf_prng_seed(&st->rng, seed);
    st->sh_value = gf_prng_bipolar(&st->rng);
}

float gf_lfo_step(GFLfoState *st, const GFLfoParams *p, float sample_rate) {
    if (!st || !p || sample_rate <= 1.0f) return 0.0f;

    float ph = st->phase;
    float raw = 0.0f;
    switch (p->wave) {
        case GF_LFO_SINE:
            raw = sinf(ph * GF_TAU);
            break;
        case GF_LFO_TRI:
            raw = (ph < 0.5f) ? (4.0f * ph - 1.0f) : (3.0f - 4.0f * ph);
            break;
        case GF_LFO_SAW:
            raw = 2.0f * ph - 1.0f;
            break;
        case GF_LFO_SQUARE:
            raw = (ph < 0.5f) ? 1.0f : -1.0f;
            break;
        case GF_LFO_SAMPLE_HOLD:
            raw = st->sh_value;
            break;
    }

    float rate = gf_clampf(p->rate_hz, 0.01f, 40.0f);
    st->phase += rate / sample_rate;
    if (st->phase >= 1.0f) {
        st->phase -= floorf(st->phase);
        st->sh_value = gf_prng_bipolar(&st->rng);
    }

    return raw * p->depth;
}
