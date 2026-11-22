#include "dsp/patchbay.h"

#include "core/math3d.h"

#include <math.h>
#include <string.h>

static const GFPortInfo g_ports[GF_PORT_COUNT] = {
    /* Outputs */
    {GF_PORT_MIDI_PITCH_CV, "midi.pitch_cv", GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MIDI_GATE_OUT, "midi.gate_out", GF_MOD_NODE_SOURCES, GF_SIG_TRIGGER, true, 6},
    {GF_PORT_MIDI_VEL_OUT,  "midi.vel_out",  GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MIDI_AT_OUT,   "midi.at_out",   GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_XYZ_X_OUT,     "xyz.x_out",     GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_XYZ_Y_OUT,     "xyz.y_out",     GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_XYZ_Z_OUT,     "xyz.z_out",     GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MACRO_1_OUT,   "macro1.out",    GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MACRO_2_OUT,   "macro2.out",    GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MACRO_3_OUT,   "macro3.out",    GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MACRO_4_OUT,   "macro4.out",    GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MACRO_5_OUT,   "macro5.out",    GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MACRO_6_OUT,   "macro6.out",    GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MACRO_7_OUT,   "macro7.out",    GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},
    {GF_PORT_MACRO_8_OUT,   "macro8.out",    GF_MOD_NODE_SOURCES, GF_SIG_CV, true, 6},

    {GF_PORT_ENV_1_OUT,  "env1.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_2_OUT,  "env2.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_3_OUT,  "env3.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_4_OUT,  "env4.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_5_OUT,  "env5.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_6_OUT,  "env6.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_7_OUT,  "env7.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_8_OUT,  "env8.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_9_OUT,  "env9.out",  GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_10_OUT, "env10.out", GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_11_OUT, "env11.out", GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},
    {GF_PORT_ENV_12_OUT, "env12.out", GF_MOD_NODE_ENVS, GF_SIG_CV, true, 3},

    {GF_PORT_LFO_1_OUT,    "lfo1.out",     GF_MOD_NODE_LFOS_SEQ, GF_SIG_CV, true, 4},
    {GF_PORT_LFO_2_OUT,    "lfo2.out",     GF_MOD_NODE_LFOS_SEQ, GF_SIG_CV, true, 4},
    {GF_PORT_LFO_3_OUT,    "lfo3.out",     GF_MOD_NODE_LFOS_SEQ, GF_SIG_CV, true, 4},
    {GF_PORT_SEQ_PITCH_CV, "seq.pitch_cv", GF_MOD_NODE_LFOS_SEQ, GF_SIG_CV, true, 4},
    {GF_PORT_SEQ_GATE_OUT, "seq.gate_out", GF_MOD_NODE_LFOS_SEQ, GF_SIG_TRIGGER, true, 4},
    {GF_PORT_SEQ_MOD_CV,   "seq.mod_cv",   GF_MOD_NODE_LFOS_SEQ, GF_SIG_CV, true, 4},

    {GF_PORT_VCA_1_OUT, "vca1.out", GF_MOD_NODE_VCAS, GF_SIG_CV, true, 4},
    {GF_PORT_VCA_2_OUT, "vca2.out", GF_MOD_NODE_VCAS, GF_SIG_CV, true, 4},
    {GF_PORT_VCA_3_OUT, "vca3.out", GF_MOD_NODE_VCAS, GF_SIG_CV, true, 4},
    {GF_PORT_VCA_4_OUT, "vca4.out", GF_MOD_NODE_VCAS, GF_SIG_CV, true, 4},

    {GF_PORT_OSC_1_OUT, "osc1.audio_out", GF_MOD_NODE_OSC1, GF_SIG_AUDIO, true, 1},
    {GF_PORT_OSC_2_OUT, "osc2.audio_out", GF_MOD_NODE_OSC2, GF_SIG_AUDIO, true, 1},
    {GF_PORT_OSC_3_OUT, "osc3.audio_out", GF_MOD_NODE_OSC3, GF_SIG_AUDIO, true, 1},

    {GF_PORT_FLT_1_OUT, "flt1.out", GF_MOD_NODE_FLT1, GF_SIG_AUDIO, true, 2},
    {GF_PORT_FLT_2_OUT, "flt2.out", GF_MOD_NODE_FLT2, GF_SIG_AUDIO, true, 2},
    {GF_PORT_FLT_3_OUT, "flt3.out", GF_MOD_NODE_FLT3, GF_SIG_AUDIO, true, 2},

    {GF_PORT_SHP_1_OUT, "shp1.out", GF_MOD_NODE_SHP1, GF_SIG_AUDIO, true, 2},
    {GF_PORT_SHP_2_OUT, "shp2.out", GF_MOD_NODE_SHP2, GF_SIG_AUDIO, true, 2},
    {GF_PORT_SHP_3_OUT, "shp3.out", GF_MOD_NODE_SHP3, GF_SIG_AUDIO, true, 2},

    {GF_PORT_VOICE_OUT_L,  "voice.out_l",      GF_MOD_NODE_VOICE_MIX, GF_SIG_AUDIO, true, 5},
    {GF_PORT_VOICE_OUT_R,  "voice.out_r",      GF_MOD_NODE_VOICE_MIX, GF_SIG_AUDIO, true, 5},
    {GF_PORT_MFLT_OUT,     "mflt.out",         GF_MOD_NODE_MFLT,      GF_SIG_AUDIO, true, 2},
    {GF_PORT_PIPE_OUT,     "pipe.out",         GF_MOD_NODE_PIPE,      GF_SIG_AUDIO, true, 5},
    {GF_PORT_MCOMB_OUT,    "multicomb.out",    GF_MOD_NODE_MCOMB,     GF_SIG_AUDIO, true, 5},
    {GF_PORT_MTAP_OUT,     "multitap.out",     GF_MOD_NODE_MTAP,      GF_SIG_AUDIO, true, 5},
    {GF_PORT_AETHER_OUT,   "aether.out",       GF_MOD_NODE_AETHER,    GF_SIG_AUDIO, true, 5},
    {GF_PORT_REVERB_OUT,   "reverb.out",       GF_MOD_NODE_REVERB,    GF_SIG_AUDIO, true, 5},
    {GF_PORT_FEEDBACK_OUT, "feedback.bus_out", GF_MOD_NODE_MASTER,    GF_SIG_AUDIO, true, 5},

    /* Inputs */
    {GF_PORT_OSC_1_PITCH_CV,   "osc1.pitch_cv",     GF_MOD_NODE_OSC1, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_1_FM_IN,      "osc1.fm_in",        GF_MOD_NODE_OSC1, GF_SIG_AUDIO, false, 1},
    {GF_PORT_OSC_1_MORPH_CV,   "osc1.morph_cv",     GF_MOD_NODE_OSC1, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_1_FRACTAL_CV, "osc1.fractal_cv",   GF_MOD_NODE_OSC1, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_1_GRAIN_CV,   "osc1.grain_pos_cv", GF_MOD_NODE_OSC1, GF_SIG_CV,    false, 1},

    {GF_PORT_OSC_2_PITCH_CV,   "osc2.pitch_cv",     GF_MOD_NODE_OSC2, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_2_FM_IN,      "osc2.fm_in",        GF_MOD_NODE_OSC2, GF_SIG_AUDIO, false, 1},
    {GF_PORT_OSC_2_MORPH_CV,   "osc2.morph_cv",     GF_MOD_NODE_OSC2, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_2_FRACTAL_CV, "osc2.fractal_cv",   GF_MOD_NODE_OSC2, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_2_GRAIN_CV,   "osc2.grain_pos_cv", GF_MOD_NODE_OSC2, GF_SIG_CV,    false, 1},

    {GF_PORT_OSC_3_PITCH_CV,   "osc3.pitch_cv",     GF_MOD_NODE_OSC3, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_3_FM_IN,      "osc3.fm_in",        GF_MOD_NODE_OSC3, GF_SIG_AUDIO, false, 1},
    {GF_PORT_OSC_3_MORPH_CV,   "osc3.morph_cv",     GF_MOD_NODE_OSC3, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_3_FRACTAL_CV, "osc3.fractal_cv",   GF_MOD_NODE_OSC3, GF_SIG_CV,    false, 1},
    {GF_PORT_OSC_3_GRAIN_CV,   "osc3.grain_pos_cv", GF_MOD_NODE_OSC3, GF_SIG_CV,    false, 1},

    {GF_PORT_FLT_1_IN,        "flt1.in",        GF_MOD_NODE_FLT1, GF_SIG_AUDIO, false, 2},
    {GF_PORT_FLT_1_CUTOFF_CV, "flt1.cutoff_cv", GF_MOD_NODE_FLT1, GF_SIG_CV,    false, 2},
    {GF_PORT_FLT_1_RES_CV,    "flt1.res_cv",    GF_MOD_NODE_FLT1, GF_SIG_CV,    false, 2},
    {GF_PORT_FLT_1_MORPH_CV,  "flt1.morph_cv",  GF_MOD_NODE_FLT1, GF_SIG_CV,    false, 2},

    {GF_PORT_FLT_2_IN,        "flt2.in",        GF_MOD_NODE_FLT2, GF_SIG_AUDIO, false, 2},
    {GF_PORT_FLT_2_CUTOFF_CV, "flt2.cutoff_cv", GF_MOD_NODE_FLT2, GF_SIG_CV,    false, 2},
    {GF_PORT_FLT_2_RES_CV,    "flt2.res_cv",    GF_MOD_NODE_FLT2, GF_SIG_CV,    false, 2},
    {GF_PORT_FLT_2_MORPH_CV,  "flt2.morph_cv",  GF_MOD_NODE_FLT2, GF_SIG_CV,    false, 2},

    {GF_PORT_FLT_3_IN,        "flt3.in",        GF_MOD_NODE_FLT3, GF_SIG_AUDIO, false, 2},
    {GF_PORT_FLT_3_CUTOFF_CV, "flt3.cutoff_cv", GF_MOD_NODE_FLT3, GF_SIG_CV,    false, 2},
    {GF_PORT_FLT_3_RES_CV,    "flt3.res_cv",    GF_MOD_NODE_FLT3, GF_SIG_CV,    false, 2},
    {GF_PORT_FLT_3_MORPH_CV,  "flt3.morph_cv",  GF_MOD_NODE_FLT3, GF_SIG_CV,    false, 2},

    {GF_PORT_SHP_1_IN,       "shp1.in",       GF_MOD_NODE_SHP1, GF_SIG_AUDIO, false, 2},
    {GF_PORT_SHP_1_DRIVE_CV, "shp1.drive_cv", GF_MOD_NODE_SHP1, GF_SIG_CV,    false, 2},
    {GF_PORT_SHP_2_IN,       "shp2.in",       GF_MOD_NODE_SHP2, GF_SIG_AUDIO, false, 2},
    {GF_PORT_SHP_2_DRIVE_CV, "shp2.drive_cv", GF_MOD_NODE_SHP2, GF_SIG_CV,    false, 2},
    {GF_PORT_SHP_3_IN,       "shp3.in",       GF_MOD_NODE_SHP3, GF_SIG_AUDIO, false, 2},
    {GF_PORT_SHP_3_DRIVE_CV, "shp3.drive_cv", GF_MOD_NODE_SHP3, GF_SIG_CV,    false, 2},

    {GF_PORT_ENV_1_RATE_CV,  "env1.rate_cv",  GF_MOD_NODE_ENVS, GF_SIG_CV, false, 3},
    {GF_PORT_ENV_2_RATE_CV,  "env2.rate_cv",  GF_MOD_NODE_ENVS, GF_SIG_CV, false, 3},
    {GF_PORT_ENV_3_RATE_CV,  "env3.rate_cv",  GF_MOD_NODE_ENVS, GF_SIG_CV, false, 3},
    {GF_PORT_ENV_4_RATE_CV,  "env4.rate_cv",  GF_MOD_NODE_ENVS, GF_SIG_CV, false, 3},
    {GF_PORT_ENV_1_SCRUB_IN, "env1.scrub_in", GF_MOD_NODE_ENVS, GF_SIG_CV, false, 3},
    {GF_PORT_ENV_2_SCRUB_IN, "env2.scrub_in", GF_MOD_NODE_ENVS, GF_SIG_CV, false, 3},
    {GF_PORT_ENV_3_SCRUB_IN, "env3.scrub_in", GF_MOD_NODE_ENVS, GF_SIG_CV, false, 3},
    {GF_PORT_ENV_4_SCRUB_IN, "env4.scrub_in", GF_MOD_NODE_ENVS, GF_SIG_CV, false, 3},

    {GF_PORT_LFO_1_RATE_CV, "lfo1.rate_cv", GF_MOD_NODE_LFOS_SEQ, GF_SIG_CV, false, 4},
    {GF_PORT_LFO_2_RATE_CV, "lfo2.rate_cv", GF_MOD_NODE_LFOS_SEQ, GF_SIG_CV, false, 4},
    {GF_PORT_LFO_3_RATE_CV, "lfo3.rate_cv", GF_MOD_NODE_LFOS_SEQ, GF_SIG_CV, false, 4},

    {GF_PORT_VCA_1_IN, "vca1.in", GF_MOD_NODE_VCAS, GF_SIG_CV, false, 4},
    {GF_PORT_VCA_1_CV, "vca1.cv", GF_MOD_NODE_VCAS, GF_SIG_CV, false, 4},
    {GF_PORT_VCA_2_IN, "vca2.in", GF_MOD_NODE_VCAS, GF_SIG_CV, false, 4},
    {GF_PORT_VCA_2_CV, "vca2.cv", GF_MOD_NODE_VCAS, GF_SIG_CV, false, 4},
    {GF_PORT_VCA_3_IN, "vca3.in", GF_MOD_NODE_VCAS, GF_SIG_CV, false, 4},
    {GF_PORT_VCA_3_CV, "vca3.cv", GF_MOD_NODE_VCAS, GF_SIG_CV, false, 4},
    {GF_PORT_VCA_4_IN, "vca4.in", GF_MOD_NODE_VCAS, GF_SIG_CV, false, 4},
    {GF_PORT_VCA_4_CV, "vca4.cv", GF_MOD_NODE_VCAS, GF_SIG_CV, false, 4},

    {GF_PORT_VOICE_AMP_CV, "voice.amp_cv", GF_MOD_NODE_VOICE_MIX, GF_SIG_CV,    false, 5},
    {GF_PORT_VOICE_PAN_CV, "voice.pan_cv", GF_MOD_NODE_VOICE_MIX, GF_SIG_CV,    false, 5},
    {GF_PORT_VOICE_BUS_L,  "voice.bus_l",  GF_MOD_NODE_VOICE_MIX, GF_SIG_AUDIO, false, 5},
    {GF_PORT_VOICE_BUS_R,  "voice.bus_r",  GF_MOD_NODE_VOICE_MIX, GF_SIG_AUDIO, false, 5},

    {GF_PORT_MFLT_IN,        "mflt.in",        GF_MOD_NODE_MFLT, GF_SIG_AUDIO, false, 2},
    {GF_PORT_MFLT_CUTOFF_CV, "mflt.cutoff_cv", GF_MOD_NODE_MFLT, GF_SIG_CV,    false, 2},
    {GF_PORT_MFLT_RES_CV,    "mflt.res_cv",    GF_MOD_NODE_MFLT, GF_SIG_CV,    false, 2},

    {GF_PORT_PIPE_IN,      "pipe.in",      GF_MOD_NODE_PIPE, GF_SIG_AUDIO, false, 5},
    {GF_PORT_PIPE_FREQ_CV, "pipe.freq_cv", GF_MOD_NODE_PIPE, GF_SIG_CV,    false, 5},

    {GF_PORT_MCOMB_IN,      "multicomb.in",      GF_MOD_NODE_MCOMB, GF_SIG_AUDIO, false, 5},
    {GF_PORT_MCOMB_TUNE_CV, "multicomb.tune_cv", GF_MOD_NODE_MCOMB, GF_SIG_CV,    false, 5},

    {GF_PORT_MTAP_IN,      "multitap.in",      GF_MOD_NODE_MTAP, GF_SIG_AUDIO, false, 5},
    {GF_PORT_MTAP_TIME_CV, "multitap.time_cv", GF_MOD_NODE_MTAP, GF_SIG_CV,    false, 5},

    {GF_PORT_AETHER_IN,       "aether.in",       GF_MOD_NODE_AETHER, GF_SIG_AUDIO, false, 5},
    {GF_PORT_AETHER_PITCH_CV, "aether.pitch_cv", GF_MOD_NODE_AETHER, GF_SIG_CV,    false, 5},
    {GF_PORT_AETHER_MIX_CV,   "aether.mix_cv",   GF_MOD_NODE_AETHER, GF_SIG_CV,    false, 5},

    {GF_PORT_REVERB_IN,          "reverb.in",          GF_MOD_NODE_REVERB, GF_SIG_AUDIO,   false, 5},
    {GF_PORT_REVERB_DECAY_CV,    "reverb.decay_cv",    GF_MOD_NODE_REVERB, GF_SIG_CV,      false, 5},
    {GF_PORT_REVERB_FREEZE_GATE, "reverb.freeze_gate", GF_MOD_NODE_REVERB, GF_SIG_TRIGGER, false, 5},

    {GF_PORT_MASTER_IN_L, "master.in_l", GF_MOD_NODE_MASTER, GF_SIG_AUDIO, false, 5},
    {GF_PORT_MASTER_IN_R, "master.in_r", GF_MOD_NODE_MASTER, GF_SIG_AUDIO, false, 5},
};

