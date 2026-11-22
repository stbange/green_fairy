#ifndef GF_DSP_ENVELOPE_H
#define GF_DSP_ENVELOPE_H

#include <stdbool.h>
#include <stdint.h>

#define GF_ENV_MAX_POINTS 68

typedef enum GFEnvMode {
    GF_ENV_MODE_SUSTAIN  = 0,
    GF_ENV_MODE_LOOP     = 1,
    GF_ENV_MODE_RHYTHMIC = 2, /* Tempo-synced rhythmic pulse loop */
    GF_ENV_MODE_ENV_LFO  = 3, /* Per-stage embedded LFO riding the envelope */
    GF_ENV_MODE_SCRUB    = 4  /* Scrubbed directly by CV / Macro / XY [0, 1] */
} GFEnvMode;

typedef struct GFEnvPoint {
    float time_sec; /* Monotonically non-decreasing timestamp in seconds */
    float level;    /* Target level [-1.0, 1.0] */
    float curve;    /* Segment curve: 0 = linear, >0 exp, <0 log */
    float lfo_rate; /* Optional per-stage embedded LFO Hz (0 = use def->lfo_rate_hz) */
    float lfo_depth;/* Optional per-stage embedded LFO depth */
} GFEnvPoint;

typedef struct GFEnvDef {
    GFEnvMode mode;
    GFEnvPoint points[GF_ENV_MAX_POINTS];
    int count;
    int sustain_index;
    int loop_start;
    int loop_end;
    float time_scale;   /* Rate multiplier [0.1, 8.0] */
    float lfo_rate_hz;  /* Embedded Env-LFO default rate [0.1, 30.0] */
    float lfo_depth;    /* Embedded Env-LFO default depth [0.0, 0.8] */
} GFEnvDef;

typedef struct GFEnvState {
    float time_sec;
    float value;
    float release_start_val;
    float lfo_phase;
    int stage;
    bool gate;
    bool active;
    bool releasing;
    bool finished;
} GFEnvState;

void gf_env_init_adsr(GFEnvDef *def, float attack, float decay, float sustain, float release);
bool gf_env_set_points(GFEnvDef *def, const GFEnvPoint *pts, int count,
                       int sustain_index, int loop_start, int loop_end);

/* Evaluates the 68-stage curve at normalized position u in [0, 1] (used by SCRUB & 3D display). */
float gf_env_eval_normalized(const GFEnvDef *def, float u01);

void gf_env_trigger(GFEnvState *st, const GFEnvDef *def);
void gf_env_release(GFEnvState *st, const GFEnvDef *def);
float gf_env_step(GFEnvState *st, const GFEnvDef *def, float dt, float rate_mod, float scrub_pos);

#endif
