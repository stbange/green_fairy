#include "dsp/oscillator.h"

#include "core/math3d.h"

#include <math.h>
#include <string.h>

void gf_wavetable_build_mips(GFWavetable *wt) {
    if (!wt) return;
    memcpy(wt->mips[0], wt->samples, sizeof(wt->samples));
    /* Progressive box/moving-average low-pass decimation per octave to suppress aliasing */
    for (int m = 1; m < GF_WT_MIPS; ++m) {
        const float *prev = wt->mips[m - 1];
        float *dst = wt->mips[m];
        int radius = 1 << (m - 1);
        float norm = 1.0f / (float)(2 * radius + 1);
        for (int i = 0; i < GF_WAVETABLE_SIZE; ++i) {
            float acc = 0.0f;
            for (int k = -radius; k <= radius; ++k) {
                acc += prev[(i + k + GF_WAVETABLE_SIZE) & (GF_WAVETABLE_SIZE - 1)];
            }
            dst[i] = acc * norm;
        }
    }
    wt->has_mips = true;
}

void gf_wavetable_from_harmonics(GFWavetable *wt, const float *harm_amps, int count) {
    if (!wt || !harm_amps || count <= 0) return;
    memset(wt->samples, 0, sizeof(wt->samples));
    memset(wt->harmonics, 0, sizeof(wt->harmonics));
    int n = count > GF_MAX_HARMONICS ? GF_MAX_HARMONICS : count;
    for (int h = 0; h < n; ++h) {
        float a = harm_amps[h];
        wt->harmonics[h] = a;
        if (fabsf(a) < 1e-6f) continue;
        float harm_num = (float)(h + 1);
        for (int i = 0; i < GF_WAVETABLE_SIZE; ++i) {
            wt->samples[i] += a * sinf(GF_TAU * harm_num * (float)i / (float)GF_WAVETABLE_SIZE);
        }
    }
    float peak = 0.0f;
    for (int i = 0; i < GF_WAVETABLE_SIZE; ++i) {
        float ab = fabsf(wt->samples[i]);
        if (ab > peak) peak = ab;
    }
    if (peak > 1e-6f) {
        float inv = 1.0f / peak;
        for (int i = 0; i < GF_WAVETABLE_SIZE; ++i) wt->samples[i] *= inv;
    }
    gf_wavetable_build_mips(wt);
}

void gf_wavetable_gen_sine(GFWavetable *wt) {
    if (!wt) return;
    float h[1] = {1.0f};
    gf_wavetable_from_harmonics(wt, h, 1);
}

void gf_wavetable_gen_saw(GFWavetable *wt) {
    if (!wt) return;
    float h[32];
    for (int i = 0; i < 32; ++i) {
        int k = i + 1;
        h[i] = ((k & 1) ? 1.0f : -1.0f) / (float)k;
    }
    gf_wavetable_from_harmonics(wt, h, 32);
}

void gf_wavetable_gen_square(GFWavetable *wt) {
    if (!wt) return;
    float h[32];
    for (int i = 0; i < 32; ++i) {
        int k = i + 1;
        h[i] = (k & 1) ? (1.0f / (float)k) : 0.0f;
    }
    gf_wavetable_from_harmonics(wt, h, 32);
}

void gf_wavetable_gen_emerald(GFWavetable *wt) {
    if (!wt) return;
    float h[16] = {0};
    h[0] = 0.60f; h[1] = 0.25f; h[2] = 0.20f; h[4] = 0.14f;
    h[6] = 0.10f; h[10] = 0.07f; h[12] = 0.05f; h[15] = 0.03f;
    gf_wavetable_from_harmonics(wt, h, 16);
}

void gf_wavetable_gen_vocal(GFWavetable *wt) {
    if (!wt) return;
    float h[24] = {0};
    for (int i = 0; i < 24; ++i) {
        float f = (float)(i + 1);
        /* Spectral formant bumps around partial 3 and partial 9 */
        float b1 = expf(-0.5f * (f - 3.0f) * (f - 3.0f) / 1.8f);
        float b2 = 0.55f * expf(-0.5f * (f - 9.0f) * (f - 9.0f) / 3.2f);
        h[i] = (0.35f / f) + b1 + b2;
    }
    gf_wavetable_from_harmonics(wt, h, 24);
}

