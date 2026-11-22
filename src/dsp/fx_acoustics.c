#include "dsp/fx_acoustics.h"

#include "core/math3d.h"

#include <math.h>
#include <string.h>

/* ---------------- 1. Bidirectional Waveguide Acoustic Pipe ---------------- */

void gf_pipe_reset(GFPipeState *st) {
    if (st) memset(st, 0, sizeof(*st));
}

void gf_pipe_process(GFPipeState *st, const GFPipeParams *p, float in_l, float in_r,
                     float freq_cv, float *out_l, float *out_r, float sample_rate) {
    if (!st || !p || !out_l || !out_r || p->wet_mix <= 1e-5f || sample_rate <= 1.0f) {
        if (out_l) *out_l = in_l;
        if (out_r) *out_r = in_r;
        return;
    }

    float hz = p->freq_hz * powf(2.0f, gf_clampf(freq_cv, -3.0f, 3.0f));
    float bore = gf_clampf(sample_rate / (2.0f * gf_clampf(hz, 25.0f, 2500.0f)), 4.0f, (float)(GF_PIPE_MAX_LEN - 8));
    uint32_t end_d = (uint32_t)bore;
    uint32_t tap_a = (uint32_t)(bore * gf_clampf(p->pickup_pos, 0.1f, 0.9f));
    uint32_t tap_b = (uint32_t)(bore * (1.0f - gf_clampf(p->pickup_pos, 0.1f, 0.9f)));

    float f_end = st->fwd[(st->pos + GF_PIPE_MAX_LEN - end_d) & GF_PIPE_MASK];
    float r_end = st->rev[(st->pos + GF_PIPE_MAX_LEN - end_d) & GF_PIPE_MASK];

    float damp = gf_clampf(p->damping, 0.02f, 0.92f);
    st->lp_f = gf_lerpf(f_end, st->lp_f, damp);
    st->lp_r = gf_lerpf(r_end, st->lp_r, damp);

    float fb = gf_clampf(p->feedback, 0.0f, 0.985f);
    float exc = 0.5f * (in_l + in_r);
    /* Open/closed pipe phase-inverted end reflection */
    st->fwd[st->pos] = tanhf(exc + st->lp_r * fb);
    st->rev[st->pos] = tanhf(exc * 0.5f - st->lp_f * fb);

    float pk_l = st->fwd[(st->pos + GF_PIPE_MAX_LEN - tap_a) & GF_PIPE_MASK]
               + st->rev[(st->pos + GF_PIPE_MAX_LEN - tap_b) & GF_PIPE_MASK];
    float pk_r = st->fwd[(st->pos + GF_PIPE_MAX_LEN - tap_b) & GF_PIPE_MASK]
               - st->rev[(st->pos + GF_PIPE_MAX_LEN - tap_a) & GF_PIPE_MASK];

    st->pos = (st->pos + 1u) & GF_PIPE_MASK;
    float wet = gf_clampf(p->wet_mix, 0.0f, 1.0f);
    float dry_g = 1.0f - wet * 0.5f;
    *out_l = in_l * dry_g + pk_l * 0.65f * wet;
    *out_r = in_r * dry_g + pk_r * 0.65f * wet;
}

/* ---------------- 2. 6-Voice Chordal Multi-Comb Resonator ---------------- */

static const float g_comb_chords[6][GF_MCOMB_VOICES] = {
    { 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f },             /* Fifths & Octaves */
    { 1.0f, 1.2f, 1.5f, 1.8f, 2.25f, 3.0f },            /* Lush Minor 9th */
    { 1.0f, 1.25f, 1.4142f, 1.5f, 1.875f, 2.25f },      /* Lydian Dream */
    { 1.0f, 1.375f, 1.618f, 2.14f, 2.718f, 3.618f },    /* Cosmic Golden/Harmonic */
    { 1.0f, 1.3333f, 1.5f, 2.0f, 2.6667f, 3.0f },       /* Suspended Cathedral */
    { 1.0f, 1.1892f, 1.4142f, 1.6818f, 2.0f, 2.3784f }, /* Diminished Mystery */
};

void gf_multicomb_reset(GFMultiCombState *st) {
    if (st) memset(st, 0, sizeof(*st));
}

