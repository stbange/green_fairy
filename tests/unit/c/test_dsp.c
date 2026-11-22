#include "test_harness.h"

#include "assets/asset_manager.h"
#include "assets/loaders.h"
#include "core/math3d.h"
#include "dsp/aetherizer.h"
#include "dsp/envelope.h"
#include "dsp/filter.h"
#include "dsp/fx_acoustics.h"
#include "dsp/lfo.h"
#include "dsp/oscillator.h"
#include "dsp/patchbay.h"
#include "dsp/sequencer.h"
#include "dsp/synth.h"
#include "dsp/waveshaper.h"

static void test_envelope_all_5_modes_and_68_points(void) {
    GFEnvDef def;
    GFEnvState st;
    gf_env_init_adsr(NULL, 0.1f, 0.1f, 0.5f, 0.1f);
    gf_env_init_adsr(&def, 0.01f, 0.01f, 0.6f, 0.02f);
    gf_env_trigger(&st, &def);

    float v = 0.0f;
    for (int i = 0; i < 50; ++i) v = gf_env_step(&st, &def, 0.001f, 0.0f, 0.0f);
    GF_ASSERT_NEAR(v, 0.6f, 0.02);
    gf_env_release(&st, &def);
    for (int i = 0; i < 40; ++i) v = gf_env_step(&st, &def, 0.001f, 0.0f, 0.0f);
    GF_ASSERT(st.finished);

    /* Loop + Embedded Env-LFO + Rhythmic + Scrub modes */
    GFEnvPoint pts[6] = {
        {0.00f, 0.0f, 0.0f, 0.0f, 0.0f},
        {0.01f, 0.9f, 0.4f, 8.0f, 0.15f},
        {0.02f, 0.3f, -0.4f, 8.0f, 0.15f},
        {0.03f, 0.85f, 0.0f, 0.0f, 0.0f},
        {0.04f, 0.5f, 0.0f, 0.0f, 0.0f},
        {0.06f, 0.0f, 0.2f, 0.0f, 0.0f}
    };
    GF_ASSERT(gf_env_set_points(&def, pts, 6, 4, 1, 3));
    gf_env_trigger(&st, &def);
    for (int i = 0; i < 120; ++i) {
        v = gf_env_step(&st, &def, 0.001f, 0.0f, 0.0f);
        GF_ASSERT(isfinite(v));
    }

    /* Env-LFO sustain oscillation */
    gf_env_init_adsr(&def, 0.005f, 0.005f, 0.5f, 0.02f);
    def.mode = GF_ENV_MODE_ENV_LFO;
    def.lfo_rate_hz = 20.0f;
    def.lfo_depth = 0.25f;
    gf_env_trigger(&st, &def);
    float lo = 10.0f, hi = -10.0f;
    for (int i = 0; i < 120; ++i) {
        v = gf_env_step(&st, &def, 0.002f, 0.0f, 0.0f);
        if (i > 20) { if (v < lo) lo = v; if (v > hi) hi = v; }
    }
    GF_ASSERT(hi > 0.6f && lo < 0.4f);

    /* Rhythmic step mode */
    def.mode = GF_ENV_MODE_RHYTHMIC;
    gf_env_trigger(&st, &def);
    for (int i = 0; i < 80; ++i) GF_ASSERT(isfinite(gf_env_step(&st, &def, 0.002f, 0.2f, 0.0f)));

    /* Control-Scrub mode */
    def.mode = GF_ENV_MODE_SCRUB;
    gf_env_trigger(&st, &def);
    float s_mid = gf_env_step(&st, &def, 0.001f, 0.0f, 0.2f);
    GF_ASSERT(s_mid > 0.1f);
    GF_ASSERT_NEAR(gf_env_eval_normalized(NULL, 0.5f), 0.0f, 1e-6);

    /* 68-stage full capacity */
    GFEnvPoint p68[GF_ENV_MAX_POINTS];
    for (int i = 0; i < GF_ENV_MAX_POINTS; ++i) {
        p68[i] = (GFEnvPoint){(float)i * 0.001f, (i & 1) ? 0.8f : 0.2f, 0.0f, 0.0f, 0.0f};
    }
    GF_ASSERT(gf_env_set_points(&def, p68, GF_ENV_MAX_POINTS, -1, -1, -1));
    def.mode = GF_ENV_MODE_SUSTAIN;
    gf_env_trigger(&st, &def);
    gf_env_release(&st, &def);
    for (int i = 0; i < 10; ++i) gf_env_step(&st, &def, 0.001f, 0.0f, 0.0f);
    GF_ASSERT(st.finished);
}

