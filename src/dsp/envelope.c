#include "dsp/envelope.h"

#include "core/math3d.h"

#include <math.h>
#include <string.h>

static float apply_curve(float t, float curve) {
    t = gf_clampf(t, 0.0f, 1.0f);
    if (fabsf(curve) < 1e-4f) return t;
    float p = (curve > 0.0f) ? (1.0f + curve * 3.0f) : (1.0f / (1.0f - curve * 3.0f));
    return powf(t, p);
}

void gf_env_init_adsr(GFEnvDef *def, float attack, float decay, float sustain, float release) {
    if (!def) return;
    memset(def, 0, sizeof(*def));
    float a = attack < 0.001f ? 0.001f : attack;
    float d = decay < 0.001f ? 0.001f : decay;
    float r = release < 0.002f ? 0.002f : release;
    float s = gf_clampf(sustain, 0.0f, 1.0f);

    def->mode = GF_ENV_MODE_SUSTAIN;
    def->count = 4;
    def->points[0] = (GFEnvPoint){0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    def->points[1] = (GFEnvPoint){a, 1.0f, -0.25f, 0.0f, 0.0f};
    def->points[2] = (GFEnvPoint){a + d, s, 0.25f, 0.0f, 0.0f};
    def->points[3] = (GFEnvPoint){a + d + r, 0.0f, 0.35f, 0.0f, 0.0f};
    def->sustain_index = 2;
    def->loop_start = -1;
    def->loop_end = -1;
    def->time_scale = 1.0f;
    def->lfo_rate_hz = 4.5f;
    def->lfo_depth = 0.0f;
}

bool gf_env_set_points(GFEnvDef *def, const GFEnvPoint *pts, int count,
                       int sustain_index, int loop_start, int loop_end) {
    if (!def || !pts || count < 2 || count > GF_ENV_MAX_POINTS) {
        return false;
    }
    GFEnvMode keep_mode = def->mode;
    float keep_scale = def->time_scale > 0.01f ? def->time_scale : 1.0f;
    float keep_lfo_r = def->lfo_rate_hz > 0.01f ? def->lfo_rate_hz : 4.5f;
    float keep_lfo_d = def->lfo_depth;
    memset(def, 0, sizeof(*def));
    def->mode = keep_mode;
    def->time_scale = keep_scale;
    def->lfo_rate_hz = keep_lfo_r;
    def->lfo_depth = keep_lfo_d;

    float prev_t = 0.0f;
    for (int i = 0; i < count; ++i) {
        float t = pts[i].time_sec < prev_t ? prev_t : pts[i].time_sec;
        def->points[i].time_sec = t;
        def->points[i].level = gf_clampf(pts[i].level, -1.0f, 1.0f);
        def->points[i].curve = gf_clampf(pts[i].curve, -1.0f, 1.0f);
        def->points[i].lfo_rate = pts[i].lfo_rate;
        def->points[i].lfo_depth = pts[i].lfo_depth;
        prev_t = t;
    }
    def->count = count;
    def->sustain_index = (sustain_index >= 0 && sustain_index < count) ? sustain_index : -1;

    if (loop_start >= 0 && loop_end > loop_start && loop_end < count &&
        (def->sustain_index < 0 || loop_end <= def->sustain_index)) {
        def->loop_start = loop_start;
        def->loop_end = loop_end;
        if (def->mode == GF_ENV_MODE_SUSTAIN) def->mode = GF_ENV_MODE_LOOP;
    } else {
        def->loop_start = -1;
        def->loop_end = -1;
    }
    return true;
}

float gf_env_eval_normalized(const GFEnvDef *def, float u01) {
    if (!def || def->count < 2) return 0.0f;
    float total = def->points[def->count - 1].time_sec;
    if (total <= 1e-6f) return def->points[def->count - 1].level;
    float target_t = gf_clampf(u01, 0.0f, 1.0f) * total;
    int stg = 0;
    while (stg < def->count - 2 && target_t >= def->points[stg + 1].time_sec) {
        stg++;
    }
    const GFEnvPoint *p0 = &def->points[stg];
    const GFEnvPoint *p1 = &def->points[stg + 1];
    float dur = p1->time_sec - p0->time_sec;
    float frac = dur > 1e-6f ? ((target_t - p0->time_sec) / dur) : 1.0f;
    return gf_lerpf(p0->level, p1->level, apply_curve(frac, p1->curve));
}

void gf_env_trigger(GFEnvState *st, const GFEnvDef *def) {
    if (!st || !def || def->count < 2) return;
    memset(st, 0, sizeof(*st));
    st->time_sec = def->points[0].time_sec;
    st->value = def->points[0].level;
    st->stage = 0;
    st->gate = true;
    st->active = true;
}

void gf_env_release(GFEnvState *st, const GFEnvDef *def) {
    if (!st || !def || !st->active || st->releasing) return;
    st->gate = false;
    st->releasing = true;
    st->release_start_val = st->value;
    if (def->sustain_index >= 0 && def->sustain_index < def->count - 1) {
        st->stage = def->sustain_index;
        st->time_sec = def->points[st->stage].time_sec;
    } else {
        st->stage = def->count - 2;
        st->time_sec = def->points[st->stage].time_sec;
    }
}

float gf_env_step(GFEnvState *st, const GFEnvDef *def, float dt, float rate_mod, float scrub_pos) {
    if (!st || !def || def->count < 2 || !st->active) {
        return 0.0f;
    }

    if (def->mode == GF_ENV_MODE_SCRUB && !st->releasing) {
        st->value = gf_env_eval_normalized(def, scrub_pos);
        return st->value;
    }

    float scale = gf_clampf((def->time_scale > 0.01f ? def->time_scale : 1.0f) * powf(2.0f, rate_mod), 0.05f, 16.0f);
    if (def->mode == GF_ENV_MODE_RHYTHMIC) scale *= 1.5f;
    st->time_sec += dt * scale;

    if (st->gate && !st->releasing) {
        int ls = def->loop_start;
        int le = def->loop_end;
        if (def->mode == GF_ENV_MODE_RHYTHMIC && (ls < 0 || le <= ls)) {
            ls = 0;
            le = (def->sustain_index > 0) ? def->sustain_index : (def->count - 1);
        }
        if (ls >= 0 && le > ls && le < def->count) {
            float t_end = def->points[le].time_sec;
            float t_start = def->points[ls].time_sec;
            float dur = t_end - t_start;
            if (dur > 1e-5f && st->time_sec >= t_end) {
                while (st->time_sec >= t_end) st->time_sec -= dur;
                st->stage = ls;
            }
        } else if (def->sustain_index >= 0) {
            float t_sus = def->points[def->sustain_index].time_sec;
            if (st->time_sec >= t_sus) {
                st->time_sec = t_sus;
                st->stage = def->sustain_index;
                st->value = def->points[def->sustain_index].level;
                if (def->mode == GF_ENV_MODE_ENV_LFO || def->lfo_depth > 1e-4f) {
                    float lr = def->lfo_rate_hz > 0.05f ? def->lfo_rate_hz : 4.5f;
                    float ld = def->lfo_depth > 0.0f ? def->lfo_depth : 0.18f;
                    st->lfo_phase += lr * dt;
                    if (st->lfo_phase >= 1.0f) st->lfo_phase -= floorf(st->lfo_phase);
                    return gf_clampf(st->value + sinf(st->lfo_phase * GF_TAU) * ld, -1.0f, 1.0f);
                }
                return st->value;
            }
        }
    }

    while (st->stage < def->count - 1 && st->time_sec >= def->points[st->stage + 1].time_sec) {
        st->stage++;
    }

    if (st->stage >= def->count - 1) {
        st->value = def->points[def->count - 1].level;
        st->active = false;
        st->finished = true;
        return st->value;
    }

    const GFEnvPoint *p0 = &def->points[st->stage];
    const GFEnvPoint *p1 = &def->points[st->stage + 1];
    float seg_dur = p1->time_sec - p0->time_sec;
    float frac = (seg_dur > 1e-6f) ? ((st->time_sec - p0->time_sec) / seg_dur) : 1.0f;
    float shaped = apply_curve(frac, p1->curve);
    float start_lvl = (st->releasing && def->sustain_index >= 0 && st->stage == def->sustain_index)
        ? st->release_start_val
        : p0->level;
    st->value = gf_lerpf(start_lvl, p1->level, shaped);

    float ld = (p1->lfo_depth > 0.0f) ? p1->lfo_depth : ((def->mode == GF_ENV_MODE_ENV_LFO) ? (def->lfo_depth > 0.0f ? def->lfo_depth : 0.18f) : def->lfo_depth);
    if (ld > 1e-4f && !st->releasing) {
        float lr = (p1->lfo_rate > 0.05f) ? p1->lfo_rate : (def->lfo_rate_hz > 0.05f ? def->lfo_rate_hz : 4.5f);
        st->lfo_phase += lr * dt;
        if (st->lfo_phase >= 1.0f) st->lfo_phase -= floorf(st->lfo_phase);
        return gf_clampf(st->value + sinf(st->lfo_phase * GF_TAU) * ld, -1.0f, 1.0f);
    }
    return st->value;
}
