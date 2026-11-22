#include "dsp/synth.h"

#include "core/math3d.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void publish_patch_plan(GFSynth *s) {
    int cur = atomic_load_explicit(&s->active_plan_idx, memory_order_relaxed);
    int nxt = 1 - cur;
    gf_patchbay_compile(&s->patchbay, &s->plans[nxt]);
    atomic_store_explicit(&s->active_plan_idx, nxt, memory_order_release);
}

const GFPatchPlan *gf_synth_active_plan(const GFSynth *s) {
    if (!s) return NULL;
    int idx = atomic_load_explicit(&s->active_plan_idx, memory_order_acquire);
    return &s->plans[idx];
}

void gf_synth_clear_cables(GFSynth *s) {
    if (!s) return;
    gf_patchbay_clear(&s->patchbay);
    publish_patch_plan(s);
}

void gf_synth_wire_default_cables(GFSynth *s) {
    if (!s) return;
    gf_patchbay_wire_default_absynth(&s->patchbay);
    publish_patch_plan(s);
}

int gf_synth_connect_cable(GFSynth *s, GFPortId src, GFPortId dst, float gain) {
    if (!s) return -1;
    int id = gf_patchbay_connect(&s->patchbay, src, dst, gain);
    if (id > 0) publish_patch_plan(s);
    return id;
}

bool gf_synth_disconnect_cable(GFSynth *s, int cable_id) {
    if (!s) return false;
    bool ok = gf_patchbay_disconnect(&s->patchbay, cable_id);
    if (ok) publish_patch_plan(s);
    return ok;
}

bool gf_synth_set_cable_gain(GFSynth *s, int cable_id, float gain) {
    if (!s) return false;
    bool ok = gf_patchbay_set_gain(&s->patchbay, cable_id, gain);
    if (ok) publish_patch_plan(s);
    return ok;
}

static void init_default_snapshot(GFPatchSnapshot *p) {
    memset(p, 0, sizeof(*p));
    p->master_gain = 0.72f;
    p->master_filter = (GFFilterParams){GF_FILTER_LADDER_LP24, 11500.0f, 0.16f, 0.2f};
    p->master_shaper = (GFShaperParams){.mode = GF_SHAPER_SOFTCLIP, .drive = 1.25f, .mix = 0.20f, .phase_offset = 0.0f};
    p->pipe      = (GFPipeParams){220.0f, 0.72f, 0.45f, 0.37f, 0.0f};
    p->multicomb = (GFMultiCombParams){110.0f, 0, 0.75f, 0.40f, 0.0f};
    p->multitap  = (GFMultiTapParams){0.36f, 0.42f, 0.75f, 0.18f};
    p->aether    = (GFAetherParams){0.34f, 0.46f, 0.40f, 0.42f, 0.28f, 12.0f, 0.25f};
    p->reverb    = (GFSpaceReverbParams){0.86f, 0.35f, 0.22f, 1.0f, 0.34f, false};

    /* Ch 1: Emerald Mip-Mapped Double Wavetable */
    p->ch[0].osc = (GFOscParams){
        .mode = GF_OSC_WAVETABLE_DOUBLE, .level = 0.72f, .pan = -0.18f,
        .detune_semi = 0.0f, .fine_cents = -4.0f, .morph = 0.35f,
        .mod_ratio = 2.0f, .fm_index = 1.5f, .fm_feedback = 0.15f,
        .fractal_depth = 0.4f, .fractal_ratio = 2.0f, .shift_hz = 7.0f,
        .grain_size = 0.08f, .grain_density = 16.0f, .grain_pos = 0.25f,
        .grain_spray = 0.08f, .grain_reverse = 0.2f, .grain_freeze = false,
        .unison_voices = 5, .unison_detune = 0.28f, .unison_spread = 0.75f,
        .sample_index = 0, .sample_loop = true
    };
    p->ch[0].filter = (GFFilterParams){GF_FILTER_LADDER_LP24, 4600.0f, 0.24f, 0.3f};
    p->ch[0].shaper = (GFShaperParams){.mode = GF_SHAPER_BYPASS, .drive = 1.0f, .mix = 0.0f};

    /* Ch 2: FM Crystal Shimmer */
    p->ch[1].osc = p->ch[0].osc;
    p->ch[1].osc.mode = GF_OSC_FM;
    p->ch[1].osc.level = 0.42f;
    p->ch[1].osc.pan = 0.22f;
    p->ch[1].osc.detune_semi = 12.0f;
    p->ch[1].osc.fine_cents = 5.0f;
    p->ch[1].osc.mod_ratio = 3.0f;
    p->ch[1].osc.fm_index = 2.1f;
    p->ch[1].filter = (GFFilterParams){GF_FILTER_BP, 2800.0f, 0.28f, 0.4f};
    p->ch[1].shaper = (GFShaperParams){.mode = GF_SHAPER_SOFTCLIP, .drive = 1.4f, .mix = 0.18f};

    /* Ch 3: Sub Super-Unison / Ringmod */
    p->ch[2].osc = p->ch[0].osc;
    p->ch[2].osc.mode = GF_OSC_SUPER_UNISON;
    p->ch[2].osc.level = 0.38f;
    p->ch[2].osc.pan = 0.0f;
    p->ch[2].osc.detune_semi = -12.0f;
    p->ch[2].osc.fine_cents = 0.0f;
    p->ch[2].filter = (GFFilterParams){GF_FILTER_LP, 1900.0f, 0.15f, 0.2f};
    p->ch[2].shaper = (GFShaperParams){.mode = GF_SHAPER_BYPASS, .drive = 1.0f, .mix = 0.0f};

    p->lfo[0] = (GFLfoParams){GF_LFO_SINE, GF_MOD_FILTER_CUTOFF, 0.55f, 0.35f, 0.0f};
    p->lfo[1] = (GFLfoParams){GF_LFO_TRI,  GF_MOD_FM_INDEX,      0.28f, 0.45f, 0.25f};
    p->lfo[2] = (GFLfoParams){GF_LFO_SINE, GF_MOD_PAN,           0.38f, 0.25f, 0.5f};
}

void gf_synth_reset_default_patch(GFSynth *s) {
    if (!s) return;
    init_default_snapshot(&s->base_patch);
    for (int i = 0; i < 4; ++i) s->xy_corners[i] = s->base_patch;
    s->xy_corners[1].ch[0].filter.cutoff_hz = 12000.0f;
    s->xy_corners[1].ch[1].osc.fm_index = 5.5f;
    s->xy_corners[2].ch[0].filter.cutoff_hz = 750.0f;
    s->xy_corners[2].reverb.wet_mix = 0.68f;
    s->xy_corners[3].ch[1].osc.fm_index = 7.5f;
    s->xy_corners[3].master_filter.cutoff_hz = 15000.0f;
    s->xy_corners[3].aether.wet_mix = 0.60f;
    s->xy_enabled = true;
    s->xy_x = 0.30f;
    s->xy_y = 0.25f;
    s->xy_z = 0.50f;

    s->tuning_scale = GF_SCALE_12TET;
    s->tuning_a4_hz = 440.0f;
    s->voice_glide = 0.02f;
    s->drone_latch = false;
    gf_seq_init(&s->sequencer);

    gf_env_init_adsr(&s->env_defs[0], 0.035f, 0.45f, 0.76f, 0.85f); /* Amp */
    gf_env_init_adsr(&s->env_defs[1], 0.060f, 0.60f, 0.48f, 0.70f); /* Filter */
    gf_env_init_adsr(&s->env_defs[2], 0.120f, 0.55f, 0.35f, 0.60f); /* FM / Mod */
    for (int e = 3; e < GF_NUM_ENVS; ++e) {
        gf_env_init_adsr(&s->env_defs[e], 0.08f + 0.03f * (float)e, 0.5f, 0.5f, 0.6f);
    }

    gf_wavetable_gen_emerald(&s->wt_a[0]);
    gf_wavetable_gen_vocal(&s->wt_b[0]);
    gf_wavetable_gen_sine(&s->wt_a[1]);
    gf_wavetable_gen_square(&s->wt_b[1]);
    gf_wavetable_gen_saw(&s->wt_a[2]);
    gf_wavetable_gen_emerald(&s->wt_b[2]);

    gf_patchbay_init(&s->patchbay);
    gf_synth_wire_default_cables(s);
}