const GFPortInfo *gf_port_info(GFPortId id) {
    if ((int)id < 0 || id >= GF_PORT_COUNT) return NULL;
    return &g_ports[id];
}

int gf_port_from_name(const char *name) {
    if (!name || name[0] == '\0') return -1;
    for (int i = 0; i < GF_PORT_COUNT; ++i) {
        if (strcmp(g_ports[i].name, name) == 0) return i;
    }
    if (strcmp(name, "osc1.out") == 0) return GF_PORT_OSC_1_OUT;
    if (strcmp(name, "osc2.out") == 0) return GF_PORT_OSC_2_OUT;
    if (strcmp(name, "osc3.out") == 0) return GF_PORT_OSC_3_OUT;
    if (strcmp(name, "osc1.fm_cv") == 0) return GF_PORT_OSC_1_FM_IN;
    if (strcmp(name, "osc2.fm_cv") == 0) return GF_PORT_OSC_2_FM_IN;
    if (strcmp(name, "osc3.fm_cv") == 0) return GF_PORT_OSC_3_FM_IN;
    if (strcmp(name, "pipe.tension_cv") == 0) return GF_PORT_PIPE_FREQ_CV;
    return -1;
}

bool gf_port_is_output(GFPortId id) {
    return (int)id >= 0 && id < GF_PORT_IN_START;
}

