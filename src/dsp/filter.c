#include "dsp/filter.h"

#include "core/math3d.h"

#include <math.h>
#include <string.h>

/* Formant frequencies (F1, F2, F3 in Hz) for vowels A, E, I, O, U */
static const float g_vowel_hz[5][3] = {
    { 730.0f, 1090.0f, 2440.0f }, /* A */
    { 530.0f, 1840.0f, 2480.0f }, /* E */
    { 270.0f, 2290.0f, 3010.0f }, /* I */
    { 570.0f,  840.0f, 2410.0f }, /* O */
    { 300.0f,  870.0f, 2240.0f }, /* U */
};

void gf_filter_reset(GFFilterState *st) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
}

static float svf_bandpass_step(float *ic1, float *ic2, float x, float freq, float q, float sr) {
    float f = gf_clampf(freq, 40.0f, sr * 0.44f);
    float g = tanf(GF_PI * f / sr);
    float k = 1.0f / gf_clampf(q, 0.5f, 18.0f);
    float a1 = 1.0f / (1.0f + g * (g + k));
    float a2 = g * a1;
    float a3 = g * a2;
    float v3 = x - *ic2;
    float v1 = a1 * (*ic1) + a2 * v3;
    float v2 = (*ic2) + a2 * (*ic1) + a3 * v3;
    *ic1 = 2.0f * v1 - (*ic1);
    *ic2 = 2.0f * v2 - (*ic2);
    return v1;
}