static void test_lfo_sequencer_and_microtuning(void) {
    GFLfoState lst;
    GFLfoWave waves[5] = {GF_LFO_SINE, GF_LFO_TRI, GF_LFO_SAW, GF_LFO_SQUARE, GF_LFO_SAMPLE_HOLD};
    for (int w = 0; w < 5; ++w) {
        gf_lfo_reset(&lst, 0.0f, 42u + (uint32_t)w);
        GFLfoParams p = {waves[w], GF_MOD_FILTER_CUTOFF, 5.0f, 1.0f, 0.0f};
        for (int i = 0; i < 1200; ++i) GF_ASSERT(isfinite(gf_lfo_step(&lst, &p, 4000.0f)));
    }

    GFSequencer seq;
    gf_seq_init(NULL);
    gf_seq_init(&seq);
    float po = 0.0f, gt = 0.0f, md = 0.0f;
    gf_seq_step_sample(&seq, 44100.0f, &po, &gt, &md);
    GF_ASSERT_NEAR(po, 0.0f, 1e-6);

    seq.enabled = true;
    seq.bpm = 240.0f;
    seq.step_count = 5;
    for (int i = 0; i < 12000; ++i) {
        gf_seq_step_sample(&seq, 8000.0f, &po, &gt, &md);
        GF_ASSERT(isfinite(po) && isfinite(gt) && isfinite(md));
    }
    GF_ASSERT(seq.current_step >= 0 && seq.current_step < 5);
    gf_seq_reset_phase(&seq);
    GF_ASSERT(seq.current_step == 0);

    /* Exercise all 8 microtuning scales across positive & negative intervals */
    for (int sc = 0; sc <= (int)GF_SCALE_COUNT; ++sc) {
        float hz_low = gf_tuning_midi_to_hz(45, (GFTuningScale)sc, 440.0f);
        float hz_hi  = gf_tuning_midi_to_hz(75, (GFTuningScale)sc, 440.0f);
        GF_ASSERT(hz_low > 40.0f && hz_hi > hz_low);
        GF_ASSERT(strlen(gf_tuning_scale_name((GFTuningScale)sc)) > 2);
    }
}

static void test_all_9_oscillator_modes(void) {
    GFWavetable wt_a, wt_b, wt_v, custom;
    gf_wavetable_gen_sine(&wt_a);
    gf_wavetable_gen_saw(&wt_b);
    gf_wavetable_gen_square(&wt_v);
    gf_wavetable_gen_vocal(&wt_v);
    GF_ASSERT(wt_a.has_mips && wt_v.has_mips);

    float raw_wt[8] = {0, 1, 2, 1, 0, -1, -2, -1};
    GF_ASSERT(gf_wavetable_load_custom(&custom, raw_wt, 8));
    GF_ASSERT_NEAR(gf_wavetable_lookup(&custom, 2.0f / 7.0f), 1.0f, 0.03);
    GF_ASSERT(isfinite(gf_wavetable_lookup_bandlimited(&wt_b, 0.35f, 0.12f)));

    GFAssetManager am;
    GF_ASSERT(gf_assets_init(&am, "assets", NULL));
    uint8_t *wdata = NULL;
    size_t wsz = 0;
    GF_ASSERT(gf_assets_read_file(&am, "sounds/absinthe_bell.wav", &wdata, &wsz));
    GFAudioSample samp;
    GF_ASSERT(gf_load_wav_memory(wdata, wsz, &samp));
    free(wdata);

    for (int m = 1; m <= 9; ++m) {
        GFOscState st;
        gf_osc_reset(&st, 100u + (uint32_t)m);
        GFOscParams p = {
            .mode = (GFOscMode)m, .level = 0.85f, .pan = 0.0f, .detune_semi = 0.0f, .fine_cents = 3.0f,
            .morph = 0.45f, .mod_ratio = 2.0f, .fm_index = 1.6f, .fm_feedback = 0.6f,
            .fractal_depth = 0.65f, .fractal_ratio = 2.4f, .shift_hz = 135.0f,
            .grain_size = 0.035f, .grain_density = 30.0f, .grain_pos = 0.2f,
            .grain_spray = 0.1f, .grain_reverse = 0.35f, .grain_freeze = false,
            .unison_voices = 7, .unison_detune = 0.4f, .unison_spread = 0.8f,
            .sample_index = 0, .sample_loop = true
        };
        float energy = 0.0f;
        for (int i = 0; i < 3000; ++i) {
            float sr = 0.0f;
            float sl = gf_osc_step(&st, &p, &wt_a, &wt_b, &samp, 220.0f, 0.1f, 0.05f, 0.1f, 0.0f, &sr, 44100.0f);
            GF_ASSERT(isfinite(sl) && isfinite(sr));
            energy += sl * sl + sr * sr;
        }
        GF_ASSERT(energy > 0.2f);
    }

    gf_audio_sample_free(&samp);
}