void gf_multicomb_process(GFMultiCombState *st, const GFMultiCombParams *p, float in_l, float in_r,
                          float tune_cv, float *out_l, float *out_r, float sample_rate) {
    if (!st || !p || !out_l || !out_r || p->wet_mix <= 1e-5f || sample_rate <= 1.0f) {
        if (out_l) *out_l = in_l;
        if (out_r) *out_r = in_r;
        return;
    }

    int chord = p->chord_type < 0 ? 0 : (p->chord_type > 5 ? 5 : p->chord_type);
    float root = gf_clampf(p->base_hz * powf(2.0f, gf_clampf(tune_cv, -2.0f, 2.0f)), 35.0f, 1600.0f);
    float fb = gf_clampf(p->feedback, 0.0f, 0.975f);
    float damp = gf_clampf(p->damping, 0.05f, 0.90f);
    float mono = 0.5f * (in_l + in_r);

    float sl = 0.0f, sr = 0.0f;
    for (int v = 0; v < GF_MCOMB_VOICES; ++v) {
        float hz = root * g_comb_chords[chord][v];
        uint32_t d = (uint32_t)gf_clampf(sample_rate / hz, 4.0f, (float)(GF_MCOMB_LEN - 4));
        float rd = st->buf[v][(st->pos + GF_MCOMB_LEN - d) & GF_MCOMB_MASK];
        st->lp[v] = gf_lerpf(rd, st->lp[v], damp);
        float y = tanhf(mono * 0.45f + st->lp[v] * fb);
        st->buf[v][st->pos] = y;
        if (v & 1) sr += y; else sl += y;
    }
    st->pos = (st->pos + 1u) & GF_MCOMB_MASK;

    float wet = gf_clampf(p->wet_mix, 0.0f, 1.0f);
    float dry_g = 1.0f - wet * 0.45f;
    *out_l = in_l * dry_g + sl * 0.5f * wet;
    *out_r = in_r * dry_g + sr * 0.5f * wet;
}

/* ---------------- 3. 6-Tap Stereo Multitap Delay ---------------- */

void gf_multitap_reset(GFMultiTapState *st) {
    if (st) memset(st, 0, sizeof(*st));
}

void gf_multitap_process(GFMultiTapState *st, const GFMultiTapParams *p, float in_l, float in_r,
                         float time_cv, float *out_l, float *out_r, float sample_rate) {
    if (!st || !p || !out_l || !out_r || p->wet_mix <= 1e-5f || sample_rate <= 1.0f) {
        if (out_l) *out_l = in_l;
        if (out_r) *out_r = in_r;
        return;
    }

    static const float tap_ratios[6] = {0.23f, 0.382f, 0.50f, 0.618f, 0.786f, 1.0f};
    static const float tap_gains[6]  = {0.48f, 0.40f, 0.35f, 0.30f, 0.25f, 0.20f};
    float base = gf_clampf(p->base_time_sec + time_cv * 0.25f, 0.04f, 1.25f) * sample_rate;
    float spread = gf_clampf(p->spread, 0.0f, 1.0f);

    float tl = 0.0f, tr = 0.0f;
    for (int t = 0; t < 6; ++t) {
        uint32_t d = (uint32_t)gf_clampf(base * tap_ratios[t], 8.0f, (float)(GF_MTAP_BUF_LEN - 8));
        uint32_t idx = (st->pos + GF_MTAP_BUF_LEN - d) & GF_MTAP_MASK;
        float s = (t & 1) ? st->buf_r[idx] : st->buf_l[idx];
        float pan = ((t & 1) ? 1.0f : -1.0f) * spread * (0.4f + 0.1f * (float)t);
        pan = gf_clampf(pan, -1.0f, 1.0f);
        tl += s * tap_gains[t] * cosf((pan + 1.0f) * 0.25f * GF_PI);
        tr += s * tap_gains[t] * sinf((pan + 1.0f) * 0.25f * GF_PI);
    }

    float fb = gf_clampf(p->feedback, 0.0f, 0.88f);
    st->buf_l[st->pos] = tanhf(in_l + tr * fb);
    st->buf_r[st->pos] = tanhf(in_r + tl * fb);
    st->pos = (st->pos + 1u) & GF_MTAP_MASK;

    float wet = gf_clampf(p->wet_mix, 0.0f, 1.0f);
    float dry_g = 1.0f - wet * 0.40f;
    *out_l = in_l * dry_g + tl * wet;
    *out_r = in_r * dry_g + tr * wet;
}

/* ---------------- 4. 8x8 Hadamard FDN Space Cathedral Reverb ---------------- */

static const uint32_t g_fdn_primes[GF_FDN_LINES] = {
    1499u, 1877u, 2311u, 2803u, 3371u, 4007u, 4789u, 5653u
};