bool gf_port_is_input(GFPortId id) {
    return id >= GF_PORT_IN_START && id < GF_PORT_COUNT;
}

void gf_patchbay_init(GFPatchbay *pb) {
    if (!pb) return;
    memset(pb, 0, sizeof(*pb));
    pb->next_cable_id = 1;
}

void gf_patchbay_clear(GFPatchbay *pb) {
    if (!pb) return;
    memset(pb->cables, 0, sizeof(pb->cables));
}

void gf_patchbay_wire_default_absynth(GFPatchbay *pb) {
    if (!pb) return;
    gf_patchbay_clear(pb);

    /* 3 Oscillator Channels -> Filters -> Waveshapers -> Stereo Voice Bus */
    gf_patchbay_connect(pb, GF_PORT_OSC_1_OUT, GF_PORT_FLT_1_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_FLT_1_OUT, GF_PORT_SHP_1_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_SHP_1_OUT, GF_PORT_VOICE_BUS_L, 0.85f);
    gf_patchbay_connect(pb, GF_PORT_SHP_1_OUT, GF_PORT_VOICE_BUS_R, 0.70f);

    gf_patchbay_connect(pb, GF_PORT_OSC_2_OUT, GF_PORT_FLT_2_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_FLT_2_OUT, GF_PORT_SHP_2_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_SHP_2_OUT, GF_PORT_VOICE_BUS_L, 0.70f);
    gf_patchbay_connect(pb, GF_PORT_SHP_2_OUT, GF_PORT_VOICE_BUS_R, 0.85f);

    gf_patchbay_connect(pb, GF_PORT_OSC_3_OUT, GF_PORT_FLT_3_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_FLT_3_OUT, GF_PORT_SHP_3_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_SHP_3_OUT, GF_PORT_VOICE_BUS_L, 0.80f);
    gf_patchbay_connect(pb, GF_PORT_SHP_3_OUT, GF_PORT_VOICE_BUS_R, 0.80f);

    /* Default Envelopes & LFO modulation cables */
    gf_patchbay_connect(pb, GF_PORT_ENV_1_OUT, GF_PORT_VOICE_AMP_CV, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_ENV_2_OUT, GF_PORT_FLT_1_CUTOFF_CV, 0.85f);
    gf_patchbay_connect(pb, GF_PORT_ENV_2_OUT, GF_PORT_FLT_2_CUTOFF_CV, 0.70f);
    gf_patchbay_connect(pb, GF_PORT_ENV_3_OUT, GF_PORT_OSC_2_FM_IN, 0.45f);
    gf_patchbay_connect(pb, GF_PORT_LFO_1_OUT, GF_PORT_FLT_1_CUTOFF_CV, 0.35f);
    gf_patchbay_connect(pb, GF_PORT_LFO_2_OUT, GF_PORT_OSC_1_MORPH_CV, 0.40f);
    gf_patchbay_connect(pb, GF_PORT_LFO_3_OUT, GF_PORT_VOICE_PAN_CV, 0.25f);

    /* Master Studio FX Chain: Voice -> Master Filter -> Pipe -> MultiComb -> MultiTap -> Aether -> Reverb -> Master */
    gf_patchbay_connect(pb, GF_PORT_VOICE_OUT_L, GF_PORT_MFLT_IN, 0.5f);
    gf_patchbay_connect(pb, GF_PORT_VOICE_OUT_R, GF_PORT_MFLT_IN, 0.5f);
    gf_patchbay_connect(pb, GF_PORT_MFLT_OUT,    GF_PORT_PIPE_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_PIPE_OUT,    GF_PORT_MCOMB_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_MCOMB_OUT,   GF_PORT_MTAP_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_MTAP_OUT,    GF_PORT_AETHER_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_AETHER_OUT,  GF_PORT_REVERB_IN, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_REVERB_OUT,  GF_PORT_MASTER_IN_L, 1.0f);
    gf_patchbay_connect(pb, GF_PORT_REVERB_OUT,  GF_PORT_MASTER_IN_R, 1.0f);
}