void gf_synth_init(GFSynth *s, float sample_rate, uint32_t seed) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->sample_rate = (sample_rate >= 8000.0f) ? sample_rate : 44100.0f;
    s->base_seed = seed ? seed : 0x51A7BEEFu;
    gf_ringbuf_init(&s->cmd_queue, s->cmd_storage, sizeof(GFSynthCmd), GF_CMD_QUEUE_CAP);
    atomic_init(&s->active_plan_idx, 0);
    atomic_init(&s->telemetry_idx, 0);
    gf_pipe_reset(&s->pipe);
    gf_multicomb_reset(&s->multicomb);
    gf_multitap_reset(&s->multitap);
    gf_aether_reset(&s->aether);
    gf_reverb_reset(&s->reverb);
    gf_synth_reset_default_patch(s);
}

void gf_synth_destroy(GFSynth *s) {
    if (!s) return;
    for (int i = 0; i < GF_NUM_SAMPLES; ++i) {
        gf_audio_sample_free(&s->samples[i]);
    }
}

static void apply_param_internal(GFSynth *s, GFParamId id, float val) {
    GFPatchSnapshot *p = &s->base_patch;

    if (id >= GF_P_CH_BASE && id < GF_P_CH_BASE + GF_NUM_CHANNELS * GF_P_CH_STRIDE) {
        int rel = id - GF_P_CH_BASE;
        GFChannelPatch *c = &p->ch[rel / GF_P_CH_STRIDE];
        switch (rel % GF_P_CH_STRIDE) {
            case 0:  c->osc.mode = (GFOscMode)gf_clampf(val, 0.0f, 9.0f); break;
            case 1:  c->osc.level = gf_clampf(val, 0.0f, 1.0f); break;
            case 2:  c->osc.pan = gf_clampf(val, -1.0f, 1.0f); break;
            case 3:  c->osc.detune_semi = gf_clampf(val, -36.0f, 36.0f); break;
            case 4:  c->osc.fine_cents = gf_clampf(val, -100.0f, 100.0f); break;
            case 5:  c->osc.morph = gf_clampf(val, 0.0f, 1.0f); break;
            case 6:  c->osc.mod_ratio = gf_clampf(val, 0.125f, 16.0f); break;
            case 7:  c->osc.fm_index = gf_clampf(val, 0.0f, 12.0f); break;
            case 8:  c->osc.fm_feedback = gf_clampf(val, 0.0f, 0.95f); break;
            case 9:  c->osc.fractal_depth = gf_clampf(val, 0.0f, 1.0f); break;
            case 10: c->osc.fractal_ratio = gf_clampf(val, 1.5f, 4.0f); break;
            case 11: c->osc.shift_hz = gf_clampf(val, -2000.0f, 2000.0f); break;
            case 12: c->osc.grain_size = gf_clampf(val, 0.012f, 0.35f); break;
            case 13: c->osc.grain_density = gf_clampf(val, 2.0f, 60.0f); break;
            case 14: c->osc.grain_pos = gf_clampf(val, 0.0f, 1.0f); break;
            case 15: c->osc.grain_spray = gf_clampf(val, 0.0f, 0.5f); break;
            case 16: c->osc.grain_reverse = gf_clampf(val, 0.0f, 1.0f); break;
            case 17: c->osc.unison_voices = (int)gf_clampf(val, 1.0f, 8.0f); break;
            case 18: c->osc.unison_detune = gf_clampf(val, 0.0f, 1.0f); break;
            case 19: c->osc.unison_spread = gf_clampf(val, 0.0f, 1.0f); break;
            case 20: c->osc.sample_index = (int)gf_clampf(val, 0.0f, (float)(GF_NUM_SAMPLES - 1)); break;
            case 21: c->filter.mode = (GFFilterMode)gf_clampf(val, 0.0f, 13.0f); break;
            case 22: c->filter.cutoff_hz = gf_clampf(val, 20.0f, 18000.0f); break;
            case 23: c->filter.resonance = gf_clampf(val, 0.0f, 0.97f); break;
        }
        return;
    }

    if (id >= GF_P_CH_EXT_BASE && id < GF_P_CH_EXT_BASE + GF_NUM_CHANNELS * GF_P_CH_EXT_STRIDE) {
        int rel = id - GF_P_CH_EXT_BASE;
        GFChannelPatch *c = &p->ch[rel / GF_P_CH_EXT_STRIDE];
        switch (rel % GF_P_CH_EXT_STRIDE) {
            case 0: c->filter.morph = gf_clampf(val, 0.0f, 1.0f); break;
            case 1: c->shaper.mode = (GFShaperMode)gf_clampf(val, 0.0f, 5.0f); break;
            case 2: c->shaper.drive = gf_clampf(val, 1.0f, 20.0f); if (val > 1.05f && c->shaper.mix <= 0.0f) c->shaper.mix = 0.5f; break;
            case 3: c->shaper.mix = gf_clampf(val, 0.0f, 1.0f); break;
        }
        return;
    }

    if (id >= GF_P_LFO_BASE && id < GF_P_LFO_BASE + GF_NUM_LFOS * GF_P_LFO_STRIDE) {
        int rel = id - GF_P_LFO_BASE;
        GFLfoParams *l = &p->lfo[rel / GF_P_LFO_STRIDE];
        switch (rel % GF_P_LFO_STRIDE) {
            case 0: l->wave = (GFLfoWave)gf_clampf(val, 0.0f, 4.0f); break;
            case 1: l->target = (GFModTarget)gf_clampf(val, 0.0f, 5.0f); break;
            case 2: l->rate_hz = gf_clampf(val, 0.01f, 40.0f); break;
            case 3: l->depth = gf_clampf(val, 0.0f, 4.0f); break;
        }
        return;
    }

    if (id >= GF_P_ENV_BASE && id < GF_P_ENV_BASE + GF_NUM_ENVS * GF_P_ENV_STRIDE) {
        int rel = id - GF_P_ENV_BASE;
        GFEnvDef *e = &s->env_defs[rel / GF_P_ENV_STRIDE];
        switch (rel % GF_P_ENV_STRIDE) {
            case 0: e->mode = (GFEnvMode)gf_clampf(val, 0.0f, 4.0f); break;
            case 1: e->time_scale = gf_clampf(val, 0.1f, 8.0f); break;
            case 2: e->lfo_rate_hz = gf_clampf(val, 0.1f, 30.0f); break;
            case 3: e->lfo_depth = gf_clampf(val, 0.0f, 0.8f); break;
        }
        return;
    }

    if (id >= GF_P_MACRO_BASE && id < GF_P_MACRO_BASE + GF_NUM_MACROS) {
        s->macros[id - GF_P_MACRO_BASE] = gf_clampf(val, 0.0f, 1.0f);
        return;
    }

    switch (id) {
        case GF_P_MASTER_GAIN:          p->master_gain = gf_clampf(val, 0.0f, 1.5f); break;
        case GF_P_MASTER_FILTER_MODE:   p->master_filter.mode = (GFFilterMode)gf_clampf(val, 0.0f, 13.0f); break;
        case GF_P_MASTER_FILTER_CUTOFF: p->master_filter.cutoff_hz = gf_clampf(val, 20.0f, 18000.0f); break;
        case GF_P_MASTER_FILTER_RES:    p->master_filter.resonance = gf_clampf(val, 0.0f, 0.97f); break;
        case GF_P_MASTER_FILTER_MORPH:  p->master_filter.morph = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_MASTER_SHAPER_MODE:   p->master_shaper.mode = (GFShaperMode)gf_clampf(val, 0.0f, 5.0f); break;
        case GF_P_MASTER_SHAPER_DRIVE:  p->master_shaper.drive = gf_clampf(val, 1.0f, 20.0f); break;
        case GF_P_MASTER_SHAPER_MIX:    p->master_shaper.mix = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_PIPE_FREQ:            p->pipe.freq_hz = gf_clampf(val, 25.0f, 2500.0f); break;
        case GF_P_PIPE_FEEDBACK:        p->pipe.feedback = gf_clampf(val, 0.0f, 0.985f); break;
        case GF_P_PIPE_DAMPING:         p->pipe.damping = gf_clampf(val, 0.02f, 0.92f); break;
        case GF_P_PIPE_PICKUP:          p->pipe.pickup_pos = gf_clampf(val, 0.1f, 0.9f); break;
        case GF_P_PIPE_MIX:             p->pipe.wet_mix = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_MCOMB_FREQ:           p->multicomb.base_hz = gf_clampf(val, 35.0f, 1600.0f); break;
        case GF_P_MCOMB_CHORD:          p->multicomb.chord_type = (int)gf_clampf(val, 0.0f, 5.0f); break;
        case GF_P_MCOMB_FEEDBACK:       p->multicomb.feedback = gf_clampf(val, 0.0f, 0.975f); break;
        case GF_P_MCOMB_DAMPING:        p->multicomb.damping = gf_clampf(val, 0.05f, 0.90f); break;
        case GF_P_MCOMB_MIX:            p->multicomb.wet_mix = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_MTAP_TIME:            p->multitap.base_time_sec = gf_clampf(val, 0.04f, 1.2f); break;
        case GF_P_MTAP_FEEDBACK:        p->multitap.feedback = gf_clampf(val, 0.0f, 0.88f); break;
        case GF_P_MTAP_SPREAD:          p->multitap.spread = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_MTAP_MIX:             p->multitap.wet_mix = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_AETHER_TIME:          p->aether.time_sec = gf_clampf(val, 0.02f, 1.2f); break;
        case GF_P_AETHER_FEEDBACK:      p->aether.feedback = gf_clampf(val, 0.0f, 0.92f); break;
        case GF_P_AETHER_MOD:           p->aether.mod_depth = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_AETHER_DAMPING:       p->aether.damping = gf_clampf(val, 0.0f, 0.95f); break;
        case GF_P_AETHER_MIX:           p->aether.wet_mix = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_AETHER_PITCH:         p->aether.pitch_semi = gf_clampf(val, -12.0f, 24.0f); break;
        case GF_P_AETHER_REVERSE:       p->aether.reverse_prob = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_REVERB_DECAY:         p->reverb.decay = gf_clampf(val, 0.20f, 0.994f); break;
        case GF_P_REVERB_DAMPING:       p->reverb.damping = gf_clampf(val, 0.05f, 0.90f); break;
        case GF_P_REVERB_SHIMMER:       p->reverb.shimmer = gf_clampf(val, 0.0f, 0.65f); break;
        case GF_P_REVERB_SIZE:          p->reverb.size = gf_clampf(val, 0.35f, 1.55f); break;
        case GF_P_REVERB_MIX:           p->reverb.wet_mix = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_REVERB_FREEZE:        p->reverb.freeze = (val >= 0.5f); break;
        case GF_P_XY_X:                 s->xy_x = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_XY_Y:                 s->xy_y = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_XY_Z:                 s->xy_z = gf_clampf(val, 0.0f, 1.0f); break;
        case GF_P_XY_ENABLED:           s->xy_enabled = (val >= 0.5f); break;
        case GF_P_TUNING_SCALE:         s->tuning_scale = (GFTuningScale)gf_clampf(val, 0.0f, 7.0f); break;
        case GF_P_TUNING_A4:            s->tuning_a4_hz = gf_clampf(val, 380.0f, 480.0f); break;
        case GF_P_VOICE_GLIDE:          s->voice_glide = gf_clampf(val, 0.0f, 0.999f); break;
        case GF_P_DRONE_LATCH: {
            bool next = (val >= 0.5f);
            if (s->drone_latch && !next) {
                /* Release latched notes when unlatched */
                for (int i = 0; i < GF_MAX_VOICES; ++i) {
                    if (s->voices[i].active && s->voices[i].latched) {
                        s->voices[i].latched = false;
                        s->voices[i].gate = false;
                        for (int e = 0; e < GF_NUM_ENVS; ++e) gf_env_release(&s->voices[i].env[e], &s->env_defs[e]);
                    }
                }
            }
            s->drone_latch = next;
            break;
        }
        case GF_P_SEQ_ENABLED:          s->sequencer.enabled = (val >= 0.5f); break;
        case GF_P_SEQ_BPM:              s->sequencer.bpm = gf_clampf(val, 30.0f, 260.0f); break;
        case GF_P_SEQ_STEPS:            s->sequencer.step_count = (int)gf_clampf(val, 1.0f, 16.0f); break;
        case GF_P_SEQ_SWING:            s->sequencer.swing = gf_clampf(val, 0.0f, 0.45f); break;
        case GF_P_SEQ_SLEW:             s->sequencer.slew = gf_clampf(val, 0.0f, 0.99f); break;
        default: break;
    }
}

