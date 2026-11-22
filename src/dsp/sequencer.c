#include "dsp/sequencer.h"

#include "core/math3d.h"

#include <math.h>
#include <string.h>

void gf_seq_init(GFSequencer *seq) {
    if (!seq) return;
    memset(seq, 0, sizeof(*seq));
    seq->enabled = false;
    seq->bpm = 112.0f;
    seq->step_count = 16;
    seq->swing = 0.08f;
    seq->slew = 0.25f;
    static const float default_semi[16] = {0, 7, 12, 7, 3, 10, 15, 10, 5, 12, 19, 12, 7, 14, 12, 0};
    for (int i = 0; i < GF_SEQ_MAX_STEPS; ++i) {
        seq->steps[i].pitch_semi = default_semi[i];
        seq->steps[i].mod_val = (float)((i * 5 + 3) % 16) / 15.0f;
        seq->steps[i].gate = (i % 4 != 3);
    }
}

void gf_seq_reset_phase(GFSequencer *seq) {
    if (!seq) return;
    seq->current_step = 0;
    seq->phase = 0.0f;
}

void gf_seq_step_sample(GFSequencer *seq, float sample_rate,
                        float *out_pitch_oct, float *out_gate, float *out_mod) {
    if (!seq || !seq->enabled || sample_rate <= 1.0f) {
        if (out_pitch_oct) *out_pitch_oct = 0.0f;
        if (out_gate) *out_gate = 0.0f;
        if (out_mod) *out_mod = 0.0f;
        return;
    }

    int len = seq->step_count < 1 ? 1 : (seq->step_count > GF_SEQ_MAX_STEPS ? GF_SEQ_MAX_STEPS : seq->step_count);
    float steps_per_sec = (gf_clampf(seq->bpm, 30.0f, 260.0f) / 60.0f) * 4.0f; /* 16th notes */
    float swing_mult = ((seq->current_step & 1) == 1) ? (1.0f + gf_clampf(seq->swing, 0.0f, 0.45f))
                                                      : (1.0f - gf_clampf(seq->swing, 0.0f, 0.45f));
    seq->phase += (steps_per_sec / (sample_rate * swing_mult));
    if (seq->phase >= 1.0f) {
        seq->phase -= floorf(seq->phase);
        seq->current_step = (seq->current_step + 1) % len;
    }

    const GFSeqStep *st = &seq->steps[seq->current_step % len];
    float target_oct = st->pitch_semi / 12.0f;
    float alpha = 1.0f - gf_clampf(seq->slew, 0.0f, 0.995f) * 0.998f;
    seq->smooth_pitch += alpha * (target_oct - seq->smooth_pitch);
    seq->smooth_mod   += alpha * (st->mod_val - seq->smooth_mod);
    seq->gate_out = (st->gate && seq->phase < 0.72f) ? 1.0f : 0.0f;

    if (out_pitch_oct) *out_pitch_oct = seq->smooth_pitch;
    if (out_gate) *out_gate = seq->gate_out;
    if (out_mod) *out_mod = seq->smooth_mod;
}

static const float g_just_ratios[12] = {
    1.0f, 16.0f/15.0f, 9.0f/8.0f, 6.0f/5.0f, 5.0f/4.0f, 4.0f/3.0f,
    45.0f/32.0f, 3.0f/2.0f, 8.0f/5.0f, 5.0f/3.0f, 9.0f/5.0f, 15.0f/8.0f
};

static const float g_pyth_ratios[12] = {
    1.0f, 256.0f/243.0f, 9.0f/8.0f, 32.0f/27.0f, 81.0f/64.0f, 4.0f/3.0f,
    729.0f/512.0f, 3.0f/2.0f, 128.0f/81.0f, 27.0f/16.0f, 16.0f/9.0f, 243.0f/128.0f
};

/* Pelog 7-tone mapped across 12 keys in cents */
static const float g_pelog_cents[12] = {
    0.0f, 120.0f, 270.0f, 390.0f, 540.0f, 670.0f, 790.0f, 840.0f, 960.0f, 1050.0f, 1110.0f, 1160.0f
};

/* Natural harmonic series partials 8..19 normalized to [1, 2) */
static const float g_harm_ratios[12] = {
    1.0f, 9.0f/8.0f, 10.0f/8.0f, 11.0f/8.0f, 12.0f/8.0f, 13.0f/8.0f,
    14.0f/8.0f, 15.0f/8.0f, 17.0f/16.0f * 1.5f, 19.0f/16.0f * 1.4f, 7.0f/4.0f, 31.0f/16.0f
};

float gf_tuning_midi_to_hz(int midi_note, GFTuningScale scale, float root_a4_hz) {
    float a4 = (root_a4_hz > 100.0f && root_a4_hz < 1000.0f) ? root_a4_hz : 440.0f;
    float c4 = a4 * powf(2.0f, -9.0f / 12.0f);
    int rel = midi_note - 60;

    switch (scale) {
        case GF_SCALE_12TET:
            return a4 * powf(2.0f, (float)(midi_note - 69) / 12.0f);
        case GF_SCALE_JUST: {
            int oct = (rel >= 0) ? (rel / 12) : ((rel - 11) / 12);
            int deg = ((rel % 12) + 12) % 12;
            return c4 * powf(2.0f, (float)oct) * g_just_ratios[deg];
        }
        case GF_SCALE_PYTHAGOREAN: {
            int oct = (rel >= 0) ? (rel / 12) : ((rel - 11) / 12);
            int deg = ((rel % 12) + 12) % 12;
            return c4 * powf(2.0f, (float)oct) * g_pyth_ratios[deg];
        }
        case GF_SCALE_BOHLEN_PIERCE:
            return c4 * powf(3.0f, (float)rel / 13.0f);
        case GF_SCALE_QUARTER_TONE:
            return c4 * powf(2.0f, (float)rel / 24.0f);
        case GF_SCALE_PELOG: {
            int oct = (rel >= 0) ? (rel / 12) : ((rel - 11) / 12);
            int deg = ((rel % 12) + 12) % 12;
            return c4 * powf(2.0f, (float)oct + g_pelog_cents[deg] / 1200.0f);
        }
        case GF_SCALE_CARLOS_ALPHA:
            return c4 * powf(2.0f, ((float)rel * 78.0f) / 1200.0f);
        case GF_SCALE_HARMONIC: {
            int oct = (rel >= 0) ? (rel / 12) : ((rel - 11) / 12);
            int deg = ((rel % 12) + 12) % 12;
            return c4 * powf(2.0f, (float)oct) * g_harm_ratios[deg];
        }
        default:
            return a4 * powf(2.0f, (float)(midi_note - 69) / 12.0f);
    }
}

const char *gf_tuning_scale_name(GFTuningScale scale) {
    switch (scale) {
        case GF_SCALE_12TET:         return "12-TET";
        case GF_SCALE_JUST:          return "JUST";
        case GF_SCALE_PYTHAGOREAN:   return "PYTHAGOREAN";
        case GF_SCALE_BOHLEN_PIERCE: return "BOHLEN-PIERCE";
        case GF_SCALE_QUARTER_TONE:  return "QUARTER-TONE";
        case GF_SCALE_PELOG:         return "PELOG";
        case GF_SCALE_CARLOS_ALPHA:  return "CARLOS-ALPHA";
        case GF_SCALE_HARMONIC:      return "HARMONIC";
        default:                     return "12-TET";
    }
}