int gf_patchbay_find_cable(const GFPatchbay *pb, GFPortId src, GFPortId dst) {
    if (!pb) return -1;
    for (int i = 0; i < GF_MAX_CABLES; ++i) {
        if (pb->cables[i].active && pb->cables[i].src == src && pb->cables[i].dst == dst) {
            return pb->cables[i].id;
        }
    }
    return -1;
}

int gf_patchbay_connect(GFPatchbay *pb, GFPortId src, GFPortId dst, float gain) {
    if (!pb || !gf_port_is_output(src) || !gf_port_is_input(dst)) return -1;

    float clamped_gain = gf_clampf(gain, -2.0f, 2.0f);
    /* Update existing identical (src, dst) cable if already plugged */
    for (int i = 0; i < GF_MAX_CABLES; ++i) {
        if (pb->cables[i].active && pb->cables[i].src == src && pb->cables[i].dst == dst) {
            pb->cables[i].gain = clamped_gain;
            return pb->cables[i].id;
        }
    }

    for (int i = 0; i < GF_MAX_CABLES; ++i) {
        if (!pb->cables[i].active) {
            GFCable *c = &pb->cables[i];
            c->id = pb->next_cable_id++;
            c->active = true;
            c->src = src;
            c->dst = dst;
            c->gain = clamped_gain;
            c->is_feedback = false;
            return c->id;
        }
    }
    return -1;
}