bool gf_synth_note_on(GFSynth *s, int midi_note, float velocity) {
    if (!s || midi_note < 0 || midi_note > 127) return false;
    GFSynthCmd cmd = {GF_CMD_NOTE_ON, midi_note, gf_clampf(velocity, 0.01f, 1.0f), 0.0f, 0.0f};
    return gf_ringbuf_push(&s->cmd_queue, &cmd);
}

bool gf_synth_note_off(GFSynth *s, int midi_note) {
    if (!s || midi_note < 0 || midi_note > 127) return false;
    GFSynthCmd cmd = {GF_CMD_NOTE_OFF, midi_note, 0.0f, 0.0f, 0.0f};
    return gf_ringbuf_push(&s->cmd_queue, &cmd);
}

bool gf_synth_all_notes_off(GFSynth *s) {
    if (!s) return false;
    GFSynthCmd cmd = {GF_CMD_ALL_NOTES_OFF, 0, 0.0f, 0.0f, 0.0f};
    return gf_ringbuf_push(&s->cmd_queue, &cmd);
}

bool gf_synth_set_param(GFSynth *s, GFParamId id, float value) {
    if (!s || id < 0 || id >= GF_P_COUNT) return false;
    GFSynthCmd cmd = {GF_CMD_SET_PARAM, (int32_t)id, value, 0.0f, 0.0f};
    return gf_ringbuf_push(&s->cmd_queue, &cmd);
}

bool gf_synth_set_xy(GFSynth *s, float x, float y) {
    if (!s) return false;
    return gf_synth_set_xyz(s, x, y, s->xy_z);
}

bool gf_synth_set_xyz(GFSynth *s, float x, float y, float z) {
    if (!s) return false;
    GFSynthCmd cmd = {GF_CMD_SET_XYZ, 0, gf_clampf(x, 0.0f, 1.0f), gf_clampf(y, 0.0f, 1.0f), gf_clampf(z, 0.0f, 1.0f)};
    return gf_ringbuf_push(&s->cmd_queue, &cmd);
}

bool gf_synth_set_macro(GFSynth *s, int macro_idx, float value) {
    if (!s || macro_idx < 0 || macro_idx >= GF_NUM_MACROS) return false;
    GFSynthCmd cmd = {GF_CMD_SET_MACRO, macro_idx, gf_clampf(value, 0.0f, 1.0f), 0.0f, 0.0f};
    return gf_ringbuf_push(&s->cmd_queue, &cmd);
}

