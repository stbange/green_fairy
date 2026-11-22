#ifndef GF_DSP_SEQUENCER_H
#define GF_DSP_SEQUENCER_H

#include <stdbool.h>
#include <stdint.h>

#define GF_SEQ_MAX_STEPS 16

typedef enum GFTuningScale {
    GF_SCALE_12TET         = 0,
    GF_SCALE_JUST          = 1,
    GF_SCALE_PYTHAGOREAN   = 2,
    GF_SCALE_BOHLEN_PIERCE = 3, /* 13-step tritave (3:1) alien cosmic scale */
    GF_SCALE_QUARTER_TONE  = 4, /* 24-TET mystic microtonal */
    GF_SCALE_PELOG         = 5, /* Indonesian shimmering gamelan intervals */
    GF_SCALE_CARLOS_ALPHA  = 6, /* Wendy Carlos 78-cent non-octave scale */
    GF_SCALE_HARMONIC      = 7, /* Pure natural overtone series */
    GF_SCALE_COUNT         = 8
} GFTuningScale;

typedef struct GFSeqStep {
    float pitch_semi; /* [-24, +24] semitone offset */
    float mod_val;    /* [0.0, 1.0] step modulator CV */
    bool  gate;
} GFSeqStep;

typedef struct GFSequencer {
    bool enabled;
    float bpm;         /* [30, 260] */
    int step_count;    /* [1, 16] polyrhythmic length */
    float swing;       /* [0.0, 0.45] */
    float slew;        /* [0.0, 0.99] glide smoothing */
    GFSeqStep steps[GF_SEQ_MAX_STEPS];

    int current_step;
    float phase;
    float smooth_pitch;
    float smooth_mod;
    float gate_out;
} GFSequencer;

void gf_seq_init(GFSequencer *seq);
void gf_seq_reset_phase(GFSequencer *seq);
void gf_seq_step_sample(GFSequencer *seq, float sample_rate,
                        float *out_pitch_oct, float *out_gate, float *out_mod);

float gf_tuning_midi_to_hz(int midi_note, GFTuningScale scale, float root_a4_hz);
const char *gf_tuning_scale_name(GFTuningScale scale);

#endif
