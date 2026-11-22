#ifndef GF_DSP_SYNTH_H
#define GF_DSP_SYNTH_H

#include "assets/loaders.h"
#include "core/ringbuf.h"
#include "dsp/aetherizer.h"
#include "dsp/envelope.h"
#include "dsp/filter.h"
#include "dsp/fx_acoustics.h"
#include "dsp/lfo.h"
#include "dsp/oscillator.h"
#include "dsp/patchbay.h"
#include "dsp/sequencer.h"
#include "dsp/waveshaper.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define GF_MAX_VOICES      16
#define GF_NUM_CHANNELS    3
#define GF_NUM_ENVS        12
#define GF_NUM_LFOS        3
#define GF_NUM_MACROS      8
#define GF_NUM_SAMPLES     16
#define GF_CMD_QUEUE_CAP   512
#define GF_TELEMETRY_SCOPE 256
#define GF_TELEMETRY_BANDS 16

typedef enum GFParamId {
    GF_P_MASTER_GAIN = 0,
    GF_P_MASTER_FILTER_MODE,
    GF_P_MASTER_FILTER_CUTOFF,
    GF_P_MASTER_FILTER_RES,
    GF_P_MASTER_FILTER_MORPH,
    GF_P_MASTER_SHAPER_MODE,
    GF_P_MASTER_SHAPER_DRIVE,
    GF_P_MASTER_SHAPER_MIX,

    /* Studio Acoustics & Space FX (Room 5) */
    GF_P_PIPE_FREQ,
    GF_P_PIPE_FEEDBACK,
    GF_P_PIPE_DAMPING,
    GF_P_PIPE_PICKUP,
    GF_P_PIPE_MIX,

    GF_P_MCOMB_FREQ,
    GF_P_MCOMB_CHORD,
    GF_P_MCOMB_FEEDBACK,
    GF_P_MCOMB_DAMPING,
    GF_P_MCOMB_MIX,

    GF_P_MTAP_TIME,
    GF_P_MTAP_FEEDBACK,
    GF_P_MTAP_SPREAD,
    GF_P_MTAP_MIX,

    GF_P_AETHER_TIME,
    GF_P_AETHER_FEEDBACK,
    GF_P_AETHER_MOD,
    GF_P_AETHER_DAMPING,
    GF_P_AETHER_MIX,
    GF_P_AETHER_PITCH,
    GF_P_AETHER_REVERSE,

    GF_P_REVERB_DECAY,
    GF_P_REVERB_DAMPING,
    GF_P_REVERB_SHIMMER,
    GF_P_REVERB_SIZE,
    GF_P_REVERB_MIX,
    GF_P_REVERB_FREEZE,

    /* Performance, Tuning, Sequencer & XYZ Morph (Rooms 4 & 6) */
    GF_P_XY_X,
    GF_P_XY_Y,
    GF_P_XY_Z,
    GF_P_XY_ENABLED,
    GF_P_TUNING_SCALE,
    GF_P_TUNING_A4,
    GF_P_VOICE_GLIDE,
    GF_P_DRONE_LATCH,

    GF_P_SEQ_ENABLED,
    GF_P_SEQ_BPM,
    GF_P_SEQ_STEPS,
    GF_P_SEQ_SWING,
    GF_P_SEQ_SLEW,

    /* Per-channel parameters: ch 0..2, 24 params per channel */
    GF_P_CH_BASE = 56,
    GF_P_CH_STRIDE = 24,
    /* Offsets within a channel:
       0:mode, 1:level, 2:pan, 3:detune, 4:fine, 5:morph, 6:mod_ratio, 7:fm_index,
       8:fm_feedback, 9:fractal_depth, 10:fractal_ratio, 11:shift_hz,
       12:grain_size, 13:grain_density, 14:grain_pos, 15:grain_spray, 16:grain_reverse,
       17:unison_voices, 18:unison_detune, 19:unison_spread, 20:sample,
       21:filter.mode, 22:filter.cutoff, 23:filter.res */

    /* Extra channel filter morph + shaper: ch 0..2, 4 params per channel */
    GF_P_CH_EXT_BASE = 128,
    GF_P_CH_EXT_STRIDE = 4,
    /* Offsets: 0:filter.morph, 1:shaper.mode, 2:shaper.drive, 3:shaper.mix */

    /* LFOs 0..2: 4 params per LFO */
    GF_P_LFO_BASE = 140,
    GF_P_LFO_STRIDE = 4,

    /* Envelopes 0..11: 4 params per Env (mode, time_scale, lfo_rate, lfo_depth) */
    GF_P_ENV_BASE = 152,
    GF_P_ENV_STRIDE = 4,

    /* Macros 0..7 */
    GF_P_MACRO_BASE = 200,
    GF_P_COUNT = 208
} GFParamId;

typedef struct GFChannelPatch {
    GFOscParams osc;
    GFFilterParams filter;
    GFShaperParams shaper;
} GFChannelPatch;

typedef struct GFPatchSnapshot {
    float master_gain;
    GFFilterParams master_filter;
    GFShaperParams master_shaper;
    GFPipeParams pipe;
    GFMultiCombParams multicomb;
    GFMultiTapParams multitap;
    GFAetherParams aether;
    GFSpaceReverbParams reverb;
    GFChannelPatch ch[GF_NUM_CHANNELS];
    GFLfoParams lfo[GF_NUM_LFOS];
} GFPatchSnapshot;