float gf_synth_get_param(const GFSynth *s, GFParamId id) {
    if (!s || id < 0 || id >= GF_P_COUNT) return 0.0f;
    const GFPatchSnapshot *p = &s->base_patch;

    if (id >= GF_P_CH_BASE && id < GF_P_CH_BASE + GF_NUM_CHANNELS * GF_P_CH_STRIDE) {
        int rel = id - GF_P_CH_BASE;
        const GFChannelPatch *c = &p->ch[rel / GF_P_CH_STRIDE];
        switch (rel % GF_P_CH_STRIDE) {
            case 0:  return (float)c->osc.mode;
            case 1:  return c->osc.level;
            case 2:  return c->osc.pan;
            case 3:  return c->osc.detune_semi;
            case 4:  return c->osc.fine_cents;
            case 5:  return c->osc.morph;
            case 6:  return c->osc.mod_ratio;
            case 7:  return c->osc.fm_index;
            case 8:  return c->osc.fm_feedback;
            case 9:  return c->osc.fractal_depth;
            case 10: return c->osc.fractal_ratio;
            case 11: return c->osc.shift_hz;
            case 12: return c->osc.grain_size;
            case 13: return c->osc.grain_density;
            case 14: return c->osc.grain_pos;
            case 15: return c->osc.grain_spray;
            case 16: return c->osc.grain_reverse;
            case 17: return (float)c->osc.unison_voices;
            case 18: return c->osc.unison_detune;
            case 19: return c->osc.unison_spread;
            case 20: return (float)c->osc.sample_index;
            case 21: return (float)c->filter.mode;
            case 22: return c->filter.cutoff_hz;
            case 23: return c->filter.resonance;
        }
    }
    if (id >= GF_P_CH_EXT_BASE && id < GF_P_CH_EXT_BASE + GF_NUM_CHANNELS * GF_P_CH_EXT_STRIDE) {
        int rel = id - GF_P_CH_EXT_BASE;
        const GFChannelPatch *c = &p->ch[rel / GF_P_CH_EXT_STRIDE];
        switch (rel % GF_P_CH_EXT_STRIDE) {
            case 0: return c->filter.morph;
            case 1: return (float)c->shaper.mode;
            case 2: return c->shaper.drive;
            case 3: return c->shaper.mix;
        }
    }
    if (id >= GF_P_LFO_BASE && id < GF_P_LFO_BASE + GF_NUM_LFOS * GF_P_LFO_STRIDE) {
        int rel = id - GF_P_LFO_BASE;
        const GFLfoParams *l = &p->lfo[rel / GF_P_LFO_STRIDE];
        switch (rel % GF_P_LFO_STRIDE) {
            case 0: return (float)l->wave;
            case 1: return (float)l->target;
            case 2: return l->rate_hz;
            case 3: return l->depth;
        }
    }
    if (id >= GF_P_ENV_BASE && id < GF_P_ENV_BASE + GF_NUM_ENVS * GF_P_ENV_STRIDE) {
        int rel = id - GF_P_ENV_BASE;
        const GFEnvDef *e = &s->env_defs[rel / GF_P_ENV_STRIDE];
        switch (rel % GF_P_ENV_STRIDE) {
            case 0: return (float)e->mode;
            case 1: return e->time_scale;
            case 2: return e->lfo_rate_hz;
            case 3: return e->lfo_depth;
        }
    }
    if (id >= GF_P_MACRO_BASE && id < GF_P_MACRO_BASE + GF_NUM_MACROS) {
        return s->macros[id - GF_P_MACRO_BASE];
    }

    switch (id) {
        case GF_P_MASTER_GAIN:          return p->master_gain;
        case GF_P_MASTER_FILTER_MODE:   return (float)p->master_filter.mode;
        case GF_P_MASTER_FILTER_CUTOFF: return p->master_filter.cutoff_hz;
        case GF_P_MASTER_FILTER_RES:    return p->master_filter.resonance;
        case GF_P_MASTER_FILTER_MORPH:  return p->master_filter.morph;
        case GF_P_MASTER_SHAPER_MODE:   return (float)p->master_shaper.mode;
        case GF_P_MASTER_SHAPER_DRIVE:  return p->master_shaper.drive;
        case GF_P_MASTER_SHAPER_MIX:    return p->master_shaper.mix;
        case GF_P_PIPE_FREQ:            return p->pipe.freq_hz;
        case GF_P_PIPE_FEEDBACK:        return p->pipe.feedback;
        case GF_P_PIPE_DAMPING:         return p->pipe.damping;
        case GF_P_PIPE_PICKUP:          return p->pipe.pickup_pos;
        case GF_P_PIPE_MIX:             return p->pipe.wet_mix;
        case GF_P_MCOMB_FREQ:           return p->multicomb.base_hz;
        case GF_P_MCOMB_CHORD:          return (float)p->multicomb.chord_type;
        case GF_P_MCOMB_FEEDBACK:       return p->multicomb.feedback;
        case GF_P_MCOMB_DAMPING:        return p->multicomb.damping;
        case GF_P_MCOMB_MIX:            return p->multicomb.wet_mix;
        case GF_P_MTAP_TIME:            return p->multitap.base_time_sec;
        case GF_P_MTAP_FEEDBACK:        return p->multitap.feedback;
        case GF_P_MTAP_SPREAD:          return p->multitap.spread;
        case GF_P_MTAP_MIX:             return p->multitap.wet_mix;
        case GF_P_AETHER_TIME:          return p->aether.time_sec;
        case GF_P_AETHER_FEEDBACK:      return p->aether.feedback;
        case GF_P_AETHER_MOD:           return p->aether.mod_depth;
        case GF_P_AETHER_DAMPING:       return p->aether.damping;
        case GF_P_AETHER_MIX:           return p->aether.wet_mix;
        case GF_P_AETHER_PITCH:         return p->aether.pitch_semi;
        case GF_P_AETHER_REVERSE:       return p->aether.reverse_prob;
        case GF_P_REVERB_DECAY:         return p->reverb.decay;
        case GF_P_REVERB_DAMPING:       return p->reverb.damping;
        case GF_P_REVERB_SHIMMER:       return p->reverb.shimmer;
        case GF_P_REVERB_SIZE:          return p->reverb.size;
        case GF_P_REVERB_MIX:           return p->reverb.wet_mix;
        case GF_P_REVERB_FREEZE:        return p->reverb.freeze ? 1.0f : 0.0f;
        case GF_P_XY_X:                 return s->xy_x;
        case GF_P_XY_Y:                 return s->xy_y;
        case GF_P_XY_Z:                 return s->xy_z;
        case GF_P_XY_ENABLED:           return s->xy_enabled ? 1.0f : 0.0f;
        case GF_P_TUNING_SCALE:         return (float)s->tuning_scale;
        case GF_P_TUNING_A4:            return s->tuning_a4_hz;
        case GF_P_VOICE_GLIDE:          return s->voice_glide;
        case GF_P_DRONE_LATCH:          return s->drone_latch ? 1.0f : 0.0f;
        case GF_P_SEQ_ENABLED:          return s->sequencer.enabled ? 1.0f : 0.0f;
        case GF_P_SEQ_BPM:              return s->sequencer.bpm;
        case GF_P_SEQ_STEPS:            return (float)s->sequencer.step_count;
        case GF_P_SEQ_SWING:            return s->sequencer.swing;
        case GF_P_SEQ_SLEW:             return s->sequencer.slew;
        default:                        return 0.0f;
    }
}

typedef struct ParamNameEntry { GFParamId id; const char *name; } ParamNameEntry;
static const ParamNameEntry g_param_names[] = {
    {GF_P_MASTER_GAIN, "master.gain"},
    {GF_P_MASTER_FILTER_MODE, "master.filter.mode"},
    {GF_P_MASTER_FILTER_CUTOFF, "master.filter.cutoff"},
    {GF_P_MASTER_FILTER_RES, "master.filter.res"},
    {GF_P_MASTER_FILTER_MORPH, "master.filter.morph"},
    {GF_P_MASTER_SHAPER_MODE, "master.shaper.mode"},
    {GF_P_MASTER_SHAPER_DRIVE, "master.shaper.drive"},
    {GF_P_MASTER_SHAPER_MIX, "master.shaper.mix"},
    {GF_P_PIPE_FREQ, "pipe.freq"}, {GF_P_PIPE_FEEDBACK, "pipe.feedback"},
    {GF_P_PIPE_DAMPING, "pipe.damping"}, {GF_P_PIPE_PICKUP, "pipe.pickup"}, {GF_P_PIPE_MIX, "pipe.mix"},
    {GF_P_MCOMB_FREQ, "multicomb.freq"}, {GF_P_MCOMB_CHORD, "multicomb.chord"},
    {GF_P_MCOMB_FEEDBACK, "multicomb.feedback"}, {GF_P_MCOMB_DAMPING, "multicomb.damping"}, {GF_P_MCOMB_MIX, "multicomb.mix"},
    {GF_P_MTAP_TIME, "multitap.time"}, {GF_P_MTAP_FEEDBACK, "multitap.feedback"},
    {GF_P_MTAP_SPREAD, "multitap.spread"}, {GF_P_MTAP_MIX, "multitap.mix"},
    {GF_P_AETHER_TIME, "aether.time"}, {GF_P_AETHER_FEEDBACK, "aether.feedback"},
    {GF_P_AETHER_MOD, "aether.mod"}, {GF_P_AETHER_DAMPING, "aether.damping"},
    {GF_P_AETHER_MIX, "aether.mix"}, {GF_P_AETHER_PITCH, "aether.pitch"}, {GF_P_AETHER_REVERSE, "aether.reverse"},
    {GF_P_REVERB_DECAY, "reverb.decay"}, {GF_P_REVERB_DAMPING, "reverb.damping"},
    {GF_P_REVERB_SHIMMER, "reverb.shimmer"}, {GF_P_REVERB_SIZE, "reverb.size"},
    {GF_P_REVERB_MIX, "reverb.mix"}, {GF_P_REVERB_FREEZE, "reverb.freeze"},
    {GF_P_XY_X, "xy.x"}, {GF_P_XY_Y, "xy.y"}, {GF_P_XY_Z, "xy.z"}, {GF_P_XY_ENABLED, "xy.enabled"},
    {GF_P_TUNING_SCALE, "tuning.scale"}, {GF_P_TUNING_A4, "tuning.a4"},
    {GF_P_VOICE_GLIDE, "voice.glide"}, {GF_P_DRONE_LATCH, "voice.drone_latch"},
    {GF_P_SEQ_ENABLED, "seq.enabled"}, {GF_P_SEQ_BPM, "seq.bpm"},
    {GF_P_SEQ_STEPS, "seq.steps"}, {GF_P_SEQ_SWING, "seq.swing"}, {GF_P_SEQ_SLEW, "seq.slew"},
};

