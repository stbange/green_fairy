#include "dsp/aetherizer.h"

#include "core/math3d.h"

#include <math.h>
#include <string.h>

void gf_aether_reset(GFAetherizer *ae) {
    if (!ae) return;
    memset(ae, 0, sizeof(*ae));
    gf_prng_seed(&ae->rng, 0xAE74E123u);
}

static float read_delay_linear(const float *buf, uint32_t write_pos, float delay_samples) {
    float d = gf_clampf(delay_samples, 1.0f, (float)(GF_AETHER_BUF_SIZE - 4));
    float read_f = (float)write_pos - d;
    while (read_f < 0.0f) read_f += (float)GF_AETHER_BUF_SIZE;
    uint32_t i0 = ((uint32_t)read_f) & GF_AETHER_BUF_MASK;
    uint32_t i1 = (i0 + 1u) & GF_AETHER_BUF_MASK;
    float frac = read_f - floorf(read_f);
    return gf_lerpf(buf[i0], buf[i1], frac);
}

static float read_ring_abs(const float *buf, float abs_pos) {
    while (abs_pos < 0.0f) abs_pos += (float)GF_AETHER_BUF_SIZE;
    uint32_t i0 = ((uint32_t)abs_pos) & GF_AETHER_BUF_MASK;
    uint32_t i1 = (i0 + 1u) & GF_AETHER_BUF_MASK;
    float frac = abs_pos - floorf(abs_pos);
    return gf_lerpf(buf[i0], buf[i1], frac);
}

void gf_aether_process(GFAetherizer *ae, const GFAetherParams *p,
                       float in_l, float in_r,
                       float pitch_cv, float mix_cv,
                       float *out_l, float *out_r,
                       float sample_rate) {
    if (!ae || !p || !out_l || !out_r || sample_rate <= 1.0f) {
        if (out_l) *out_l = in_l;
        if (out_r) *out_r = in_r;
        return;
    }

    float wet = gf_clampf(p->wet_mix + mix_cv, 0.0f, 1.0f);
    if (wet <= 1e-5f) {
        *out_l = in_l;
        *out_r = in_r;
        return;
    }

    float base_samps = gf_clampf(p->time_sec, 0.02f, 1.2f) * sample_rate;
    float mod_depth = gf_clampf(p->mod_depth, 0.0f, 1.0f) * 90.0f;
    float fb = gf_clampf(p->feedback, 0.0f, 0.92f);
    float damp = gf_clampf(p->damping, 0.0f, 0.95f);

    float m1 = sinf(ae->mod_phase * GF_TAU) * mod_depth;
    float m2 = cosf(ae->mod_phase * GF_TAU * 1.37f) * mod_depth;
    ae->mod_phase += 0.45f / sample_rate;
    if (ae->mod_phase >= 1.0f) ae->mod_phase -= 1.0f;

    /* Spawn pitched crystal shimmer grains */
    ae->spawn_timer -= 1.0f;
    if (ae->spawn_timer <= 0.0f) {
        ae->spawn_timer = sample_rate / 22.0f;
        for (int g = 0; g < GF_AETHER_GRAINS; ++g) {
            if (!ae->grains[g].active) {
                float semi = gf_clampf(p->pitch_semi + pitch_cv * 12.0f, -12.0f, 24.0f);
                float rate = powf(2.0f, semi / 12.0f);
                if (gf_prng_f32(&ae->rng) < gf_clampf(p->reverse_prob, 0.0f, 1.0f)) {
                    rate = -rate;
                }
                float dur = (0.045f + gf_prng_f32(&ae->rng) * 0.065f) * sample_rate;
                float offset = base_samps * (0.25f + 0.70f * gf_prng_f32(&ae->rng));
                ae->grains[g].active = 1;
                ae->grains[g].read_pos = (float)ae->write_pos - offset;
                ae->grains[g].rate = rate;
                ae->grains[g].age = 0.0f;
                ae->grains[g].dur = dur;
                ae->grains[g].pan = gf_prng_bipolar(&ae->rng) * 0.85f;
                break;
            }
        }
    }

    float gr_l = 0.0f, gr_r = 0.0f;
    for (int g = 0; g < GF_AETHER_GRAINS; ++g) {
        GFAetherGrain *gr = &ae->grains[g];
        if (!gr->active) continue;
        float win = 0.5f * (1.0f - cosf(GF_TAU * (gr->age / gr->dur)));
        float s = 0.5f * (read_ring_abs(ae->buf_l, gr->read_pos) + read_ring_abs(ae->buf_r, gr->read_pos));
        gr_l += s * win * cosf((gr->pan + 1.0f) * 0.25f * GF_PI);
        gr_r += s * win * sinf((gr->pan + 1.0f) * 0.25f * GF_PI);
        gr->read_pos += gr->rate;
        gr->age += 1.0f;
        if (gr->age >= gr->dur) gr->active = 0;
    }

    float tap_l = 0.40f * read_delay_linear(ae->buf_l, ae->write_pos, base_samps + m1)
                + 0.22f * read_delay_linear(ae->buf_r, ae->write_pos, base_samps * 0.618f + m2)
                + 0.45f * gr_l;
    float tap_r = 0.40f * read_delay_linear(ae->buf_r, ae->write_pos, base_samps * 0.89f + m2)
                + 0.22f * read_delay_linear(ae->buf_l, ae->write_pos, base_samps * 0.52f - m1)
                + 0.45f * gr_r;

    ae->damp_l = gf_lerpf(tap_l, ae->damp_l, damp);
    ae->damp_r = gf_lerpf(tap_r, ae->damp_r, damp);

    ae->buf_l[ae->write_pos] = tanhf(in_l + ae->damp_l * fb);
    ae->buf_r[ae->write_pos] = tanhf(in_r + ae->damp_r * fb);
    ae->write_pos = (ae->write_pos + 1u) & GF_AETHER_BUF_MASK;

    float dry_g = 1.0f - wet * 0.45f;
    *out_l = in_l * dry_g + tap_l * wet;
    *out_r = in_r * dry_g + tap_r * wet;
}