float gf_filter_step(GFFilterState *st, const GFFilterParams *p,
                     float input, float cutoff_mod_octaves, float res_mod,
                     float morph_mod, float sample_rate) {
    if (!st || !p || p->mode == GF_FILTER_BYPASS || sample_rate <= 1.0f) {
        return input;
    }

    float cutoff = p->cutoff_hz * powf(2.0f, gf_clampf(cutoff_mod_octaves, -6.0f, 6.0f));
    cutoff = gf_clampf(cutoff, 20.0f, sample_rate * 0.44f);
    float res = gf_clampf(p->resonance + res_mod, 0.0f, 0.97f);
    float morph = gf_clampf(p->morph + morph_mod, 0.0f, 1.0f);

    if (p->mode >= GF_FILTER_LP && p->mode <= GF_FILTER_NOTCH) {
        float g = tanf(GF_PI * cutoff / sample_rate);
        float k = 2.0f - 1.92f * res;
        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;
        float v3 = input - st->ic2eq;
        float v1 = a1 * st->ic1eq + a2 * v3;
        float v2 = st->ic2eq + a2 * st->ic1eq + a3 * v3;
        st->ic1eq = 2.0f * v1 - st->ic1eq;
        st->ic2eq = 2.0f * v2 - st->ic2eq;
        switch (p->mode) {
            case GF_FILTER_LP:    return v2;
            case GF_FILTER_HP:    return input - k * v1 - v2;
            case GF_FILTER_BP:    return v1;
            case GF_FILTER_NOTCH: return input - k * v1;
            default:              return input;
        }
    }

    if (p->mode >= GF_FILTER_LADDER_LP24 && p->mode <= GF_FILTER_LADDER_BP24) {
        /* 4-pole nonlinear transistor ladder filter */
        float g = gf_clampf(1.0f - expf(-GF_TAU * cutoff / sample_rate), 0.001f, 0.92f);
        float fb = res * 3.85f;
        float u = tanhf(input - fb * st->stg[3]);
        st->stg[0] += g * (u - st->stg[0]);
        st->stg[1] += g * (st->stg[0] - st->stg[1]);
        st->stg[2] += g * (st->stg[1] - st->stg[2]);
        st->stg[3] += g * (st->stg[2] - st->stg[3]);
        if (p->mode == GF_FILTER_LADDER_LP24) return st->stg[3];
        if (p->mode == GF_FILTER_LADDER_HP24) return u - 4.0f * st->stg[0] + 6.0f * st->stg[1] - 4.0f * st->stg[2] + st->stg[3];
        return 4.0f * (st->stg[1] - st->stg[3]);
    }

    if (p->mode == GF_FILTER_ALLPASS) {
        float g = tanf(GF_PI * gf_clampf(cutoff, 40.0f, 12000.0f) / sample_rate);
        float c = (g - 1.0f) / (g + 1.0f);
        float x = input + st->ic1eq * (res * 0.75f);
        for (int s = 0; s < 4; ++s) {
            float y = c * x + st->stg[s];
            st->stg[s] = x - c * y;
            x = y;
        }
        st->ic1eq = tanhf(x);
        return 0.5f * (input + x);
    }

    if (p->mode == GF_FILTER_COMB_POS || p->mode == GF_FILTER_COMB_NEG) {
        /* Key/cutoff-tuned damped Karplus-Strong physical resonator */
        float delay_samps = gf_clampf(sample_rate / gf_clampf(cutoff, 25.0f, 8000.0f), 2.0f, (float)(GF_COMB_MAX_DELAY - 4));
        float rd = (float)st->comb_pos - delay_samps;
        while (rd < 0.0f) rd += (float)GF_COMB_MAX_DELAY;
        uint32_t i0 = ((uint32_t)rd) & GF_COMB_MASK;
        uint32_t i1 = (i0 + 1u) & GF_COMB_MASK;
        float frac = rd - floorf(rd);
        float delayed = gf_lerpf(st->comb_buf[i0], st->comb_buf[i1], frac);

        float damp = gf_clampf(0.08f + morph * 0.75f, 0.02f, 0.92f);
        st->comb_damp = gf_lerpf(delayed, st->comb_damp, damp);
        float sign = (p->mode == GF_FILTER_COMB_NEG) ? -1.0f : 1.0f;
        float fb = sign * (0.35f + res * 0.635f);
        float y = tanhf(input + st->comb_damp * fb);
        st->comb_buf[st->comb_pos] = y;
        st->comb_pos = (st->comb_pos + 1u) & GF_COMB_MASK;
        return y;
    }

    if (p->mode == GF_FILTER_FORMANT) {
        float vpos = morph * 4.0f;
        int v0 = (int)vpos;
        if (v0 > 3) v0 = 3;
        int v1 = v0 + 1;
        float vf = vpos - (float)v0;
        float shift = gf_clampf(cutoff / 1000.0f, 0.45f, 2.2f);
        float q = 3.5f + res * 10.0f;
        static const float gains[3] = {0.65f, 0.42f, 0.25f};
        float sum = 0.0f;
        for (int b = 0; b < 3; ++b) {
            float f_hz = gf_lerpf(g_vowel_hz[v0][b], g_vowel_hz[v1][b], vf) * shift;
            sum += gains[b] * svf_bandpass_step(&st->f_ic1[b], &st->f_ic2[b], input, f_hz, q, sample_rate);
        }
        return sum * 1.4f;
    }

    if (p->mode == GF_FILTER_CLOUD) {
        /* Absynth 5 Cloud Filter: shimmering modulated harmonic resonator + micro-delay grain cloud */
        st->cloud_phase += (0.8f + morph * 5.0f) / sample_rate;
        if (st->cloud_phase >= 1.0f) st->cloud_phase -= 1.0f;
        float mod1 = 1.0f + 0.18f * sinf(st->cloud_phase * GF_TAU);
        float mod2 = 1.5f + 0.22f * cosf(st->cloud_phase * GF_TAU * 1.618f);
        float q = 2.5f + res * 9.0f;
        float b1 = svf_bandpass_step(&st->f_ic1[0], &st->f_ic2[0], input, cutoff * mod1, q, sample_rate);
        float b2 = svf_bandpass_step(&st->f_ic1[1], &st->f_ic2[1], input, cutoff * mod2, q, sample_rate);
        st->comb_buf[st->comb_pos] = tanhf(input + (b1 + b2) * 0.5f * res);
        uint32_t tap = (st->comb_pos + GF_COMB_MAX_DELAY - 317u) & GF_COMB_MASK;
        st->comb_pos = (st->comb_pos + 1u) & GF_COMB_MASK;
        return gf_lerpf(input, (b1 + b2) * 0.8f + st->comb_buf[tap] * 0.4f, 0.45f + morph * 0.55f);
    }

    /* GF_FILTER_PEAK */
    float bp = svf_bandpass_step(&st->ic1eq, &st->ic2eq, input, cutoff, 1.5f + res * 8.0f, sample_rate);
    return input + bp * (1.0f + res * 3.0f);
}