static const char *g_ch_suffixes[GF_P_CH_STRIDE] = {
    "mode", "level", "pan", "detune", "fine", "morph", "mod_ratio", "fm_index",
    "fm_feedback", "fractal_depth", "fractal_ratio", "shift_hz",
    "grain_size", "grain_density", "grain_pos", "grain_spray", "grain_reverse",
    "unison_voices", "unison_detune", "unison_spread", "sample",
    "filter.mode", "filter.cutoff", "filter.res"
};

static const char *g_ch_ext_suffixes[GF_P_CH_EXT_STRIDE] = {
    "filter.morph", "shaper.mode", "shaper.drive", "shaper.mix"
};

static const char *g_lfo_suffixes[GF_P_LFO_STRIDE] = {"wave", "target", "rate", "depth"};
static const char *g_env_suffixes[GF_P_ENV_STRIDE] = {"mode", "scale", "lfo_rate", "lfo_depth"};

int gf_synth_param_from_name(const char *name) {
    if (!name || name[0] == '\0') return -1;
    for (size_t i = 0; i < sizeof(g_param_names) / sizeof(g_param_names[0]); ++i) {
        if (strcmp(name, g_param_names[i].name) == 0) return (int)g_param_names[i].id;
    }
    if (strncmp(name, "ch", 2) == 0 && name[2] >= '1' && name[2] <= '3' && name[3] == '.') {
        int ch = name[2] - '1';
        const char *suf = name + 4;
        for (int k = 0; k < GF_P_CH_STRIDE; ++k) {
            if (strcmp(suf, g_ch_suffixes[k]) == 0) return GF_P_CH_BASE + ch * GF_P_CH_STRIDE + k;
        }
        for (int k = 0; k < GF_P_CH_EXT_STRIDE; ++k) {
            if (strcmp(suf, g_ch_ext_suffixes[k]) == 0) return GF_P_CH_EXT_BASE + ch * GF_P_CH_EXT_STRIDE + k;
        }
    }
    if (strncmp(name, "lfo", 3) == 0 && name[3] >= '1' && name[3] <= '3' && name[4] == '.') {
        int idx = name[3] - '1';
        const char *suf = name + 5;
        for (int k = 0; k < GF_P_LFO_STRIDE; ++k) {
            if (strcmp(suf, g_lfo_suffixes[k]) == 0) return GF_P_LFO_BASE + idx * GF_P_LFO_STRIDE + k;
        }
    }
    if (strncmp(name, "env", 3) == 0) {
        int idx = -1;
        const char *dot = strchr(name + 3, '.');
        if (dot) {
            idx = atoi(name + 3) - 1;
            if (idx >= 0 && idx < GF_NUM_ENVS) {
                for (int k = 0; k < GF_P_ENV_STRIDE; ++k) {
                    if (strcmp(dot + 1, g_env_suffixes[k]) == 0) return GF_P_ENV_BASE + idx * GF_P_ENV_STRIDE + k;
                }
            }
        }
    }
    if (strncmp(name, "macro.", 6) == 0 && name[6] >= '1' && name[6] <= '8' && name[7] == '\0') {
        return GF_P_MACRO_BASE + (name[6] - '1');
    }
    return -1;
}

const char *gf_synth_param_name(GFParamId id) {
    static char scratch[48];
    for (size_t i = 0; i < sizeof(g_param_names) / sizeof(g_param_names[0]); ++i) {
        if (g_param_names[i].id == id) return g_param_names[i].name;
    }
    if (id >= GF_P_CH_BASE && id < GF_P_CH_BASE + GF_NUM_CHANNELS * GF_P_CH_STRIDE) {
        int rel = id - GF_P_CH_BASE;
        snprintf(scratch, sizeof(scratch), "ch%d.%s", (rel / GF_P_CH_STRIDE) + 1, g_ch_suffixes[rel % GF_P_CH_STRIDE]);
        return scratch;
    }
    if (id >= GF_P_CH_EXT_BASE && id < GF_P_CH_EXT_BASE + GF_NUM_CHANNELS * GF_P_CH_EXT_STRIDE) {
        int rel = id - GF_P_CH_EXT_BASE;
        snprintf(scratch, sizeof(scratch), "ch%d.%s", (rel / GF_P_CH_EXT_STRIDE) + 1, g_ch_ext_suffixes[rel % GF_P_CH_EXT_STRIDE]);
        return scratch;
    }
    if (id >= GF_P_LFO_BASE && id < GF_P_LFO_BASE + GF_NUM_LFOS * GF_P_LFO_STRIDE) {
        int rel = id - GF_P_LFO_BASE;
        snprintf(scratch, sizeof(scratch), "lfo%d.%s", (rel / GF_P_LFO_STRIDE) + 1, g_lfo_suffixes[rel % GF_P_LFO_STRIDE]);
        return scratch;
    }
    if (id >= GF_P_ENV_BASE && id < GF_P_ENV_BASE + GF_NUM_ENVS * GF_P_ENV_STRIDE) {
        int rel = id - GF_P_ENV_BASE;
        snprintf(scratch, sizeof(scratch), "env%d.%s", (rel / GF_P_ENV_STRIDE) + 1, g_env_suffixes[rel % GF_P_ENV_STRIDE]);
        return scratch;
    }
    if (id >= GF_P_MACRO_BASE && id < GF_P_MACRO_BASE + GF_NUM_MACROS) {
        snprintf(scratch, sizeof(scratch), "macro.%d", (id - GF_P_MACRO_BASE) + 1);
        return scratch;
    }
    return "";
}

void gf_synth_set_xy_corner(GFSynth *s, int corner_idx, const GFPatchSnapshot *snap) {
    if (!s || !snap || corner_idx < 0 || corner_idx >= 4) return;
    s->xy_corners[corner_idx] = *snap;
}

bool gf_synth_set_wavetable(GFSynth *s, int ch_idx, bool slot_b, const float *samples, int count) {
    if (!s || ch_idx < 0 || ch_idx >= GF_NUM_CHANNELS) return false;
    GFWavetable *dst = slot_b ? &s->wt_b[ch_idx] : &s->wt_a[ch_idx];
    return gf_wavetable_load_custom(dst, samples, count);
}

bool gf_synth_set_harmonics(GFSynth *s, int ch_idx, bool slot_b, const float *harms, int count) {
    if (!s || ch_idx < 0 || ch_idx >= GF_NUM_CHANNELS || !harms || count <= 0) return false;
    GFWavetable *dst = slot_b ? &s->wt_b[ch_idx] : &s->wt_a[ch_idx];
    gf_wavetable_from_harmonics(dst, harms, count);
    return true;
}

bool gf_synth_set_shaper_curve(GFSynth *s, int ch_idx, const float *pts, int count) {
    if (!s || !pts || count < 2) return false;
    if (ch_idx < 0) {
        gf_waveshaper_set_curve(&s->base_patch.master_shaper, pts, count);
        return true;
    }
    if (ch_idx >= GF_NUM_CHANNELS) return false;
    gf_waveshaper_set_curve(&s->base_patch.ch[ch_idx].shaper, pts, count);
    return true;
}

bool gf_synth_set_envelope(GFSynth *s, int env_slot, const GFEnvPoint *pts, int count,
                           int sustain_idx, int loop_start, int loop_end) {
    if (!s || env_slot < 0 || env_slot >= GF_NUM_ENVS) return false;
    return gf_env_set_points(&s->env_defs[env_slot], pts, count, sustain_idx, loop_start, loop_end);
}