bool gf_wavetable_load_custom(GFWavetable *wt, const float *samples, int count) {
    if (!wt || !samples || count < 2) return false;
    float peak = 0.0f;
    for (int i = 0; i < GF_WAVETABLE_SIZE; ++i) {
        float src_f = (float)i * (float)(count - 1) / (float)(GF_WAVETABLE_SIZE - 1);
        int i0 = (int)src_f;
        int i1 = (i0 + 1 < count) ? (i0 + 1) : i0;
        float frac = src_f - (float)i0;
        float s = gf_lerpf(samples[i0], samples[i1], frac);
        if (!isfinite(s)) s = 0.0f;
        wt->samples[i] = s;
        float a = fabsf(s);
        if (a > peak) peak = a;
    }
    if (peak > 1e-6f) {
        float inv = 1.0f / peak;
        for (int i = 0; i < GF_WAVETABLE_SIZE; ++i) wt->samples[i] *= inv;
    }
    gf_wavetable_build_mips(wt);
    return true;
}

static float lookup_array(const float *arr, float phase01) {
    float p = phase01 - floorf(phase01);
    if (p < 0.0f) p += 1.0f;
    float idx_f = p * (float)GF_WAVETABLE_SIZE;
    int i0 = ((int)idx_f) & (GF_WAVETABLE_SIZE - 1);
    int i1 = (i0 + 1) & (GF_WAVETABLE_SIZE - 1);
    float frac = idx_f - floorf(idx_f);
    return gf_lerpf(arr[i0], arr[i1], frac);
}

float gf_wavetable_lookup(const GFWavetable *wt, float phase01) {
    if (!wt) return 0.0f;
    return lookup_array(wt->samples, phase01);
}

float gf_wavetable_lookup_bandlimited(const GFWavetable *wt, float phase01, float phase_inc) {
    if (!wt) return 0.0f;
    if (!wt->has_mips || phase_inc <= (1.0f / 512.0f)) {
        return lookup_array(wt->samples, phase01);
    }
    float mip_f = gf_clampf(log2f(phase_inc * 512.0f), 0.0f, (float)(GF_WT_MIPS - 1));
    int m0 = (int)mip_f;
    int m1 = (m0 + 1 < GF_WT_MIPS) ? (m0 + 1) : m0;
    float frac = mip_f - (float)m0;
    float s0 = lookup_array(wt->mips[m0], phase01);
    float s1 = lookup_array(wt->mips[m1], phase01);
    return gf_lerpf(s0, s1, frac);
}

void gf_osc_reset(GFOscState *st, uint32_t seed) {
    if (!st) return;
    memset(st, 0, sizeof(*st));
    gf_prng_seed(&st->rng, seed);
    for (int u = 0; u < GF_UNISON_MAX; ++u) {
        st->unison_phases[u] = gf_prng_f32(&st->rng);
    }
}

static float sample_read_mono(const GFAudioSample *samp, float pos) {
    if (!samp || !samp->samples || samp->frame_count == 0) return 0.0f;
    if (pos < 0.0f || pos >= (float)samp->frame_count) return 0.0f;
    uint32_t i0 = (uint32_t)pos;
    uint32_t i1 = (i0 + 1 < samp->frame_count) ? (i0 + 1) : i0;
    float frac = pos - (float)i0;
    float s0 = samp->samples[i0 * samp->channels];
    float s1 = samp->samples[i1 * samp->channels];
    return gf_lerpf(s0, s1, frac);
}

