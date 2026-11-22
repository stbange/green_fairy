#ifndef GF_DSP_PATCHBAY_H
#define GF_DSP_PATCHBAY_H

#include <stdbool.h>
#include <stdint.h>

#define GF_MAX_CABLES 128

typedef enum GFSignalKind {
    GF_SIG_AUDIO   = 0,
    GF_SIG_CV      = 1,
    GF_SIG_TRIGGER = 2
} GFSignalKind;

typedef enum GFModuleId {
    GF_MOD_NODE_SOURCES = 0,
    GF_MOD_NODE_ENVS,
    GF_MOD_NODE_LFOS_SEQ,
    GF_MOD_NODE_VCAS,
    GF_MOD_NODE_OSC1,
    GF_MOD_NODE_OSC2,
    GF_MOD_NODE_OSC3,
    GF_MOD_NODE_FLT1,
    GF_MOD_NODE_FLT2,
    GF_MOD_NODE_FLT3,
    GF_MOD_NODE_SHP1,
    GF_MOD_NODE_SHP2,
    GF_MOD_NODE_SHP3,
    GF_MOD_NODE_VOICE_MIX,
    GF_MOD_NODE_MFLT,
    GF_MOD_NODE_PIPE,
    GF_MOD_NODE_MCOMB,
    GF_MOD_NODE_MTAP,
    GF_MOD_NODE_AETHER,
    GF_MOD_NODE_REVERB,
    GF_MOD_NODE_MASTER,
    GF_MOD_NODE_COUNT
} GFModuleId;

typedef enum GFPortId {
    /* ---- Output Ports (Sources) ---- */
    GF_PORT_MIDI_PITCH_CV = 0,
    GF_PORT_MIDI_GATE_OUT,
    GF_PORT_MIDI_VEL_OUT,
    GF_PORT_MIDI_AT_OUT,
    GF_PORT_XYZ_X_OUT,
    GF_PORT_XYZ_Y_OUT,
    GF_PORT_XYZ_Z_OUT,
    GF_PORT_MACRO_1_OUT,
    GF_PORT_MACRO_2_OUT,
    GF_PORT_MACRO_3_OUT,
    GF_PORT_MACRO_4_OUT,
    GF_PORT_MACRO_5_OUT,
    GF_PORT_MACRO_6_OUT,
    GF_PORT_MACRO_7_OUT,
    GF_PORT_MACRO_8_OUT,

    GF_PORT_ENV_1_OUT,
    GF_PORT_ENV_2_OUT,
    GF_PORT_ENV_3_OUT,
    GF_PORT_ENV_4_OUT,
    GF_PORT_ENV_5_OUT,
    GF_PORT_ENV_6_OUT,
    GF_PORT_ENV_7_OUT,
    GF_PORT_ENV_8_OUT,
    GF_PORT_ENV_9_OUT,
    GF_PORT_ENV_10_OUT,
    GF_PORT_ENV_11_OUT,
    GF_PORT_ENV_12_OUT,

    GF_PORT_LFO_1_OUT,
    GF_PORT_LFO_2_OUT,
    GF_PORT_LFO_3_OUT,
    GF_PORT_SEQ_PITCH_CV,
    GF_PORT_SEQ_GATE_OUT,
    GF_PORT_SEQ_MOD_CV,

    GF_PORT_VCA_1_OUT,
    GF_PORT_VCA_2_OUT,
    GF_PORT_VCA_3_OUT,
    GF_PORT_VCA_4_OUT,

    GF_PORT_OSC_1_OUT,
    GF_PORT_OSC_2_OUT,
    GF_PORT_OSC_3_OUT,

    GF_PORT_FLT_1_OUT,
    GF_PORT_FLT_2_OUT,
    GF_PORT_FLT_3_OUT,

    GF_PORT_SHP_1_OUT,
    GF_PORT_SHP_2_OUT,
    GF_PORT_SHP_3_OUT,

    GF_PORT_VOICE_OUT_L,
    GF_PORT_VOICE_OUT_R,
    GF_PORT_MFLT_OUT,
    GF_PORT_PIPE_OUT,
    GF_PORT_MCOMB_OUT,
    GF_PORT_MTAP_OUT,
    GF_PORT_AETHER_OUT,
    GF_PORT_REVERB_OUT,
    GF_PORT_FEEDBACK_OUT,

    /* ---- Input Ports (Destinations) ---- */
    GF_PORT_IN_START,
    GF_PORT_OSC_1_PITCH_CV = GF_PORT_IN_START,
    GF_PORT_OSC_1_FM_IN,
    GF_PORT_OSC_1_MORPH_CV,
    GF_PORT_OSC_1_FRACTAL_CV,
    GF_PORT_OSC_1_GRAIN_CV,

    GF_PORT_OSC_2_PITCH_CV,
    GF_PORT_OSC_2_FM_IN,
    GF_PORT_OSC_2_MORPH_CV,
    GF_PORT_OSC_2_FRACTAL_CV,
    GF_PORT_OSC_2_GRAIN_CV,

    GF_PORT_OSC_3_PITCH_CV,
    GF_PORT_OSC_3_FM_IN,
    GF_PORT_OSC_3_MORPH_CV,
    GF_PORT_OSC_3_FRACTAL_CV,
    GF_PORT_OSC_3_GRAIN_CV,

    GF_PORT_FLT_1_IN,
    GF_PORT_FLT_1_CUTOFF_CV,
    GF_PORT_FLT_1_RES_CV,
    GF_PORT_FLT_1_MORPH_CV,

    GF_PORT_FLT_2_IN,
    GF_PORT_FLT_2_CUTOFF_CV,
    GF_PORT_FLT_2_RES_CV,
    GF_PORT_FLT_2_MORPH_CV,

    GF_PORT_FLT_3_IN,
    GF_PORT_FLT_3_CUTOFF_CV,
    GF_PORT_FLT_3_RES_CV,
    GF_PORT_FLT_3_MORPH_CV,

    GF_PORT_SHP_1_IN,
    GF_PORT_SHP_1_DRIVE_CV,
    GF_PORT_SHP_2_IN,
    GF_PORT_SHP_2_DRIVE_CV,
    GF_PORT_SHP_3_IN,
    GF_PORT_SHP_3_DRIVE_CV,

    GF_PORT_ENV_1_RATE_CV,
    GF_PORT_ENV_2_RATE_CV,
    GF_PORT_ENV_3_RATE_CV,
    GF_PORT_ENV_4_RATE_CV,
    GF_PORT_ENV_1_SCRUB_IN,
    GF_PORT_ENV_2_SCRUB_IN,
    GF_PORT_ENV_3_SCRUB_IN,
    GF_PORT_ENV_4_SCRUB_IN,

    GF_PORT_LFO_1_RATE_CV,
    GF_PORT_LFO_2_RATE_CV,
    GF_PORT_LFO_3_RATE_CV,

    GF_PORT_VCA_1_IN,
    GF_PORT_VCA_1_CV,
    GF_PORT_VCA_2_IN,
    GF_PORT_VCA_2_CV,
    GF_PORT_VCA_3_IN,
    GF_PORT_VCA_3_CV,
    GF_PORT_VCA_4_IN,
    GF_PORT_VCA_4_CV,

    GF_PORT_VOICE_AMP_CV,
    GF_PORT_VOICE_PAN_CV,
    GF_PORT_VOICE_BUS_L,
    GF_PORT_VOICE_BUS_R,

    GF_PORT_MFLT_IN,
    GF_PORT_MFLT_CUTOFF_CV,
    GF_PORT_MFLT_RES_CV,

    GF_PORT_PIPE_IN,
    GF_PORT_PIPE_FREQ_CV,

    GF_PORT_MCOMB_IN,
    GF_PORT_MCOMB_TUNE_CV,

    GF_PORT_MTAP_IN,
    GF_PORT_MTAP_TIME_CV,

    GF_PORT_AETHER_IN,
    GF_PORT_AETHER_PITCH_CV,
    GF_PORT_AETHER_MIX_CV,

    GF_PORT_REVERB_IN,
    GF_PORT_REVERB_DECAY_CV,
    GF_PORT_REVERB_FREEZE_GATE,

    GF_PORT_MASTER_IN_L,
    GF_PORT_MASTER_IN_R,

    GF_PORT_COUNT
} GFPortId;