static void test_all_14_filters_shapers_and_5_studio_fx(void) {
    /* Exercise all 14 filter modes */
    for (int m = 0; m <= 13; ++m) {
        GFFilterState fst;
        gf_filter_reset(&fst);
        GFFilterParams fp = {(GFFilterMode)m, 550.0f, 0.65f, 0.5f};
        float energy = 0.0f;
        for (int i = 0; i < 1024; ++i) {
            float x = sinf(GF_TAU * 220.0f * (float)i / 44100.0f) + 0.4f * sinf(GF_TAU * 1760.0f * (float)i / 44100.0f);
            float y = gf_filter_step(&fst, &fp, x, 0.1f, 0.05f, 0.1f, 44100.0f);
            GF_ASSERT(isfinite(y));
            energy += y * y;
        }
        GF_ASSERT(energy > 0.05f);
    }

    /* Exercise all 6 waveshaper modes + custom transfer curve */
    GFShaperParams sp;
    memset(&sp, 0, sizeof(sp));
    sp.drive = 3.5f; sp.mix = 0.85f; sp.phase_offset = 0.2f;
    for (int m = 0; m <= 5; ++m) {
        sp.mode = (GFShaperMode)m;
        GF_ASSERT(isfinite(gf_waveshaper_process(&sp, 0.45f, 0.2f)));
        GF_ASSERT(isfinite(gf_waveshaper_process(&sp, -0.45f, 0.0f)));
    }
    float curve_pts[5] = {-1.0f, -0.8f, 0.0f, 0.8f, 1.0f};
    gf_waveshaper_set_curve(&sp, curve_pts, 5);
    sp.mode = GF_SHAPER_WAVE_TRANSFER;
    GF_ASSERT(isfinite(gf_waveshaper_process(&sp, 0.3f, 0.0f)));

    /* Studio Acoustics: Pipe, Multi-Comb, Multi-Tap, Aetherizer Pro, 8x8 FDN Space Reverb */
    GFPipeState pipe; gf_pipe_reset(&pipe);
    GFPipeParams pp = {220.0f, 0.85f, 0.35f, 0.4f, 0.7f};

    GFMultiCombState mc; gf_multicomb_reset(&mc);
    GFMultiCombParams mcp = {110.0f, 2, 0.82f, 0.35f, 0.65f};

    GFMultiTapState mt; gf_multitap_reset(&mt);
    GFMultiTapParams mtp = {0.06f, 0.65f, 0.8f, 0.6f};

    GFAetherizer ae; gf_aether_reset(&ae);
    GFAetherParams aep = {0.05f, 0.7f, 0.4f, 0.3f, 0.65f, 12.0f, 0.3f};

    GFSpaceReverbState rev; gf_reverb_reset(&rev);
    GFSpaceReverbParams rvp = {0.88f, 0.3f, 0.35f, 0.8f, 0.7f, false};

    float tail = 0.0f;
    for (int i = 0; i < 6500; ++i) {
        float l = (i < 128) ? 0.8f * sinf(GF_TAU * 330.0f * (float)i / 44100.0f) : 0.0f;
        float r = l;
        gf_pipe_process(&pipe, &pp, l, r, 0.0f, &l, &r, 44100.0f);
        gf_multicomb_process(&mc, &mcp, l, r, 0.0f, &l, &r, 44100.0f);
        gf_multitap_process(&mt, &mtp, l, r, 0.0f, &l, &r, 44100.0f);
        gf_aether_process(&ae, &aep, l, r, 0.0f, 0.0f, &l, &r, 44100.0f);
        gf_reverb_process(&rev, &rvp, l, r, 0.0f, i > 6000, &l, &r, 44100.0f);
        GF_ASSERT(isfinite(l) && isfinite(r));
        if (i > 2500) tail += l * l + r * r;
    }
    GF_ASSERT(tail > 0.05f);
}