/* 4-section polyphase all-pass quadrature Hilbert pair (Niemitalo coefficients) */
static void hilbert_process(GFHilbertState *h, float x, float *out_i, float *out_q) {
    static const float ca[4] = {0.6923878f, 0.9360654f, 0.9882295f, 0.9987488f};
    static const float cb[4] = {0.4021921f, 0.8561711f, 0.9722910f, 0.9952885f};

    float yi = h->delay_x;
    h->delay_x = x;
    for (int k = 0; k < 4; ++k) {
        float c2 = ca[k] * ca[k];
        float yn = c2 * (yi + h->i_y1[k]) - h->i_x1[k];
        h->i_x1[k] = yi;
        h->i_y1[k] = yn;
        yi = yn;
    }

    float yq = x;
    for (int k = 0; k < 4; ++k) {
        float c2 = cb[k] * cb[k];
        float yn = c2 * (yq + h->q_y1[k]) - h->q_x1[k];
        h->q_x1[k] = yq;
        h->q_y1[k] = yn;
        yq = yn;
    }
    *out_i = yi;
    *out_q = yq;
}

float gf_osc_step(GFOscState *st, const GFOscParams *p,
                  const GFWavetable *wt_a, const GFWavetable *wt_b,
                  const GFAudioSample *sample,
                  float base_freq_hz, float fm_ext_in, float morph_mod,
                  float fractal_mod, float grain_pos_mod,
                  float *out_stereo_r, float sample_rate) {
    if (out_stereo_r) *out_stereo_r = 0.0f;
    if (!st || !p || p->mode == GF_OSC_OFF || sample_rate <= 1.0f) {
        return 0.0f;
    }

    float total_semi = p->detune_semi + (p->fine_cents * 0.01f);
    float freq = base_freq_hz * powf(2.0f, total_semi / 12.0f);
    freq = gf_clampf(freq, 6.0f, sample_rate * 0.46f);
    float inc = freq / sample_rate;
    float morph = gf_clampf(p->morph + morph_mod, 0.0f, 1.0f);
    float out_l = 0.0f;
    float out_r = 0.0f;
    bool is_stereo = false;

    switch (p->mode) {
        case GF_OSC_WAVETABLE_SINGLE: {
            /* Phase-distortion warp controlled by morph */
            float warped = st->phase;
            if (morph > 0.01f) {
                float pivot = 0.5f - morph * 0.42f;
                warped = (st->phase < pivot)
                    ? (0.5f * st->phase / pivot)
                    : (0.5f + 0.5f * (st->phase - pivot) / (1.0f - pivot));
            }
            out_l = gf_wavetable_lookup_bandlimited(wt_a, warped, inc);
            st->phase += inc;
            if (st->phase >= 1.0f) st->phase -= floorf(st->phase);
            break;
        }
        case GF_OSC_WAVETABLE_DOUBLE: {
            float sync_ratio = gf_clampf(p->mod_ratio, 0.25f, 16.0f);
            float a = gf_wavetable_lookup_bandlimited(wt_a, st->phase, inc);
            float b = gf_wavetable_lookup_bandlimited(wt_b ? wt_b : wt_a, st->phase_b, inc * sync_ratio);
            out_l = gf_lerpf(a, b, morph);
            st->phase += inc;
            st->phase_b += inc * sync_ratio * 1.002f;
            if (st->phase >= 1.0f) {
                st->phase -= floorf(st->phase);
                if (p->fm_feedback > 0.5f) st->phase_b = 0.0f; /* Hard sync option */
            }
            if (st->phase_b >= 1.0f) st->phase_b -= floorf(st->phase_b);
            break;
        }
        case GF_OSC_FM: {
            float ratio = gf_clampf(p->mod_ratio, 0.125f, 16.0f);
            float idx = gf_clampf(p->fm_index, 0.0f, 12.0f);
            float fb = gf_clampf(p->fm_feedback, 0.0f, 0.95f) * st->prev_out * 0.35f;
            float mod = sinf((st->mod_phase + fb) * GF_TAU) * idx + fm_ext_in * 3.5f;
            out_l = gf_wavetable_lookup_bandlimited(wt_a, st->phase + mod / GF_TAU, inc);
            st->prev_out = out_l;
            st->phase += inc;
            st->mod_phase += inc * ratio;
            if (st->phase >= 1.0f) st->phase -= floorf(st->phase);
            if (st->mod_phase >= 1.0f) st->mod_phase -= floorf(st->mod_phase);
            break;
        }
        case GF_OSC_RINGMOD: {
            float ratio = gf_clampf(p->mod_ratio, 0.125f, 16.0f);
            float car = gf_wavetable_lookup_bandlimited(wt_a, st->phase, inc);
            float mod = (fabsf(fm_ext_in) > 1e-4f) ? fm_ext_in : sinf(st->mod_phase * GF_TAU);
            float mix = gf_clampf(p->fm_index, 0.0f, 1.0f);
            out_l = gf_lerpf(car, car * mod, mix);
            st->phase += inc;
            st->mod_phase += inc * ratio;
            if (st->phase >= 1.0f) st->phase -= floorf(st->phase);
            if (st->mod_phase >= 1.0f) st->mod_phase -= floorf(st->mod_phase);
            break;
        }
        case GF_OSC_SAMPLE: {
            if (sample && sample->samples && sample->frame_count > 1) {
                float pitch_ratio = (freq / 261.625565f) * ((float)sample->sample_rate / sample_rate);
                out_l = sample_read_mono(sample, st->sample_pos);
                st->sample_pos += pitch_ratio;
                if (st->sample_pos >= (float)sample->frame_count) {
                    if (p->sample_loop) {
                        st->sample_pos = fmodf(st->sample_pos, (float)sample->frame_count);
                    } else {
                        st->sample_pos = (float)sample->frame_count;
                    }
                }
            } else {
                out_l = gf_wavetable_lookup_bandlimited(wt_a, st->phase, inc);
                st->phase += inc;
                if (st->phase >= 1.0f) st->phase -= floorf(st->phase);
            }
            break;
        }
        case GF_OSC_GRANULAR: {
            if (sample && sample->samples && sample->frame_count > 16) {
                is_stereo = true;
                float density = gf_clampf(p->grain_density, 2.0f, 60.0f);
                float spawn_interval = sample_rate / density;
                st->grain_timer -= 1.0f;
                if (st->grain_timer <= 0.0f) {
                    st->grain_timer = spawn_interval;
                    for (int g = 0; g < GF_MAX_GRAINS; ++g) {
                        if (!st->grains[g].active) {
                            float g_dur = gf_clampf(p->grain_size, 0.012f, 0.35f) * sample_rate;
                            float spray = gf_clampf(p->grain_spray, 0.0f, 0.5f);
                            float jitter = gf_prng_bipolar(&st->rng) * (0.02f + spray);
                            float base_pos = gf_clampf(p->grain_pos + grain_pos_mod + jitter, 0.02f, 0.96f);
                            bool rev = gf_prng_f32(&st->rng) < gf_clampf(p->grain_reverse, 0.0f, 1.0f);
                            float step_mag = (freq / 261.625565f) * ((float)sample->sample_rate / sample_rate);
                            st->grains[g].active = true;
                            st->grains[g].pos = base_pos * (float)(sample->frame_count - 1);
                            st->grains[g].step = rev ? -step_mag : step_mag;
                            st->grains[g].age = 0.0f;
                            st->grains[g].duration = g_dur;
                            st->grains[g].pan = gf_prng_bipolar(&st->rng) * 0.8f;
                            break;
                        }
                    }
                }
                float sl = 0.0f, sr = 0.0f;
                for (int g = 0; g < GF_MAX_GRAINS; ++g) {
                    GFGrain *gr = &st->grains[g];
                    if (!gr->active) continue;
                    float env_t = gr->age / gr->duration;
                    float win = 0.5f * (1.0f - cosf(GF_TAU * env_t));
                    float s = sample_read_mono(sample, gr->pos);
                    float gl = cosf((gr->pan + 1.0f) * 0.25f * GF_PI);
                    float gr_gain = sinf((gr->pan + 1.0f) * 0.25f * GF_PI);
                    sl += s * win * gl;
                    sr += s * win * gr_gain;
                    if (!p->grain_freeze) {
                        gr->pos += gr->step;
                    }
                    gr->age += 1.0f;
                    if (gr->age >= gr->duration || gr->pos < 0.0f || gr->pos >= (float)sample->frame_count) {
                        gr->active = false;
                    }
                }
                out_l = sl * 0.55f;
                out_r = sr * 0.55f;
            } else {
                out_l = gf_wavetable_lookup_bandlimited(wt_a, st->phase, inc);
                st->phase += inc;
                if (st->phase >= 1.0f) st->phase -= floorf(st->phase);
            }
            break;
        }
        case GF_OSC_FRACTALIZE: {
            /* Absynth-style recursive self-similar waveform fractalization */
            float fdepth = gf_clampf(p->fractal_depth + fractal_mod, 0.0f, 1.0f);
            float fratio = gf_clampf(p->fractal_ratio, 1.5f, 4.0f);
            float acc = 0.0f;
            float norm = 0.0f;
            float amp = 1.0f;
            float scale = 1.0f;
            for (int iter = 0; iter < 5; ++iter) {
                float sub_inc = inc * scale;
                if (sub_inc > 0.45f) break;
                acc += amp * gf_wavetable_lookup_bandlimited(wt_a, st->phase * scale, sub_inc);
                norm += amp;
                amp *= (0.25f + 0.60f * fdepth);
                scale *= fratio;
            }
            out_l = (norm > 1e-5f) ? (acc / norm) : 0.0f;
            st->phase += inc;
            if (st->phase >= 1.0f) st->phase -= floorf(st->phase);
            break;
        }
        case GF_OSC_FREQ_SHIFT: {
            /* Single-Sideband Hilbert Bode Frequency Shifter (USB on Left, LSB stereo option on Right) */
            is_stereo = true;
            float src = gf_wavetable_lookup_bandlimited(wt_a, st->phase, inc) + fm_ext_in * 0.5f;
            float hi = 0.0f, hq = 0.0f;
            hilbert_process(&st->hilbert, src, &hi, &hq);
            float shift_hz = gf_clampf(p->shift_hz + morph_mod * 400.0f, -2000.0f, 2000.0f);
            float cs = cosf(st->shift_phase * GF_TAU);
            float sn = sinf(st->shift_phase * GF_TAU);
            out_l = hi * cs - hq * sn; /* Upper sideband */
            out_r = hi * cs + hq * sn; /* Lower sideband for wide stereo shimmer */
            st->phase += inc;
            if (st->phase >= 1.0f) st->phase -= floorf(st->phase);
            st->shift_phase += shift_hz / sample_rate;
            st->shift_phase -= floorf(st->shift_phase);
            break;
        }
        case GF_OSC_SUPER_UNISON: {
            is_stereo = true;
            int nvoices = p->unison_voices < 2 ? 7 : (p->unison_voices > GF_UNISON_MAX ? GF_UNISON_MAX : p->unison_voices);
            float det = gf_clampf(p->unison_detune + morph * 0.35f, 0.002f, 1.0f) * 0.035f;
            float spread = gf_clampf(p->unison_spread, 0.0f, 1.0f);
            float sl = 0.0f, sr = 0.0f;
            for (int u = 0; u < nvoices; ++u) {
                float norm_u = (nvoices > 1) ? ((float)u / (float)(nvoices - 1) * 2.0f - 1.0f) : 0.0f;
                float u_inc = inc * (1.0f + norm_u * det);
                const GFWavetable *wt = ((u & 1) && wt_b) ? wt_b : wt_a;
                float s = gf_wavetable_lookup_bandlimited(wt, st->unison_phases[u], u_inc);
                float upan = norm_u * spread;
                sl += s * cosf((upan + 1.0f) * 0.25f * GF_PI);
                sr += s * sinf((upan + 1.0f) * 0.25f * GF_PI);
                st->unison_phases[u] += u_inc;
                if (st->unison_phases[u] >= 1.0f) st->unison_phases[u] -= floorf(st->unison_phases[u]);
            }
            float gain = 1.0f / sqrtf((float)nvoices);
            out_l = sl * gain;
            out_r = sr * gain;
            break;
        }
        default:
            break;
    }

    float lvl = gf_clampf(p->level, 0.0f, 1.0f);
    out_l *= lvl;
    out_r = is_stereo ? (out_r * lvl) : out_l;
    if (out_stereo_r) *out_stereo_r = out_r;
    return out_l;
}