typedef enum GFSynthCmdType {
    GF_CMD_NOTE_ON = 1,
    GF_CMD_NOTE_OFF = 2,
    GF_CMD_ALL_NOTES_OFF = 3,
    GF_CMD_SET_PARAM = 4,
    GF_CMD_SET_XYZ = 5,
    GF_CMD_SET_MACRO = 6
} GFSynthCmdType;

typedef struct GFSynthCmd {
    GFSynthCmdType type;
    int32_t iarg;
    float farg0;
    float farg1;
    float farg2;
} GFSynthCmd;

typedef struct GFVoice {
    bool active;
    bool gate;
    bool latched;
    int midi_note;
    float velocity;
    float current_freq;
    float target_freq;
    uint64_t age_counter;
    GFOscState osc[GF_NUM_CHANNELS];
    GFFilterState ch_filter[GF_NUM_CHANNELS];
    GFEnvState env[GF_NUM_ENVS];
    GFLfoState lfo[GF_NUM_LFOS];
    float feedback_bus[GF_PORT_COUNT];
} GFVoice;

typedef struct GFAudioTelemetry {
    float rms_l;
    float rms_r;
    float peak;
    int active_voices;
    float voice_levels[GF_MAX_VOICES];
    float bands[GF_TELEMETRY_BANDS];
    float scope[GF_TELEMETRY_SCOPE];
    int cable_count;
    float cable_levels[GF_MAX_CABLES];
    int seq_step;
} GFAudioTelemetry;

typedef struct GFSynth {
    float sample_rate;
    uint32_t base_seed;
    uint64_t note_seq;

    GFPatchSnapshot base_patch;
    GFPatchSnapshot xy_corners[4];
    bool xy_enabled;
    float xy_x;
    float xy_y;
    float xy_z;
    float macros[GF_NUM_MACROS];

    GFTuningScale tuning_scale;
    float tuning_a4_hz;
    float voice_glide;
    bool drone_latch;

    GFEnvDef env_defs[GF_NUM_ENVS];
    GFSequencer sequencer;

    GFWavetable wt_a[GF_NUM_CHANNELS];
    GFWavetable wt_b[GF_NUM_CHANNELS];
    GFAudioSample samples[GF_NUM_SAMPLES];

    GFPatchbay patchbay;
    GFPatchPlan plans[2];
    atomic_int active_plan_idx;

    GFVoice voices[GF_MAX_VOICES];
    GFFilterState master_flt_l;
    GFFilterState master_flt_r;
    GFPipeState pipe;
    GFMultiCombState multicomb;
    GFMultiTapState multitap;
    GFAetherizer aether;
    GFSpaceReverbState reverb;
    float master_feedback_bus[GF_PORT_COUNT];

    GFSynthCmd cmd_storage[GF_CMD_QUEUE_CAP];
    GFRingBuf cmd_queue;

    GFAudioTelemetry telemetry_buf[2];
    atomic_int telemetry_idx;
} GFSynth;

void gf_synth_init(GFSynth *s, float sample_rate, uint32_t seed);
void gf_synth_destroy(GFSynth *s);
void gf_synth_reset_default_patch(GFSynth *s);

/* Control-thread safe API */
bool gf_synth_note_on(GFSynth *s, int midi_note, float velocity);
bool gf_synth_note_off(GFSynth *s, int midi_note);
bool gf_synth_all_notes_off(GFSynth *s);
bool gf_synth_set_param(GFSynth *s, GFParamId id, float value);
float gf_synth_get_param(const GFSynth *s, GFParamId id);
bool gf_synth_set_xy(GFSynth *s, float x, float y);
bool gf_synth_set_xyz(GFSynth *s, float x, float y, float z);
bool gf_synth_set_macro(GFSynth *s, int macro_idx, float value);

int gf_synth_param_from_name(const char *name);
const char *gf_synth_param_name(GFParamId id);

/* Modular Patchbay API (compiles & atomically publishes updated GFPatchPlan) */
void gf_synth_clear_cables(GFSynth *s);
void gf_synth_wire_default_cables(GFSynth *s);
int gf_synth_connect_cable(GFSynth *s, GFPortId src, GFPortId dst, float gain);
bool gf_synth_disconnect_cable(GFSynth *s, int cable_id);
bool gf_synth_set_cable_gain(GFSynth *s, int cable_id, float gain);
const GFPatchPlan *gf_synth_active_plan(const GFSynth *s);

/* Patch / Wavetable /Harmonics / Envelope / Sequencer / Sample management */
void gf_synth_set_xy_corner(GFSynth *s, int corner_idx, const GFPatchSnapshot *snap);
bool gf_synth_set_wavetable(GFSynth *s, int ch_idx, bool slot_b, const float *samples, int count);
bool gf_synth_set_harmonics(GFSynth *s, int ch_idx, bool slot_b, const float *harms, int count);
bool gf_synth_set_shaper_curve(GFSynth *s, int ch_idx, const float *pts, int count);
bool gf_synth_set_envelope(GFSynth *s, int env_slot, const GFEnvPoint *pts, int count,
                           int sustain_idx, int loop_start, int loop_end);
bool gf_synth_set_seq_step(GFSynth *s, int step_idx, float pitch_semi, float mod_val, bool gate);
bool gf_synth_load_sample(GFSynth *s, int slot, const GFAudioSample *src);

/* Real-time safe / deterministic offline stereo render */
void gf_synth_render_block(GFSynth *s, float *out_lr, int frames);
void gf_synth_get_telemetry(const GFSynth *s, GFAudioTelemetry *out);

#endif