static void test_modular_synth_live_cable_rewiring_and_drone_latch(void) {
    GFSynth s1, s2;
    gf_synth_init(&s1, 44100.0f, 1337u);
    gf_synth_init(&s2, 44100.0f, 1337u);

    /* Parameter round-trip across all 208 parameter slots */
    float tmp[64];
    for (int id = 0; id < GF_P_COUNT; ++id) {
        const char *name = gf_synth_param_name((GFParamId)id);
        if (name && name[0] != '\0') {
            GF_ASSERT(gf_synth_param_from_name(name) == id);
            gf_synth_set_param(&s1, (GFParamId)id, gf_synth_get_param(&s1, (GFParamId)id));
        }
    }
    gf_synth_render_block(&s1, tmp, 16);

    /* Fresh deterministic check */
    gf_synth_init(&s1, 44100.0f, 1337u);
    gf_synth_init(&s2, 44100.0f, 1337u);
    float harms[8] = {1.0f, 0.5f, 0.25f, 0.1f, 0.4f, 0.0f, 0.2f, 0.1f};
    GF_ASSERT(gf_synth_set_harmonics(&s1, 0, false, harms, 8));
    GF_ASSERT(gf_synth_set_harmonics(&s2, 0, false, harms, 8));
    GF_ASSERT(gf_synth_set_shaper_curve(&s1, 0, harms, 4));
    GF_ASSERT(gf_synth_set_shaper_curve(&s2, 0, harms, 4));
    GF_ASSERT(gf_synth_set_shaper_curve(&s1, -1, harms, 4));
    GF_ASSERT(gf_synth_set_shaper_curve(&s2, -1, harms, 4));
    GF_ASSERT(gf_synth_set_seq_step(&s1, 0, 7.0f, 0.8f, true));
    GF_ASSERT(gf_synth_set_seq_step(&s2, 0, 7.0f, 0.8f, true));

    gf_synth_note_on(&s1, 60, 0.9f);
    gf_synth_note_on(&s2, 60, 0.9f);
    float b1[1024], b2[1024];
    gf_synth_render_block(&s1, b1, 512);
    gf_synth_render_block(&s2, b2, 512);
    gf_synth_render_block(&s1, b1, 512);
    gf_synth_render_block(&s2, b2, 512);
    float e_patched = 0.0f;
    for (int i = 0; i < 1024; ++i) {
        GF_ASSERT(b1[i] == b2[i]);
        e_patched += b1[i] * b1[i];
    }
    GF_ASSERT(e_patched > 0.05f);

    /* Unplug all cables -> output becomes completely silent! */
    gf_synth_clear_cables(&s1);
    gf_synth_render_block(&s1, b1, 512);
    float e_unplugged = 0.0f;
    for (int i = 0; i < 1024; ++i) e_unplugged += b1[i] * b1[i];
    GF_ASSERT_NEAR(e_unplugged, 0.0f, 1e-7);

    /* Re-patch Osc1 directly to Master L/R + VCA + Cross-FM cycle! */
    int cid = gf_synth_connect_cable(&s1, GF_PORT_OSC_1_OUT, GF_PORT_MASTER_IN_L, 0.9f);
    gf_synth_connect_cable(&s1, GF_PORT_OSC_1_OUT, GF_PORT_MASTER_IN_R, 0.9f);
    gf_synth_connect_cable(&s1, GF_PORT_LFO_1_OUT, GF_PORT_VCA_1_IN, 1.0f);
    gf_synth_connect_cable(&s1, GF_PORT_ENV_1_OUT, GF_PORT_VCA_1_CV, 1.0f);
    gf_synth_connect_cable(&s1, GF_PORT_VCA_1_OUT, GF_PORT_OSC_1_MORPH_CV, 0.5f);
    gf_synth_connect_cable(&s1, GF_PORT_FEEDBACK_OUT, GF_PORT_OSC_1_FM_IN, 0.4f);
    GF_ASSERT(gf_synth_set_cable_gain(&s1, cid, 0.8f));

    gf_synth_render_block(&s1, b1, 512);
    float e_repatched = 0.0f;
    for (int i = 0; i < 1024; ++i) e_repatched += b1[i] * b1[i];
    GF_ASSERT(e_repatched > 0.05f);
    GF_ASSERT(gf_synth_disconnect_cable(&s1, cid));

    /* Test Drone Hold Latch: notes keep sustaining after note_off until unlatched */
    gf_synth_wire_default_cables(&s1);
    gf_synth_set_param(&s1, GF_P_DRONE_LATCH, 1.0f);
    gf_synth_note_on(&s1, 64, 0.85f);
    gf_synth_note_off(&s1, 64);
    gf_synth_render_block(&s1, b1, 512);
    GFAudioTelemetry tel;
    gf_synth_get_telemetry(&s1, &tel);
    GF_ASSERT(tel.active_voices >= 1);
    GF_ASSERT(tel.cable_count >= 20);

    /* Unlatch releases latched voices */
    gf_synth_set_param(&s1, GF_P_DRONE_LATCH, 0.0f);
    gf_synth_all_notes_off(&s1);
    for (int k = 0; k < 85; ++k) gf_synth_render_block(&s1, b1, 512);
    gf_synth_get_telemetry(&s1, &tel);
    GF_ASSERT(tel.active_voices == 0);

    gf_synth_destroy(&s1);
    gf_synth_destroy(&s2);
}

int main(void) {
    GF_RUN_TEST(test_envelope_all_5_modes_and_68_points);
    GF_RUN_TEST(test_lfo_sequencer_and_microtuning);
    GF_RUN_TEST(test_all_9_oscillator_modes);
    GF_RUN_TEST(test_all_14_filters_shapers_and_5_studio_fx);
    GF_RUN_TEST(test_modular_synth_live_cable_rewiring_and_drone_latch);
    GF_TEST_SUMMARY();
}