bool gf_patchbay_disconnect(GFPatchbay *pb, int cable_id) {
    if (!pb || cable_id <= 0) return false;
    for (int i = 0; i < GF_MAX_CABLES; ++i) {
        if (pb->cables[i].active && pb->cables[i].id == cable_id) {
            pb->cables[i].active = false;
            return true;
        }
    }
    return false;
}

int gf_patchbay_disconnect_port(GFPatchbay *pb, GFPortId port) {
    if (!pb || (int)port < 0 || port >= GF_PORT_COUNT) return 0;
    int removed = 0;
    for (int i = 0; i < GF_MAX_CABLES; ++i) {
        if (pb->cables[i].active && (pb->cables[i].src == port || pb->cables[i].dst == port)) {
            pb->cables[i].active = false;
            removed++;
        }
    }
    return removed;
}

bool gf_patchbay_set_gain(GFPatchbay *pb, int cable_id, float gain) {
    if (!pb || cable_id <= 0) return false;
    for (int i = 0; i < GF_MAX_CABLES; ++i) {
        if (pb->cables[i].active && pb->cables[i].id == cable_id) {
            pb->cables[i].gain = gf_clampf(gain, -2.0f, 2.0f);
            return true;
        }
    }
    return false;
}

void gf_patchbay_compile(const GFPatchbay *pb, GFPatchPlan *out_plan) {
    if (!out_plan) return;
    memset(out_plan, 0, sizeof(*out_plan));
    if (!pb) return;

    int adj[GF_MOD_NODE_COUNT][GF_MOD_NODE_COUNT];
    int indeg[GF_MOD_NODE_COUNT];
    memset(adj, 0, sizeof(adj));
    memset(indeg, 0, sizeof(indeg));

    /* Collect active cables; self-module or master->voice back-edges are immediate feedback */
    for (int i = 0; i < GF_MAX_CABLES; ++i) {
        if (!pb->cables[i].active) continue;
        GFCable c = pb->cables[i];
        GFModuleId u = g_ports[c.src].module;
        GFModuleId v = g_ports[c.dst].module;

        /* Master-stage modules (>= MFLT) feeding per-voice modules (<= VOICE_MIX) cross voice/master boundary */
        if (u == v || c.src == GF_PORT_FEEDBACK_OUT || (u >= GF_MOD_NODE_MFLT && v <= GF_MOD_NODE_VOICE_MIX)) {
            c.is_feedback = true;
            out_plan->has_cycles = true;
        } else {
            c.is_feedback = false;
            if (!adj[u][v]) {
                adj[u][v] = 1;
                indeg[v]++;
            }
        }
        out_plan->cables[out_plan->cable_count++] = c;
        out_plan->in_port_mask[(uint32_t)c.dst / 32u] |= (1u << ((uint32_t)c.dst % 32u));
    }

    /* Kahn topological sort across module nodes */
    bool placed[GF_MOD_NODE_COUNT];
    int rank[GF_MOD_NODE_COUNT];
    memset(placed, 0, sizeof(placed));
    int count = 0;

    while (count < GF_MOD_NODE_COUNT) {
        int pick = -1;
        for (int m = 0; m < GF_MOD_NODE_COUNT; ++m) {
            if (!placed[m] && indeg[m] == 0) {
                pick = m;
                break;
            }
        }
        if (pick < 0) {
            /* Directed cycle detected! Break cycle deterministically at lowest-index unplaced module */
            out_plan->has_cycles = true;
            for (int m = 0; m < GF_MOD_NODE_COUNT; ++m) {
                if (!placed[m]) {
                    pick = m;
                    break;
                }
            }
        }
        placed[pick] = true;
        rank[pick] = count;
        out_plan->exec_order[count++] = (GFModuleId)pick;
        for (int nxt = 0; nxt < GF_MOD_NODE_COUNT; ++nxt) {
            if (adj[pick][nxt]) {
                adj[pick][nxt] = 0;
                if (indeg[nxt] > 0) indeg[nxt]--;
            }
        }
    }
    out_plan->exec_count = count;

    /* Any cable whose src module executes after dst module becomes a 1-sample feedback edge */
    for (int i = 0; i < out_plan->cable_count; ++i) {
        GFCable *c = &out_plan->cables[i];
        GFModuleId u = g_ports[c->src].module;
        GFModuleId v = g_ports[c->dst].module;
        if (rank[u] >= rank[v]) {
            c->is_feedback = true;
        }
    }
}

