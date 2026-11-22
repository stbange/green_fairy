#ifndef GF_DSP_OSCILLATOR_H
#define GF_DSP_OSCILLATOR_H

#include "assets/loaders.h"
#include "core/prng.h"

#include <stdbool.h>
#include <stdint.h>

#define GF_WAVETABLE_SIZE  2048
#define GF_WT_MIPS         8
#define GF_MAX_HARMONICS   64
#define GF_MAX_GRAINS      16
#define GF_UNISON_MAX      8

typedef enum GFOscMode {
    GF_OSC_OFF              = 0,
    GF_OSC_WAVETABLE_SINGLE = 1,
    GF_OSC_WAVETABLE_DOUBLE = 2,
    GF_OSC_FM               = 3,
    GF_OSC_RINGMOD          = 4,
    GF_OSC_SAMPLE           = 5,
    GF_OSC_GRANULAR         = 6,
    GF_OSC_FRACTALIZE       = 7,
    GF_OSC_FREQ_SHIFT       = 8,
    GF_OSC_SUPER_UNISON     = 9
} GFOscMode;

typedef struct GFWavetable {
    float samples[GF_WAVETABLE_SIZE];
    float mips[GF_WT_MIPS][GF_WAVETABLE_SIZE];
    float harmonics[GF_MAX_HARMONICS];
    bool has_mips;
} GFWavetable;

typedef struct GFGrain {
    bool active;
    float pos;
    float step;
    float age;
    float duration;
    float pan;
} GFGrain;

/* 4-section matched polyphase all-pass Hilbert quadrature filter */
typedef struct GFHilbertState {
    float i_x1[4], i_y1[4];
    float q_x1[4], q_y1[4];
    float delay_x;
} GFHilbertState;

typedef struct GFOscParams {
    GFOscMode mode;
    float level;          /* [0, 1] */
    float pan;            /* [-1, 1] */
    float detune_semi;    /* [-36, 36] */
    float fine_cents;     /* [-100, 100] */
    float morph;          /* [0, 1] wavetable blend / phase distortion */
    float mod_ratio;      /* [0.125, 16] FM / Ringmod / Sync ratio */
    float fm_index;       /* [0, 12] FM modulation depth / RM mix */
    float fm_feedback;    /* [0, 0.95] self-FM feedback */
    float fractal_depth;  /* [0, 1] recursive fractal mix */
    float fractal_ratio;  /* [1.5, 4.0] self-similarity contraction */
    float shift_hz;       /* [-2000, 2000] Hilbert single-sideband Hz shift */
    float grain_size;     /* [0.012, 0.35] seconds */
    float grain_density;  /* [2, 60] grains/sec */
    float grain_pos;      /* [0, 1] normalized sample scrub */
    float grain_spray;    /* [0, 0.5] random position/pitch cloud spray */
    float grain_reverse;  /* [0, 1] reverse grain probability */
    bool  grain_freeze;
    int   unison_voices;  /* [1, 8] */
    float unison_detune;  /* [0, 1] super-oscillator detune spread */
    float unison_spread;  /* [0, 1] stereo width */
    int   sample_index;
    bool  sample_loop;
} GFOscParams;

typedef struct GFOscState {
    float phase;
    float phase_b;
    float mod_phase;
    float shift_phase;
    float prev_out;
    float sample_pos;
    float grain_timer;
    float unison_phases[GF_UNISON_MAX];
    GFHilbertState hilbert;
    GFGrain grains[GF_MAX_GRAINS];
    GFPrng rng;
} GFOscState;

void gf_wavetable_build_mips(GFWavetable *wt);
void gf_wavetable_from_harmonics(GFWavetable *wt, const float *harm_amps, int count);
void gf_wavetable_gen_sine(GFWavetable *wt);
void gf_wavetable_gen_saw(GFWavetable *wt);
void gf_wavetable_gen_square(GFWavetable *wt);
void gf_wavetable_gen_emerald(GFWavetable *wt);
void gf_wavetable_gen_vocal(GFWavetable *wt);
bool gf_wavetable_load_custom(GFWavetable *wt, const float *samples, int count);
float gf_wavetable_lookup(const GFWavetable *wt, float phase01);
float gf_wavetable_lookup_bandlimited(const GFWavetable *wt, float phase01, float phase_inc);

void gf_osc_reset(GFOscState *st, uint32_t seed);
float gf_osc_step(GFOscState *st, const GFOscParams *p,
                  const GFWavetable *wt_a, const GFWavetable *wt_b,
                  const GFAudioSample *sample,
                  float base_freq_hz, float fm_ext_in, float morph_mod,
                  float fractal_mod, float grain_pos_mod,
                  float *out_stereo_r, float sample_rate);

#endif