typedef struct GFPortInfo {
    GFPortId id;
    const char *name;
    GFModuleId module;
    GFSignalKind kind;
    bool is_output;
    int room_id; /* Primary 3D room 1..7 */
} GFPortInfo;

typedef struct GFCable {
    int id;
    bool active;
    GFPortId src;
    GFPortId dst;
    float gain;        /* Bipolar attenuverter [-2.0, +2.0] */
    bool is_feedback;  /* Computed by topological sort */
} GFCable;

typedef struct GFPatchPlan {
    GFCable cables[GF_MAX_CABLES];
    int cable_count;
    GFModuleId exec_order[GF_MOD_NODE_COUNT];
    int exec_count;
    bool has_cycles;
    uint32_t in_port_mask[(GF_PORT_COUNT + 31) / 32];
} GFPatchPlan;

typedef struct GFPatchbay {
    GFCable cables[GF_MAX_CABLES];
    int next_cable_id;
} GFPatchbay;

const GFPortInfo *gf_port_info(GFPortId id);
int gf_port_from_name(const char *name);
bool gf_port_is_output(GFPortId id);
bool gf_port_is_input(GFPortId id);

void gf_patchbay_init(GFPatchbay *pb);
void gf_patchbay_clear(GFPatchbay *pb);
void gf_patchbay_wire_default_absynth(GFPatchbay *pb);

int gf_patchbay_connect(GFPatchbay *pb, GFPortId src, GFPortId dst, float gain);
bool gf_patchbay_disconnect(GFPatchbay *pb, int cable_id);
int gf_patchbay_disconnect_port(GFPatchbay *pb, GFPortId port);
bool gf_patchbay_set_gain(GFPatchbay *pb, int cable_id, float gain);
int gf_patchbay_find_cable(const GFPatchbay *pb, GFPortId src, GFPortId dst);

void gf_patchbay_compile(const GFPatchbay *pb, GFPatchPlan *out_plan);
bool gf_patch_plan_port_connected(const GFPatchPlan *plan, GFPortId in_port);

/* Evaluates all cables targeting `dst_module` before that module runs, accumulating per-cable peaks. */
void gf_patch_plan_route_module(const GFPatchPlan *plan, GFModuleId dst_module,
                                float *bus, const float *feedback_bus,
                                float *cable_peaks);

/* Updates 1-sample soft-saturated feedback buffer at end of sample frame. */
void gf_patch_plan_commit_feedback(const float *bus, float *feedback_bus);

#endif
