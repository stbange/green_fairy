#include "dsp/waveshaper.h"

#include "core/math3d.h"

#include <math.h>

void gf_waveshaper_set_curve(GFShaperParams *p, const float *pts, int count) {
    if (!p || !pts || count < 2) return;
    for (int i = 0; i < GF_TRANSFER_SIZE; ++i) {
        float t = (float)i * (float)(count - 1) / (float)(GF_TRANSFER_SIZE - 1);
        int i0 = (int)t;
        int i1 = (i0 + 1 < count) ? (i0 + 1) : i0;
        p->curve[i] = gf_clampf(gf_lerpf(pts[i0], pts[i1], t - (float)i0), -1.0f, 1.0f);
    }
    p->has_custom_curve = true;
}

float gf_waveshaper_process(const GFShaperParams *p, float input, float drive_mod) {
    if (!p || p->mode == GF_SHAPER_BYPASS) {
        return input;
    }

    float drive = gf_clampf(p->drive + drive_mod * 4.0f, 1.0f, 20.0f);
    float mix = gf_clampf(p->mix, 0.0f, 1.0f);
    float x = input * drive + p->phase_offset * 0.35f;
    float shaped = input;

    switch (p->mode) {
        case GF_SHAPER_SOFTCLIP:
            shaped = tanhf(x);
            break;
        case GF_SHAPER_FOLD:
            shaped = sinf(x * 1.5707963f);
            break;
        case GF_SHAPER_BITCRUSH: {
            float levels = gf_clampf(32.0f / drive, 2.0f, 32.0f);
            shaped = floorf(gf_clampf(x, -1.0f, 1.0f) * levels + 0.5f) / levels;
            break;
        }
        case GF_SHAPER_TUBE_ASYM: {
            float pos = (x >= 0.0f) ? tanhf(x * 1.3f) : (tanhf(x * 0.75f) * 0.85f);
            shaped = pos - tanhf(p->phase_offset * 0.35f);
            break;
        }
        case GF_SHAPER_WAVE_TRANSFER: {
            if (p->has_custom_curve) {
                float norm = gf_clampf(x * 0.5f + 0.5f, 0.0f, 1.0f) * (float)(GF_TRANSFER_SIZE - 1);
                int i0 = (int)norm;
                int i1 = (i0 + 1 < GF_TRANSFER_SIZE) ? (i0 + 1) : i0;
                shaped = gf_lerpf(p->curve[i0], p->curve[i1], norm - (float)i0);
            } else {
                /* Chebyshev T3 + T5 harmonic wave-transfer fallback */
                float c = gf_clampf(x, -1.0f, 1.0f);
                float t3 = 4.0f * c * c * c - 3.0f * c;
                shaped = 0.65f * tanhf(x) + 0.35f * t3;
            }
            break;
        }
        default:
            break;
    }

    return gf_lerpf(input, shaped, mix);
}
