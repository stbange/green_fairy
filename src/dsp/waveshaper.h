#ifndef GF_DSP_WAVESHAPER_H
#define GF_DSP_WAVESHAPER_H

#include <stdbool.h>

#define GF_TRANSFER_SIZE 256

typedef enum GFShaperMode {
    GF_SHAPER_BYPASS   = 0,
    GF_SHAPER_SOFTCLIP = 1,
    GF_SHAPER_FOLD     = 2,
    GF_SHAPER_BITCRUSH = 3,
    GF_SHAPER_TUBE_ASYM = 4,
    GF_SHAPER_WAVE_TRANSFER = 5
} GFShaperMode;

typedef struct GFShaperParams {
    GFShaperMode mode;
    float drive;        /* [1.0, 20.0] */
    float mix;          /* [0.0, 1.0] */
    float phase_offset; /* [-1.0, 1.0] asymmetry / wave-transfer bias */
    float curve[GF_TRANSFER_SIZE];
    bool  has_custom_curve;
} GFShaperParams;

void gf_waveshaper_set_curve(GFShaperParams *p, const float *pts, int count);
float gf_waveshaper_process(const GFShaperParams *p, float input, float drive_mod);

#endif