static void hadamard8(const float in[8], float out[8]) {
    float a0 = in[0] + in[1], a1 = in[0] - in[1];
    float a2 = in[2] + in[3], a3 = in[2] - in[3];
    float a4 = in[4] + in[5], a5 = in[4] - in[5];
    float a6 = in[6] + in[7], a7 = in[6] - in[7];

    float b0 = a0 + a2, b1 = a1 + a3, b2 = a0 - a2, b3 = a1 - a3;
    float b4 = a4 + a6, b5 = a5 + a7, b6 = a4 - a6, b7 = a5 - a7;

    const float norm = 0.35355339f; /* 1 / sqrt(8) */
    out[0] = (b0 + b4) * norm; out[1] = (b1 + b5) * norm;
    out[2] = (b2 + b6) * norm; out[3] = (b3 + b7) * norm;
    out[4] = (b0 - b4) * norm; out[5] = (b1 - b5) * norm;
    out[6] = (b2 - b6) * norm; out[7] = (b3 - b7) * norm;
}

void gf_reverb_reset(GFSpaceReverbState *st) {
    if (st) memset(st, 0, sizeof(*st));
}

void gf_reverb_process(GFSpaceReverbState *st, const GFSpaceReverbParams *p, float in_l, float in_r,
                       float decay_cv, bool freeze_gate, float *out_l, float *out_r, float sample_rate) {
    if (!st || !p || !out_l || !out_r || p->wet_mix <= 1e-5f || sample_rate <= 1.0f) {
        if (out_l) *out_l = in_l;
        if (out_r) *out_r = in_r;
        return;
    }

    bool frozen = p->freeze || freeze_gate;
    float decay = frozen ? 0.9995f : gf_clampf(p->decay + decay_cv * 0.15f, 0.20f, 0.994f);
    float damp = frozen ? 0.01f : gf_clampf(p->damping, 0.05f, 0.90f);
    float size = gf_clampf(p->size, 0.35f, 1.55f) * (sample_rate / 44100.0f);
    float shim = gf_clampf(p->shimmer, 0.0f, 0.65f);

    st->lfo_phase += 0.31f / sample_rate;
    if (st->lfo_phase >= 1.0f) st->lfo_phase -= 1.0f;

    float taps[8];
    for (int i = 0; i < GF_FDN_LINES; ++i) {
        float mod = 14.0f * sinf((st->lfo_phase + (float)i * 0.125f) * GF_TAU);
        float d_f = gf_clampf((float)g_fdn_primes[i] * size + mod, 16.0f, (float)(GF_FDN_LEN - 16));
        float rd = (float)st->pos - d_f;
        while (rd < 0.0f) rd += (float)GF_FDN_LEN;
        uint32_t i0 = ((uint32_t)rd) & GF_FDN_MASK;
        uint32_t i1 = (i0 + 1u) & GF_FDN_MASK;
        float s = gf_lerpf(st->buf[i][i0], st->buf[i][i1], rd - floorf(rd));
        st->damp[i] = gf_lerpf(s, st->damp[i], damp);
        taps[i] = st->damp[i];
    }

    float mixed[8];
    hadamard8(taps, mixed);

    st->shim_phase += 2.0f / (float)g_fdn_primes[0];
    if (st->shim_phase >= 1.0f) st->shim_phase -= 1.0f;
    float shim_sig = (fabsf(mixed[0]) - fabsf(mixed[1])) * shim * 0.45f;

    float feed_in_l = frozen ? 0.0f : (in_l * 0.45f);
    float feed_in_r = frozen ? 0.0f : (in_r * 0.45f);

    for (int i = 0; i < GF_FDN_LINES; ++i) {
        float exc = (i & 1) ? feed_in_r : feed_in_l;
        if (i < 2) exc += shim_sig;
        st->buf[i][st->pos] = tanhf(exc + mixed[i] * decay);
    }
    st->pos = (st->pos + 1u) & GF_FDN_MASK;

    float rev_l = (taps[0] + taps[2] + taps[4] + taps[6]) * 0.55f;
    float rev_r = (taps[1] + taps[3] + taps[5] + taps[7]) * 0.55f;
    float wet = gf_clampf(p->wet_mix, 0.0f, 1.0f);
    float dry_g = 1.0f - wet * 0.40f;
    *out_l = in_l * dry_g + rev_l * wet;
    *out_r = in_r * dry_g + rev_r * wet;
}