bool gf_synth_set_seq_step(GFSynth *s, int step_idx, float pitch_semi, float mod_val, bool gate) {
    if (!s || step_idx < 0 || step_idx >= GF_SEQ_MAX_STEPS) return false;
    s->sequencer.steps[step_idx].pitch_semi = gf_clampf(pitch_semi, -24.0f, 24.0f);
    s->sequencer.steps[step_idx].mod_val = gf_clampf(mod_val, 0.0f, 1.0f);
    s->sequencer.steps[step_idx].gate = gate;
    return true;
}

bool gf_synth_load_sample(GFSynth *s, int slot, const GFAudioSample *src) {
    if (!s || !src || !src->samples || src->frame_count == 0 || slot < 0 || slot >= GF_NUM_SAMPLES) {
        return false;
    }
    gf_audio_sample_free(&s->samples[slot]);
    size_t count = (size_t)src->frame_count * (size_t)src->channels;
    float *copy = (float *)malloc(count * sizeof(float));
    if (!copy) return false;
    memcpy(copy, src->samples, count * sizeof(float));
    s->samples[slot].samples = copy;
    s->samples[slot].frame_count = src->frame_count;
    s->samples[slot].sample_rate = src->sample_rate;
    s->samples[slot].channels = src->channels;
    return true;
}

static float bilerp(float c00, float c10, float c01, float c11, float x, float y) {
    return gf_lerpf(gf_lerpf(c00, c10, x), gf_lerpf(c01, c11, x), y);
}

static GFPatchSnapshot compute_effective_patch(const GFSynth *s) {
    GFPatchSnapshot eff = s->base_patch;
    if (s->xy_enabled) {
        const GFPatchSnapshot *c = s->xy_corners;
        float x = s->xy_x, y = s->xy_y, z = s->xy_z;
        /* Blend base_patch with 4-corner XY morph proportionally to Z depth */
        float mcut = bilerp(c[0].master_filter.cutoff_hz, c[1].master_filter.cutoff_hz,
                            c[2].master_filter.cutoff_hz, c[3].master_filter.cutoff_hz, x, y);
        eff.master_filter.cutoff_hz = gf_lerpf(eff.master_filter.cutoff_hz, mcut, 0.45f * z);
        eff.reverb.wet_mix = gf_clampf(eff.reverb.wet_mix + (y - 0.25f) * 0.25f * z, 0.0f, 0.95f);
        eff.aether.wet_mix = gf_clampf(eff.aether.wet_mix + (x - 0.30f) * 0.25f * z, 0.0f, 0.95f);
        for (int i = 0; i < GF_NUM_CHANNELS; ++i) {
            eff.ch[i].osc.morph = gf_clampf(eff.ch[i].osc.morph + (x - 0.3f) * 0.5f * z, 0.0f, 1.0f);
            eff.ch[i].filter.morph = gf_clampf(eff.ch[i].filter.morph + (y - 0.25f) * 0.6f * z, 0.0f, 1.0f);
        }
    }
    eff.master_filter.cutoff_hz = gf_clampf(eff.master_filter.cutoff_hz * (1.0f + s->macros[0] * 1.4f), 20.0f, 18000.0f);
    eff.aether.wet_mix = gf_clampf(eff.aether.wet_mix + s->macros[1] * 0.4f, 0.0f, 1.0f);
    eff.reverb.wet_mix = gf_clampf(eff.reverb.wet_mix + s->macros[2] * 0.4f, 0.0f, 1.0f);
    eff.ch[0].osc.grain_pos = gf_clampf(eff.ch[0].osc.grain_pos + s->macros[3] * 0.5f, 0.0f, 0.96f);
    return eff;
}

static void handle_note_on(GFSynth *s, int note, float vel) {
    /* Re-trigger if same note is already latched */
    int chosen = -1;
    for (int i = 0; i < GF_MAX_VOICES; ++i) {
        if (s->voices[i].active && s->voices[i].latched && s->voices[i].midi_note == note) {
            chosen = i;
            break;
        }
    }
    if (chosen < 0) {
        for (int i = 0; i < GF_MAX_VOICES; ++i) {
            if (!s->voices[i].active) { chosen = i; break; }
        }
    }
    if (chosen < 0) {
        uint64_t oldest = UINT64_MAX;
        for (int i = 0; i < GF_MAX_VOICES; ++i) {
            if (s->voices[i].age_counter < oldest) {
                oldest = s->voices[i].age_counter;
                chosen = i;
            }
        }
    }

    float prev_freq = s->voices[chosen].active ? s->voices[chosen].current_freq : 0.0f;
    GFVoice *v = &s->voices[chosen];
    memset(v, 0, sizeof(*v));
    v->active = true;
    v->gate = true;
    v->latched = s->drone_latch;
    v->midi_note = note;
    v->velocity = vel;
    v->target_freq = gf_tuning_midi_to_hz(note, s->tuning_scale, s->tuning_a4_hz);
    v->current_freq = (prev_freq > 20.0f && s->voice_glide > 0.01f) ? prev_freq : v->target_freq;
    v->age_counter = ++s->note_seq;

    for (int c = 0; c < GF_NUM_CHANNELS; ++c) {
        gf_osc_reset(&v->osc[c], s->base_seed ^ (uint32_t)(v->age_counter * 131u + (uint32_t)c * 17u));
        gf_filter_reset(&v->ch_filter[c]);
    }
    for (int e = 0; e < GF_NUM_ENVS; ++e) {
        gf_env_trigger(&v->env[e], &s->env_defs[e]);
    }
    for (int l = 0; l < GF_NUM_LFOS; ++l) {
        gf_lfo_reset(&v->lfo[l], s->base_patch.lfo[l].phase_offset, s->base_seed + (uint32_t)l * 997u);
    }
}

static void handle_note_off(GFSynth *s, int note) {
    for (int i = 0; i < GF_MAX_VOICES; ++i) {
        if (s->voices[i].active && s->voices[i].gate && s->voices[i].midi_note == note) {
            if (s->drone_latch) {
                s->voices[i].latched = true;
                continue;
            }
            s->voices[i].gate = false;
            for (int e = 0; e < GF_NUM_ENVS; ++e) {
                gf_env_release(&s->voices[i].env[e], &s->env_defs[e]);
            }
        }
    }
}

static void drain_commands(GFSynth *s) {
    GFSynthCmd cmd;
    while (gf_ringbuf_pop(&s->cmd_queue, &cmd)) {
        switch (cmd.type) {
            case GF_CMD_NOTE_ON:
                handle_note_on(s, cmd.iarg, cmd.farg0);
                break;
            case GF_CMD_NOTE_OFF:
                handle_note_off(s, cmd.iarg);
                break;
            case GF_CMD_ALL_NOTES_OFF:
                for (int i = 0; i < GF_MAX_VOICES; ++i) {
                    if (s->voices[i].active) {
                        s->voices[i].gate = false;
                        s->voices[i].latched = false;
                        for (int e = 0; e < GF_NUM_ENVS; ++e) gf_env_release(&s->voices[i].env[e], &s->env_defs[e]);
                    }
                }
                break;
            case GF_CMD_SET_PARAM:
                apply_param_internal(s, (GFParamId)cmd.iarg, cmd.farg0);
                break;
            case GF_CMD_SET_XYZ:
                s->xy_x = cmd.farg0; s->xy_y = cmd.farg1; s->xy_z = cmd.farg2;
                break;
            case GF_CMD_SET_MACRO:
                if (cmd.iarg >= 0 && cmd.iarg < GF_NUM_MACROS) s->macros[cmd.iarg] = cmd.farg0;
                break;
        }
    }
}