bool gf_patch_plan_port_connected(const GFPatchPlan *plan, GFPortId in_port) {
    if (!plan || !gf_port_is_input(in_port)) return false;
    return (plan->in_port_mask[(uint32_t)in_port / 32u] & (1u << ((uint32_t)in_port % 32u))) != 0u;
}

void gf_patch_plan_route_module(const GFPatchPlan *plan, GFModuleId dst_module,
                                float *bus, const float *feedback_bus,
                                float *cable_peaks) {
    if (!plan || !bus) return;
    for (int i = 0; i < plan->cable_count; ++i) {
        const GFCable *c = &plan->cables[i];
        if (g_ports[c->dst].module != dst_module) continue;
        float src_val = (c->is_feedback && feedback_bus) ? feedback_bus[c->src] : bus[c->src];
        float contrib = src_val * c->gain;
        bus[c->dst] += contrib;
        if (cable_peaks) {
            float a = fabsf(contrib);
            if (a > cable_peaks[i]) cable_peaks[i] = a;
        }
    }
}

void gf_patch_plan_commit_feedback(const float *bus, float *feedback_bus) {
    if (!bus || !feedback_bus) return;
    for (int p = 0; p < GF_PORT_IN_START; ++p) {
        float v = bus[p];
        feedback_bus[p] = isfinite(v) ? tanhf(v) : 0.0f;
    }
}
