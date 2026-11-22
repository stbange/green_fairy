#ifndef GF_DSP_FX_ACOUSTICS_H
#define GF_DSP_FX_ACOUSTICS_H

#include <stdbool.h>
#include <stdint.h>

/* 1. Bidirectional Waveguide Acoustic Pipe */
#define GF_PIPE_MAX_LEN 4096
#define GF_PIPE_MASK    (GF_PIPE_MAX_LEN - 1)

typedef struct GFPipeParams {
    float freq_hz;    /* [30, 2000] fundamental bore resonance */
    float feedback;   /* [0.0, 0.985] */
    float damping;    /* [0.0, 0.92] end reflection LP */
    float pickup_pos; /* [0.1, 0.9] tap position along bore */
    float wet_mix;    /* [0.0, 1.0] */
} GFPipeParams;

typedef struct GFPipeState {
    float fwd[GF_PIPE_MAX_LEN];
    float rev[GF_PIPE_MAX_LEN];
    uint32_t pos;
    float lp_f, lp_r;
} GFPipeState;

/* 2. 6-Voice Chordal Multi-Comb Resonator */
#define GF_MCOMB_VOICES 6
#define GF_MCOMB_LEN    4096
#define GF_MCOMB_MASK   (GF_MCOMB_LEN - 1)

typedef struct GFMultiCombParams {
    float base_hz;    /* [40, 1200] root resonator frequency */
    int   chord_type; /* 0:Fifth/Oct, 1:Min9, 2:Lydian, 3:CosmicHarm, 4:Sus4, 5:Dim */
    float feedback;   /* [0.0, 0.98] */
    float damping;    /* [0.0, 0.9] */
    float wet_mix;    /* [0.0, 1.0] */
} GFMultiCombParams;

typedef struct GFMultiCombState {
    float buf[GF_MCOMB_VOICES][GF_MCOMB_LEN];
    float lp[GF_MCOMB_VOICES];
    uint32_t pos;
} GFMultiCombState;

/* 3. 6-Tap Stereo Multitap Delay */
#define GF_MTAP_BUF_LEN 65536
#define GF_MTAP_MASK    (GF_MTAP_BUF_LEN - 1)

typedef struct GFMultiTapParams {
    float base_time_sec; /* [0.04, 1.1] */
    float feedback;      /* [0.0, 0.88] */
    float spread;        /* [0.0, 1.0] stereo tap pan width */
    float wet_mix;       /* [0.0, 1.0] */
} GFMultiTapParams;

typedef struct GFMultiTapState {
    float buf_l[GF_MTAP_BUF_LEN];
    float buf_r[GF_MTAP_BUF_LEN];
    uint32_t pos;
} GFMultiTapState;

/* 4. 8x8 Hadamard FDN Space Cathedral Reverb */
#define GF_FDN_LINES 8
#define GF_FDN_LEN   16384
#define GF_FDN_MASK  (GF_FDN_LEN - 1)

typedef struct GFSpaceReverbParams {
    float decay;      /* [0.2, 0.996] up to 30s cosmic tail */
    float damping;    /* [0.05, 0.90] */
    float shimmer;    /* [0.0, 0.65] octave-up harmonic bloom */
    float size;       /* [0.3, 1.5] room scale */
    float wet_mix;    /* [0.0, 1.0] */
    bool  freeze;     /* Infinite frozen drone cathedral */
} GFSpaceReverbParams;

typedef struct GFSpaceReverbState {
    float buf[GF_FDN_LINES][GF_FDN_LEN];
    float damp[GF_FDN_LINES];
    uint32_t pos;
    float lfo_phase;
    float shim_phase;
} GFSpaceReverbState;

void gf_pipe_reset(GFPipeState *st);
void gf_pipe_process(GFPipeState *st, const GFPipeParams *p, float in_l, float in_r,
                     float freq_cv, float *out_l, float *out_r, float sample_rate);

void gf_multicomb_reset(GFMultiCombState *st);
void gf_multicomb_process(GFMultiCombState *st, const GFMultiCombParams *p, float in_l, float in_r,
                          float tune_cv, float *out_l, float *out_r, float sample_rate);

void gf_multitap_reset(GFMultiTapState *st);
void gf_multitap_process(GFMultiTapState *st, const GFMultiTapParams *p, float in_l, float in_r,
                         float time_cv, float *out_l, float *out_r, float sample_rate);

void gf_reverb_reset(GFSpaceReverbState *st);
void gf_reverb_process(GFSpaceReverbState *st, const GFSpaceReverbParams *p, float in_l, float in_r,
                       float decay_cv, bool freeze_gate, float *out_l, float *out_r, float sample_rate);

#endif