void gf_synth_render_block(GFSynth *s, float *out_lr, int frames) {
    if (!s || !out_lr || frames <= 0) return;

    drain_commands(s);
    GFPatchSnapshot eff = compute_effective_patch(s);
    const GFPatchPlan *plan = gf_synth_active_plan(s);
    float dt = 1.0f / s->sample_rate;
    float glide_coeff = (s->voice_glide > 0.001f) ? (1.0f - s->voice_glide * 0.9992f) : 1.0f;

    int write_tel = 1 - atomic_load_explicit(&s->telemetry_idx, memory_order_relaxed);
    GFAudioTelemetry *tel = &s->telemetry_buf[write_tel];
    memset(tel, 0, sizeof(*tel));
    tel->cable_count = plan ? plan->cable_count : 0;

    float sum_sq_l = 0.0f, sum_sq_r = 0.0f, peak = 0.0f;

    for (int f = 0; f < frames; ++f) {
        float seq_pitch = 0.0f, seq_gate = 0.0f, seq_mod = 0.0f;
        gf_seq_step_sample(&s->sequencer, s->sample_rate, &seq_pitch, &seq_gate, &seq_mod);

        float voice_sum_l = 0.0f;
        float voice_sum_r = 0.0f;
        float voice_port_sums[GF_PORT_IN_START];
        memset(voice_port_sums, 0, sizeof(voice_port_sums));

        for (int vi = 0; vi < GF_MAX_VOICES; ++vi) {
            GFVoice *v = &s->voices[vi];
            if (!v->active) continue;

            v->current_freq += glide_coeff * (v->target_freq - v->current_freq);

            float bus[GF_PORT_COUNT];
            memset(bus, 0, sizeof(bus));

            /* Populate performance source ports */
            bus[GF_PORT_MIDI_PITCH_CV] = ((float)v->midi_note - 60.0f) / 12.0f;
            bus[GF_PORT_MIDI_GATE_OUT] = v->gate ? 1.0f : 0.0f;
            bus[GF_PORT_MIDI_VEL_OUT]  = v->velocity;
            bus[GF_PORT_XYZ_X_OUT]     = s->xy_x;
            bus[GF_PORT_XYZ_Y_OUT]     = s->xy_y;
            bus[GF_PORT_XYZ_Z_OUT]     = s->xy_z;
            for (int m = 0; m < GF_NUM_MACROS; ++m) bus[GF_PORT_MACRO_1_OUT + m] = s->macros[m];

            /* Inherit master-level feedback into voice feedback bus */
            v->feedback_bus[GF_PORT_AETHER_OUT]   = s->master_feedback_bus[GF_PORT_AETHER_OUT];
            v->feedback_bus[GF_PORT_REVERB_OUT]   = s->master_feedback_bus[GF_PORT_REVERB_OUT];
            v->feedback_bus[GF_PORT_FEEDBACK_OUT] = s->master_feedback_bus[GF_PORT_FEEDBACK_OUT];

            float ch_stereo_r[GF_NUM_CHANNELS] = {0, 0, 0};
            for (int step = 0; step < plan->exec_count; ++step) {
                GFModuleId mod = plan->exec_order[step];
                if (mod > GF_MOD_NODE_VOICE_MIX) continue;

                gf_patch_plan_route_module(plan, mod, bus, v->feedback_bus, tel->cable_levels);

                if (mod == GF_MOD_NODE_ENVS) {
                    for (int e = 0; e < GF_NUM_ENVS; ++e) {
                        float rmod = (e < 4) ? bus[GF_PORT_ENV_1_RATE_CV + e] : 0.0f;
                        float scrub = (e < 4 && gf_patch_plan_port_connected(plan, (GFPortId)(GF_PORT_ENV_1_SCRUB_IN + e)))
                            ? bus[GF_PORT_ENV_1_SCRUB_IN + e] : s->xy_x;
                        bus[GF_PORT_ENV_1_OUT + e] = gf_env_step(&v->env[e], &s->env_defs[e], dt, rmod, scrub);
                    }
                } else if (mod == GF_MOD_NODE_LFOS_SEQ) {
                    for (int l = 0; l < GF_NUM_LFOS; ++l) {
                        GFLfoParams lp = eff.lfo[l];
                        lp.rate_hz *= powf(2.0f, gf_clampf(bus[GF_PORT_LFO_1_RATE_CV + l], -3.0f, 3.0f));
                        bus[GF_PORT_LFO_1_OUT + l] = gf_lfo_step(&v->lfo[l], &lp, s->sample_rate);
                    }
                    bus[GF_PORT_SEQ_PITCH_CV] = seq_pitch;
                    bus[GF_PORT_SEQ_GATE_OUT] = seq_gate;
                    bus[GF_PORT_SEQ_MOD_CV]   = seq_mod;
                } else if (mod == GF_MOD_NODE_VCAS) {
                    for (int k = 0; k < 4; ++k) {
                        float cv = gf_patch_plan_port_connected(plan, (GFPortId)(GF_PORT_VCA_1_CV + k * 2))
                            ? bus[GF_PORT_VCA_1_CV + k * 2] : 1.0f;
                        bus[GF_PORT_VCA_1_OUT + k] = bus[GF_PORT_VCA_1_IN + k * 2] * cv;
                    }
                } else if (mod >= GF_MOD_NODE_OSC1 && mod <= GF_MOD_NODE_OSC3) {
                    int c = (int)(mod - GF_MOD_NODE_OSC1);
                    const GFChannelPatch *cp = &eff.ch[c];
                    GFPortId p_base = (GFPortId)(GF_PORT_OSC_1_PITCH_CV + c * 5);
                    float pitch_oct = bus[p_base + 0] + (s->sequencer.enabled ? seq_pitch : 0.0f);
                    float ofreq = v->current_freq * powf(2.0f, gf_clampf(pitch_oct, -4.0f, 4.0f));
                    int sidx = cp->osc.sample_index;
                    const GFAudioSample *samp = (sidx >= 0 && sidx < GF_NUM_SAMPLES && s->samples[sidx].samples)
                        ? &s->samples[sidx] : NULL;
                    bus[GF_PORT_OSC_1_OUT + c] = gf_osc_step(
                        &v->osc[c], &cp->osc, &s->wt_a[c], &s->wt_b[c], samp,
                        ofreq, bus[p_base + 1], bus[p_base + 2], bus[p_base + 3], bus[p_base + 4],
                        &ch_stereo_r[c], s->sample_rate);
                } else if (mod >= GF_MOD_NODE_FLT1 && mod <= GF_MOD_NODE_FLT3) {
                    int c = (int)(mod - GF_MOD_NODE_FLT1);
                    GFPortId f_base = (GFPortId)(GF_PORT_FLT_1_IN + c * 4);
                    bus[GF_PORT_FLT_1_OUT + c] = gf_filter_step(
                        &v->ch_filter[c], &eff.ch[c].filter,
                        bus[f_base + 0], bus[f_base + 1] * 2.5f, bus[f_base + 2], bus[f_base + 3],
                        s->sample_rate);
                } else if (mod >= GF_MOD_NODE_SHP1 && mod <= GF_MOD_NODE_SHP3) {
                    int c = (int)(mod - GF_MOD_NODE_SHP1);
                    GFPortId s_base = (GFPortId)(GF_PORT_SHP_1_IN + c * 2);
                    bus[GF_PORT_SHP_1_OUT + c] = gf_waveshaper_process(
                        &eff.ch[c].shaper, bus[s_base + 0], bus[s_base + 1]);
                } else if (mod == GF_MOD_NODE_VOICE_MIX) {
                    float amp_cv = gf_patch_plan_port_connected(plan, GF_PORT_VOICE_AMP_CV)
                        ? gf_clampf(bus[GF_PORT_VOICE_AMP_CV], 0.0f, 1.5f)
                        : v->env[0].value;
                    float pan = gf_clampf(bus[GF_PORT_VOICE_PAN_CV], -1.0f, 1.0f);
                    float gl = cosf((pan + 1.0f) * 0.25f * GF_PI);
                    float gr = sinf((pan + 1.0f) * 0.25f * GF_PI);
                    float gain = amp_cv * v->velocity * 0.45f;
                    voice_sum_l += bus[GF_PORT_VOICE_BUS_L] * gl * gain;
                    voice_sum_r += bus[GF_PORT_VOICE_BUS_R] * gr * gain;
                }
            }

            if (v->env[0].finished) {
                v->active = false;
                continue;
            }
            float v_scale = v->env[0].value * v->velocity * 0.45f;
            for (int p = GF_PORT_ENV_1_OUT; p < GF_PORT_VOICE_OUT_L; ++p) {
                voice_port_sums[p] += (p >= GF_PORT_OSC_1_OUT) ? (bus[p] * v_scale) : bus[p];
            }
            gf_patch_plan_commit_feedback(bus, v->feedback_bus);
        }

        /* Execute Stereo Master & Studio Acoustics FX Graph */
        float mbus[GF_PORT_COUNT];
        memset(mbus, 0, sizeof(mbus));
        memcpy(mbus, voice_port_sums, sizeof(voice_port_sums));
        mbus[GF_PORT_VOICE_OUT_L] = voice_sum_l;
        mbus[GF_PORT_VOICE_OUT_R] = voice_sum_r;
        mbus[GF_PORT_XYZ_X_OUT] = s->xy_x;
        mbus[GF_PORT_XYZ_Y_OUT] = s->xy_y;
        mbus[GF_PORT_XYZ_Z_OUT] = s->xy_z;
        for (int m = 0; m < GF_NUM_MACROS; ++m) mbus[GF_PORT_MACRO_1_OUT + m] = s->macros[m];

        float st_l = voice_sum_l, st_r = voice_sum_r;

        for (int step = 0; step < plan->exec_count; ++step) {
            GFModuleId mod = plan->exec_order[step];
            if (mod < GF_MOD_NODE_MFLT) continue;

            gf_patch_plan_route_module(plan, mod, mbus, s->master_feedback_bus, tel->cable_levels);

            if (mod == GF_MOD_NODE_MFLT) {
                if (gf_patch_plan_port_connected(plan, GF_PORT_MFLT_IN)) {
                    float in_delta = mbus[GF_PORT_MFLT_IN] - 0.5f * (voice_sum_l + voice_sum_r);
                    st_l = gf_filter_step(&s->master_flt_l, &eff.master_filter, voice_sum_l + in_delta,
                                          mbus[GF_PORT_MFLT_CUTOFF_CV] * 2.0f, mbus[GF_PORT_MFLT_RES_CV], 0.0f, s->sample_rate);
                    st_r = gf_filter_step(&s->master_flt_r, &eff.master_filter, voice_sum_r + in_delta,
                                          mbus[GF_PORT_MFLT_CUTOFF_CV] * 2.0f, mbus[GF_PORT_MFLT_RES_CV], 0.0f, s->sample_rate);
                    st_l = gf_waveshaper_process(&eff.master_shaper, st_l, 0.0f);
                    st_r = gf_waveshaper_process(&eff.master_shaper, st_r, 0.0f);
                }
                mbus[GF_PORT_MFLT_OUT] = 0.5f * (st_l + st_r);
            } else if (mod == GF_MOD_NODE_PIPE) {
                if (gf_patch_plan_port_connected(plan, GF_PORT_PIPE_IN)) {
                    float nl = 0.0f, nr = 0.0f;
                    gf_pipe_process(&s->pipe, &eff.pipe, st_l, st_r, mbus[GF_PORT_PIPE_FREQ_CV], &nl, &nr, s->sample_rate);
                    st_l = nl; st_r = nr;
                }
                mbus[GF_PORT_PIPE_OUT] = 0.5f * (st_l + st_r);
            } else if (mod == GF_MOD_NODE_MCOMB) {
                if (gf_patch_plan_port_connected(plan, GF_PORT_MCOMB_IN)) {
                    float nl = 0.0f, nr = 0.0f;
                    gf_multicomb_process(&s->multicomb, &eff.multicomb, st_l, st_r, mbus[GF_PORT_MCOMB_TUNE_CV], &nl, &nr, s->sample_rate);
                    st_l = nl; st_r = nr;
                }
                mbus[GF_PORT_MCOMB_OUT] = 0.5f * (st_l + st_r);
            } else if (mod == GF_MOD_NODE_MTAP) {
                if (gf_patch_plan_port_connected(plan, GF_PORT_MTAP_IN)) {
                    float nl = 0.0f, nr = 0.0f;
                    gf_multitap_process(&s->multitap, &eff.multitap, st_l, st_r, mbus[GF_PORT_MTAP_TIME_CV], &nl, &nr, s->sample_rate);
                    st_l = nl; st_r = nr;
                }
                mbus[GF_PORT_MTAP_OUT] = 0.5f * (st_l + st_r);
            } else if (mod == GF_MOD_NODE_AETHER) {
                if (gf_patch_plan_port_connected(plan, GF_PORT_AETHER_IN)) {
                    float nl = 0.0f, nr = 0.0f;
                    gf_aether_process(&s->aether, &eff.aether, st_l, st_r,
                                      mbus[GF_PORT_AETHER_PITCH_CV], mbus[GF_PORT_AETHER_MIX_CV],
                                      &nl, &nr, s->sample_rate);
                    st_l = nl; st_r = nr;
                }
                mbus[GF_PORT_AETHER_OUT] = 0.5f * (st_l + st_r);
            } else if (mod == GF_MOD_NODE_REVERB) {
                if (gf_patch_plan_port_connected(plan, GF_PORT_REVERB_IN)) {
                    float nl = 0.0f, nr = 0.0f;
                    gf_reverb_process(&s->reverb, &eff.reverb, st_l, st_r,
                                      mbus[GF_PORT_REVERB_DECAY_CV], mbus[GF_PORT_REVERB_FREEZE_GATE] > 0.5f,
                                      &nl, &nr, s->sample_rate);
                    st_l = nl; st_r = nr;
                }
                mbus[GF_PORT_REVERB_OUT] = 0.5f * (st_l + st_r);
            }
        }

        /* Master output combines stereo chain (when Reverb->Master is patched) plus any direct Master cables! */
        float direct_l = mbus[GF_PORT_MASTER_IN_L] - (gf_patch_plan_port_connected(plan, GF_PORT_REVERB_IN) ? mbus[GF_PORT_REVERB_OUT] : 0.0f);
        float direct_r = mbus[GF_PORT_MASTER_IN_R] - (gf_patch_plan_port_connected(plan, GF_PORT_REVERB_IN) ? mbus[GF_PORT_REVERB_OUT] : 0.0f);
        float final_l = 0.0f, final_r = 0.0f;
        if (gf_patch_plan_port_connected(plan, GF_PORT_MASTER_IN_L)) {
            final_l = (gf_patch_plan_port_connected(plan, GF_PORT_REVERB_IN) ? st_l : 0.0f) + direct_l;
        }
        if (gf_patch_plan_port_connected(plan, GF_PORT_MASTER_IN_R)) {
            final_r = (gf_patch_plan_port_connected(plan, GF_PORT_REVERB_IN) ? st_r : 0.0f) + direct_r;
        }
        mbus[GF_PORT_FEEDBACK_OUT] = 0.5f * (final_l + final_r);
        gf_patch_plan_commit_feedback(mbus, s->master_feedback_bus);

        float ol = tanhf(final_l * eff.master_gain);
        float or_ = tanhf(final_r * eff.master_gain);
        out_lr[f * 2 + 0] = ol;
        out_lr[f * 2 + 1] = or_;

        sum_sq_l += ol * ol;
        sum_sq_r += or_ * or_;
        float a = fabsf(ol) > fabsf(or_) ? fabsf(ol) : fabsf(or_);
        if (a > peak) peak = a;

        int scope_i = (f * GF_TELEMETRY_SCOPE) / frames;
        if (scope_i >= 0 && scope_i < GF_TELEMETRY_SCOPE) {
            tel->scope[scope_i] = 0.5f * (ol + or_);
        }
    }

    tel->rms_l = sqrtf(sum_sq_l / (float)frames);
    tel->rms_r = sqrtf(sum_sq_r / (float)frames);
    tel->peak = peak;
    tel->seq_step = s->sequencer.current_step;

    int active_cnt = 0;
    for (int vi = 0; vi < GF_MAX_VOICES; ++vi) {
        if (s->voices[vi].active) {
            active_cnt++;
            tel->voice_levels[vi] = s->voices[vi].env[0].value * s->voices[vi].velocity;
        }
    }
    tel->active_voices = active_cnt;

    int seg = GF_TELEMETRY_SCOPE / GF_TELEMETRY_BANDS;
    for (int b = 0; b < GF_TELEMETRY_BANDS; ++b) {
        float acc = 0.0f;
        for (int k = 0; k < seg; ++k) {
            float s0 = tel->scope[b * seg + k];
            float prev = (k > 0) ? tel->scope[b * seg + k - 1] : 0.0f;
            acc += (b < 4) ? fabsf(s0) : fabsf(s0 - prev);
        }
        tel->bands[b] = gf_clampf((acc / (float)seg) * 3.5f, 0.0f, 1.0f);
    }

    atomic_store_explicit(&s->telemetry_idx, write_tel, memory_order_release);
}

void gf_synth_get_telemetry(const GFSynth *s, GFAudioTelemetry *out) {
    if (!s || !out) return;
    int idx = atomic_load_explicit(&s->telemetry_idx, memory_order_acquire);
    *out = s->telemetry_buf[idx];
}
