#include "render/spatial_ui.h"

#include <stdio.h>
#include <string.h>

static GFMat4 mat4_from_basis(GFVec3 r, GFVec3 u, GFVec3 f, GFVec3 pos, GFVec3 scale) {
    GFMat4 m = gf_mat4_identity();
    m.m[0] = r.x * scale.x; m.m[1] = r.y * scale.x; m.m[2]  = r.z * scale.x;
    m.m[4] = u.x * scale.y; m.m[5] = u.y * scale.y; m.m[6]  = u.z * scale.y;
    m.m[8] = f.x * scale.z; m.m[9] = f.y * scale.z; m.m[10] = f.z * scale.z;
    m.m[12] = pos.x;        m.m[13] = pos.y;        m.m[14] = pos.z;
    return m;
}

void gf_spatial_init(GFSpatialUI *ui) {
    if (!ui) return;
    memset(ui, 0, sizeof(*ui));
    gf_spatial_clear(ui);
}

void gf_spatial_clear(GFSpatialUI *ui) {
    if (!ui) return;
    ui->rack_count = 0;
    ui->widget_count = 0;
    memset(ui->racks, 0, sizeof(ui->racks));
    memset(ui->widgets, 0, sizeof(ui->widgets));
    memset(ui->curves, 0, sizeof(ui->curves));
    for (int i = 0; i < GF_PORT_COUNT; ++i) {
        ui->primary_port_widget[i] = -1;
    }
    ui->hovered_widget_id = -1;
    ui->active_widget_id = -1;
    ui->tooltip_visible = false;
    ui->tooltip_text[0] = '\0';
}

void gf_rack_basis(const GFRack3D *rack, GFVec3 *out_r, GFVec3 *out_u, GFVec3 *out_f) {
    if (!rack) {
        if (out_r) *out_r = gf_vec3(1, 0, 0);
        if (out_u) *out_u = gf_vec3(0, 1, 0);
        if (out_f) *out_f = gf_vec3(0, 0, 1);
        return;
    }
    GFMat4 rot = gf_mat4_mul(gf_mat4_rotate_y(rack->rot_y), gf_mat4_rotate_x(rack->tilt_x));
    if (out_r) *out_r = gf_vec3_normalize(gf_vec3(rot.m[0], rot.m[1], rot.m[2]));
    if (out_u) *out_u = gf_vec3_normalize(gf_vec3(rot.m[4], rot.m[5], rot.m[6]));
    if (out_f) *out_f = gf_vec3_normalize(gf_vec3(rot.m[8], rot.m[9], rot.m[10]));
}

GFVec3 gf_rack_local_to_world(const GFRack3D *rack, GFVec3 local_pt) {
    if (!rack) return local_pt;
    GFVec3 r, u, f;
    gf_rack_basis(rack, &r, &u, &f);
    GFVec3 p = rack->pos;
    p = gf_vec3_add(p, gf_vec3_scale(r, local_pt.x));
    p = gf_vec3_add(p, gf_vec3_scale(u, local_pt.y));
    p = gf_vec3_add(p, gf_vec3_scale(f, local_pt.z));
    return p;
}

GFMat4 gf_rack_model_matrix(const GFRack3D *rack) {
    if (!rack) return gf_mat4_identity();
    GFVec3 r, u, f;
    gf_rack_basis(rack, &r, &u, &f);
    return mat4_from_basis(r, u, f, rack->pos, gf_vec3(rack->width, rack->height, rack->depth));
}

const GFRack3D *gf_spatial_get_rack(const GFSpatialUI *ui, int rack_id) {
    if (!ui || rack_id < 0 || rack_id >= ui->rack_count) return NULL;
    if (!ui->racks[rack_id].active) return NULL;
    return &ui->racks[rack_id];
}

int gf_spatial_add_rack(GFSpatialUI *ui, int room_id, const char *title,
                        GFVec3 pos, float rot_y, float tilt_x,
                        float width, float height, GFVec4 theme_color) {
    if (!ui || ui->rack_count >= GF_MAX_RACKS_3D || width <= 0.05f || height <= 0.05f) {
        return -1;
    }
    int id = ui->rack_count++;
    GFRack3D *rk = &ui->racks[id];
    memset(rk, 0, sizeof(*rk));
    rk->id = id;
    rk->active = true;
    rk->room_id = room_id;
    snprintf(rk->title, sizeof(rk->title), "%s", title ? title : "SYNTH RACK");
    rk->pos = pos;
    rk->rot_y = rot_y;
    rk->tilt_x = tilt_x;
    rk->width = width;
    rk->height = height;
    rk->depth = 1.0f;
    rk->theme_color = theme_color;

    /* Automatically mount clickable 3D Rack Header bar along top of rack for Rack-Focus zoom */
    GFWidgetDesc3D hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.room_id = room_id;
    hdr.rack_id = id;
    hdr.kind = GF_W3D_RACK_HEADER;
    hdr.local_pos = gf_vec3(0.0f, height * 0.455f, 0.015f);
    hdr.size = (GFVec2){width * 0.94f, height * 0.09f};
    hdr.param_id = -1;
    hdr.port_id = -1;
    hdr.aux_idx = id;
    hdr.color = theme_color;
    hdr.label = "";
    gf_spatial_add_widget(ui, &hdr);

    return id;
}

void gf_spatial_update_widget_bounds(GFSpatialUI *ui, GFWidget3D *w) {
    if (!ui || !w) return;
    const GFRack3D *rk = gf_spatial_get_rack(ui, w->rack_id);
    if (rk) {
        gf_rack_basis(rk, &w->axis_r, &w->axis_u, &w->axis_f);
        GFVec3 lp = w->local_pos;
        if (w->kind == GF_W3D_FADER && w->track_len > 0.01f) {
            lp.y += (w->norm_val - 0.5f) * w->track_len;
        } else if (w->kind == GF_W3D_HARMONIC_BAR && w->track_len > 0.01f) {
            float bar_h = 0.02f + w->norm_val * w->track_len;
            lp.y += bar_h * 0.5f;
        }
        w->world_pos = gf_rack_local_to_world(rk, lp);
    } else {
        w->axis_r = gf_vec3(1, 0, 0);
        w->axis_u = gf_vec3(0, 1, 0);
        w->axis_f = gf_vec3(0, 0, 1);
        w->world_pos = w->local_pos;
    }

    float sx = w->size.x > 0.01f ? w->size.x : 0.10f;
    float sy = w->size.y > 0.01f ? w->size.y : sx;
    switch (w->kind) {
        case GF_W3D_RACK_HEADER:
            w->half_extents = gf_vec3(sx * 0.5f, sy * 0.5f, 0.035f);
            break;
        case GF_W3D_FADER:
            w->half_extents = gf_vec3(sx * 0.55f, (w->track_len > sy ? w->track_len * 0.55f : sy * 0.6f), 0.055f);
            if (rk) w->world_pos = gf_rack_local_to_world(rk, w->local_pos);
            break;
        case GF_W3D_HARMONIC_BAR:
            w->half_extents = gf_vec3(sx * 0.55f, (w->track_len > 0.02f ? w->track_len * 0.55f : 0.15f), 0.045f);
            if (rk) {
                GFVec3 base = w->local_pos;
                base.y += (w->track_len > 0.02f ? w->track_len * 0.5f : 0.12f);
                w->world_pos = gf_rack_local_to_world(rk, base);
            }
            break;
        case GF_W3D_JACK_OUT:
        case GF_W3D_JACK_IN:
            w->half_extents = gf_vec3(sx * 0.58f, sy * 0.58f, 0.045f);
            break;
        case GF_W3D_XYZ_ORB:
            w->half_extents = gf_vec3(sx * 0.65f, sx * 0.65f, sx * 0.65f);
            break;
        case GF_W3D_PIANO_KEY:
            w->half_extents = gf_vec3(sx * 0.49f, sy * 0.50f, 0.022f);
            break;
        default:
            w->half_extents = gf_vec3(sx * 0.55f, sy * 0.55f, 0.055f);
            break;
    }
}

int gf_spatial_add_widget(GFSpatialUI *ui, const GFWidgetDesc3D *desc) {
    if (!ui || !desc || ui->widget_count >= GF_MAX_WIDGETS_3D) return -1;
    int id = ui->widget_count++;
    GFWidget3D *w = &ui->widgets[id];
    memset(w, 0, sizeof(*w));
    w->id = id;
    w->active = true;
    w->visible = true;
    w->room_id = desc->room_id;
    w->rack_id = desc->rack_id;
    w->kind = desc->kind;
    w->local_pos = desc->local_pos;
    w->size = desc->size;
    w->track_len = desc->track_len;
    w->param_id = desc->param_id;
    w->port_id = desc->port_id;
    w->aux_idx = desc->aux_idx;
    w->step_count = desc->step_count > 1 ? desc->step_count : 2;
    w->min_val = desc->min_val;
    w->max_val = (desc->max_val > desc->min_val) ? desc->max_val : (desc->min_val + 1.0f);
    w->default_val = gf_clampf(desc->default_val, w->min_val, w->max_val);
    w->norm_val = (w->default_val - w->min_val) / (w->max_val - w->min_val);
    w->color = desc->color;
    snprintf(w->label, sizeof(w->label), "%s", desc->label ? desc->label : "");
    snprintf(w->action_tag, sizeof(w->action_tag), "%s", desc->action_tag ? desc->action_tag : "");

    gf_spatial_update_widget_bounds(ui, w);

    if ((w->kind == GF_W3D_JACK_OUT || w->kind == GF_W3D_JACK_IN) &&
        w->port_id >= 0 && w->port_id < GF_PORT_COUNT) {
        const GFPortInfo *pi = gf_port_info((GFPortId)w->port_id);
        if (ui->primary_port_widget[w->port_id] < 0 || (pi && pi->room_id == w->room_id)) {
            ui->primary_port_widget[w->port_id] = id;
        }
    }
    return id;
}

GFWidget3D *gf_spatial_get_widget(GFSpatialUI *ui, int widget_id) {
    if (!ui || widget_id < 0 || widget_id >= ui->widget_count) return NULL;
    if (!ui->widgets[widget_id].active) return NULL;
    return &ui->widgets[widget_id];
}

const GFWidget3D *gf_spatial_get_widget_const(const GFSpatialUI *ui, int widget_id) {
    if (!ui || widget_id < 0 || widget_id >= ui->widget_count) return NULL;
    if (!ui->widgets[widget_id].active) return NULL;
    return &ui->widgets[widget_id];
}

float gf_widget_value_from_norm(const GFWidget3D *w) {
    if (!w) return 0.0f;
    float n = gf_clampf(w->norm_val, 0.0f, 1.0f);
    if (w->kind == GF_W3D_SWITCH) {
        return n >= 0.5f ? w->max_val : w->min_val;
    }
    if (w->kind == GF_W3D_STEP_DIAL && w->step_count > 1) {
        int step = (int)floorf(n * (float)(w->step_count - 1) + 0.5f);
        if (step < 0) step = 0;
        if (step >= w->step_count) step = w->step_count - 1;
        return w->min_val + ((float)step / (float)(w->step_count - 1)) * (w->max_val - w->min_val);
    }
    return w->min_val + n * (w->max_val - w->min_val);
}

void gf_widget_set_real_value(GFWidget3D *w, float real_val) {
    if (!w) return;
    float span = w->max_val - w->min_val;
    if (span <= 1e-8f) {
        w->norm_val = 0.0f;
        return;
    }
    w->norm_val = gf_clampf((real_val - w->min_val) / span, 0.0f, 1.0f);
}

void gf_spatial_sync_from_synth(GFSpatialUI *ui, const GFSynth *synth) {
    if (!ui || !synth) return;
    for (int i = 0; i < ui->widget_count; ++i) {
        GFWidget3D *w = &ui->widgets[i];
        if (!w->active) continue;
        if (w->param_id >= 0 && w->param_id < GF_P_COUNT) {
            float v = gf_synth_get_param(synth, (GFParamId)w->param_id);
            gf_widget_set_real_value(w, v);
        } else if (w->kind == GF_W3D_XYZ_ORB) {
            /* Float above Room 6 Morph Pedestal according to live XYZ coordinates */
            w->world_pos = gf_vec3(
                w->local_pos.x + (synth->xy_x - 0.5f) * 0.90f,
                w->local_pos.y + synth->xy_z * 0.55f,
                w->local_pos.z + (0.5f - synth->xy_y) * 0.90f
            );
        }
    }
}

const char *gf_spatial_widget_mesh_path(GFWidgetKind3D kind) {
    switch (kind) {
        case GF_W3D_KNOB:
        case GF_W3D_STEP_DIAL:    return "models/knob_rotary.obj";
        case GF_W3D_FADER:        return "models/fader_cap.obj";
        case GF_W3D_SWITCH:       return "models/switch_toggle.obj";
        case GF_W3D_JACK_OUT:
        case GF_W3D_JACK_IN:      return "models/jack_socket.obj";
        case GF_W3D_ENV_NODE:
        case GF_W3D_XYZ_ORB:      return "models/crystal_octa.obj";
        case GF_W3D_HARMONIC_BAR:
        case GF_W3D_BUTTON:       return "models/fader_cap.obj";
        case GF_W3D_PIANO_KEY:    return "models/piano_key_3d.obj";
        case GF_W3D_RACK_HEADER:
        default:                  return NULL;
    }
}

GFMat4 gf_spatial_widget_model_matrix(const GFSpatialUI *ui, const GFWidget3D *w) {
    if (!ui || !w) return gf_mat4_identity();
    GFVec3 r = w->axis_r, u = w->axis_u, f = w->axis_f;
    GFVec3 pos = w->world_pos;
    float sx = w->size.x > 0.01f ? w->size.x : 0.10f;
    float sy = w->size.y > 0.01f ? w->size.y : sx;
    float sz = sx * 0.65f;

    const GFRack3D *rk = gf_spatial_get_rack(ui, w->rack_id);

    if (w->kind == GF_W3D_KNOB || w->kind == GF_W3D_STEP_DIAL) {
        /* Rotate indicator pointer around rack forward axis from +135 deg (7 o'clock) to -135 deg (5 o'clock) */
        float ang = (0.75f * GF_PI) - w->norm_val * (1.50f * GF_PI);
        float ca = cosf(ang), sa = sinf(ang);
        GFVec3 nr = gf_vec3_add(gf_vec3_scale(r, ca), gf_vec3_scale(u, sa));
        GFVec3 nu = gf_vec3_add(gf_vec3_scale(r, -sa), gf_vec3_scale(u, ca));
        r = nr;
        u = nu;
        sz = sx * 0.62f;
    } else if (w->kind == GF_W3D_FADER && rk) {
        GFVec3 lp = w->local_pos;
        lp.y += (w->norm_val - 0.5f) * w->track_len;
        pos = gf_rack_local_to_world(rk, lp);
        sz = sx * 0.55f;
    } else if (w->kind == GF_W3D_SWITCH) {
        /* Tilt switch baton around rack right axis */
        float tilt = (w->norm_val >= 0.5f) ? 0.42f : -0.42f;
        float ct = cosf(tilt), st = sinf(tilt);
        GFVec3 nu = gf_vec3_add(gf_vec3_scale(u, ct), gf_vec3_scale(f, -st));
        GFVec3 nf = gf_vec3_add(gf_vec3_scale(u, st), gf_vec3_scale(f, ct));
        u = nu;
        f = nf;
        sz = sx * 0.70f;
    } else if (w->kind == GF_W3D_PIANO_KEY) {
        sz = sx * 0.65f;
        if (w->norm_val > 0.1f) {
            pos = gf_vec3_sub(pos, gf_vec3_scale(f, 0.018f));
        }
    } else if (w->kind == GF_W3D_HARMONIC_BAR && rk) {
        float bar_h = 0.018f + w->norm_val * (w->track_len > 0.02f ? w->track_len : 0.25f);
        GFVec3 lp = w->local_pos;
        lp.y += bar_h * 0.5f;
        pos = gf_rack_local_to_world(rk, lp);
        sy = bar_h * 1.85f;
        sz = sx * 0.8f;
    } else if (w->kind == GF_W3D_JACK_OUT || w->kind == GF_W3D_JACK_IN) {
        sz = sx * 0.52f;
    } else if (w->kind == GF_W3D_ENV_NODE || w->kind == GF_W3D_XYZ_ORB) {
        sz = sx;
    } else if (w->kind == GF_W3D_BUTTON) {
        sz = (w->norm_val >= 0.5f) ? sx * 0.25f : sx * 0.42f;
    }

    return mat4_from_basis(r, u, f, pos, gf_vec3(sx, sy, sz));
}

bool gf_spatial_port_world_socket(const GFSpatialUI *ui, GFPortId port, int preferred_room,
                                  GFVec3 *out_pos, GFVec3 *out_normal, int *out_room) {
    if (!ui || (int)port < 0 || port >= GF_PORT_COUNT) return false;

    const GFWidget3D *chosen = NULL;
    if (preferred_room >= 1) {
        for (int i = 0; i < ui->widget_count; ++i) {
            const GFWidget3D *w = &ui->widgets[i];
            if (w->active && (w->kind == GF_W3D_JACK_OUT || w->kind == GF_W3D_JACK_IN) &&
                w->port_id == (int)port && w->room_id == preferred_room) {
                chosen = w;
                break;
            }
        }
    }
    if (!chosen) {
        int idx = ui->primary_port_widget[port];
        if (idx >= 0 && idx < ui->widget_count && ui->widgets[idx].active) {
            chosen = &ui->widgets[idx];
        }
    }
    if (!chosen) return false;

    float tip_offset = (chosen->size.x > 0.01f ? chosen->size.x : 0.08f) * 0.26f;
    if (out_pos) *out_pos = gf_vec3_add(chosen->world_pos, gf_vec3_scale(chosen->axis_f, tip_offset));
    if (out_normal) *out_normal = chosen->axis_f;
    if (out_room) *out_room = chosen->room_id;
    return true;
}

GFRay gf_ray_from_screen(float screen_x, float screen_y, float vp_w, float vp_h,
                         const GFMat4 *view, const GFMat4 *proj) {
    GFRay fallback = {gf_vec3(0, 0, 0), gf_vec3(0, 0, -1)};
    if (!view || !proj || vp_w <= 1.0f || vp_h <= 1.0f) return fallback;

    GFMat4 vp = gf_mat4_mul(*proj, *view);
    GFMat4 inv_vp;
    if (!gf_mat4_invert(vp, &inv_vp)) return fallback;

    float ndc_x = (2.0f * screen_x) / vp_w - 1.0f;
    float ndc_y = 1.0f - (2.0f * screen_y) / vp_h;

    GFVec4 p_near = gf_mat4_mul_vec4(inv_vp, (GFVec4){ndc_x, ndc_y, -1.0f, 1.0f});
    GFVec4 p_far  = gf_mat4_mul_vec4(inv_vp, (GFVec4){ndc_x, ndc_y,  1.0f, 1.0f});
    if (fabsf(p_near.w) < 1e-7f || fabsf(p_far.w) < 1e-7f) return fallback;

    GFVec3 wn = gf_vec3(p_near.x / p_near.w, p_near.y / p_near.w, p_near.z / p_near.w);
    GFVec3 wf = gf_vec3(p_far.x / p_far.w,   p_far.y / p_far.w,   p_far.z / p_far.w);

    GFRay r;
    r.origin = wn;
    r.dir = gf_vec3_normalize(gf_vec3_sub(wf, wn));
    return r;
}

bool gf_ray_intersect_obb(GFRay ray, GFVec3 center,
                          GFVec3 axis_r, GFVec3 axis_u, GFVec3 axis_f,
                          GFVec3 half_extents, float *out_t) {
    float t_min = 0.0f;
    float t_max = 1e30f;
    GFVec3 p = gf_vec3_sub(center, ray.origin);
    GFVec3 axes[3] = {axis_r, axis_u, axis_f};
    float ext[3] = {half_extents.x, half_extents.y, half_extents.z};

    for (int i = 0; i < 3; ++i) {
        float e = gf_vec3_dot(axes[i], p);
        float f = gf_vec3_dot(axes[i], ray.dir);
        if (fabsf(f) > 1e-6f) {
            float t1 = (e - ext[i]) / f;
            float t2 = (e + ext[i]) / f;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > t_min) t_min = t1;
            if (t2 < t_max) t_max = t2;
            if (t_min > t_max || t_max < 0.0f) return false;
        } else {
            if (-e - ext[i] > 0.0f || -e + ext[i] < 0.0f) return false;
        }
    }
    if (out_t) *out_t = t_min >= 0.0f ? t_min : t_max;
    return true;
}

int gf_spatial_pick(const GFSpatialUI *ui, GFRay ray, const bool *room_vis,
                    bool touch_inflate, float *out_t) {
    if (!ui) return -1;
    int best_id = -1;
    float best_t = 1e30f;
    float inflate = touch_inflate ? 1.45f : 1.0f;

    for (int i = 0; i < ui->widget_count; ++i) {
        const GFWidget3D *w = &ui->widgets[i];
        if (!w->active || !w->visible) continue;
        if (room_vis && w->room_id >= 1 && w->room_id <= 7 && !room_vis[w->room_id]) continue;

        GFVec3 he = w->half_extents;
        if (w->kind != GF_W3D_RACK_HEADER && w->kind != GF_W3D_PIANO_KEY) {
            he = gf_vec3_scale(he, inflate);
        }
        float t = 0.0f;
        if (gf_ray_intersect_obb(ray, w->world_pos, w->axis_r, w->axis_u, w->axis_f, he, &t)) {
            if (t >= 0.0f && t < best_t) {
                best_t = t;
                best_id = w->id;
            }
        }
    }
    if (best_id >= 0 && out_t) *out_t = best_t;
    return best_id;
}

bool gf_spatial_ray_to_rack_local(const GFSpatialUI *ui, int rack_id, GFRay ray, GFVec2 *out_local_xy) {
    const GFRack3D *rk = gf_spatial_get_rack(ui, rack_id);
    if (!rk || !out_local_xy) return false;
    GFVec3 r, u, f;
    gf_rack_basis(rk, &r, &u, &f);
    float denom = gf_vec3_dot(ray.dir, f);
    if (fabsf(denom) < 1e-5f) return false;
    float t = gf_vec3_dot(gf_vec3_sub(rk->pos, ray.origin), f) / denom;
    if (t < 0.0f) return false;
    GFVec3 hit = gf_vec3_add(ray.origin, gf_vec3_scale(ray.dir, t));
    GFVec3 rel = gf_vec3_sub(hit, rk->pos);
    out_local_xy->x = gf_vec3_dot(rel, r);
    out_local_xy->y = gf_vec3_dot(rel, u);
    return true;
}

bool gf_spatial_set_curve_xy(GFSpatialUI *ui, int slot, int rack_id,
                             float cx, float cy, float w, float h,
                             const float *xs, const float *ys, int count,
                             bool is_bars, GFVec4 color) {
    if (!ui || slot < 0 || slot >= GF_MAX_CURVES_3D || !ys || count < 1) return false;
    const GFRack3D *rk = gf_spatial_get_rack(ui, rack_id);
    if (!rk) return false;

    GFCurveDisplay3D *cd = &ui->curves[slot];
    cd->active = true;
    cd->room_id = rk->room_id;
    cd->rack_id = rack_id;
    cd->cx = cx; cd->cy = cy; cd->w = w; cd->h = h;
    cd->is_bars = is_bars;
    cd->color = color;
    cd->count = count > GF_CURVE_MAX_PTS ? GF_CURVE_MAX_PTS : count;

    for (int i = 0; i < cd->count; ++i) {
        cd->pts_x[i] = xs ? gf_clampf(xs[i], 0.0f, 1.0f)
                          : (cd->count > 1 ? (float)i / (float)(cd->count - 1) : 0.5f);
        cd->pts_y[i] = gf_clampf(ys[i], 0.0f, 1.0f);
    }
    return true;
}

static void push_quad_3d(GFVertex *out, int *cnt, int max_v,
                         GFVec3 p0, GFVec3 p1, GFVec3 p2, GFVec3 p3,
                         GFVec3 nrm, float u0, float v0, float u1, float v1, GFVec4 col) {
    if (*cnt + 6 > max_v) return;
    GFVertex v[4] = {
        {p0.x, p0.y, p0.z, nrm.x, nrm.y, nrm.z, u0, v0, col.x, col.y, col.z, col.w},
        {p1.x, p1.y, p1.z, nrm.x, nrm.y, nrm.z, u1, v0, col.x, col.y, col.z, col.w},
        {p2.x, p2.y, p2.z, nrm.x, nrm.y, nrm.z, u1, v1, col.x, col.y, col.z, col.w},
        {p3.x, p3.y, p3.z, nrm.x, nrm.y, nrm.z, u0, v1, col.x, col.y, col.z, col.w},
    };
    out[(*cnt)++] = v[0]; out[(*cnt)++] = v[1]; out[(*cnt)++] = v[2];
    out[(*cnt)++] = v[0]; out[(*cnt)++] = v[2]; out[(*cnt)++] = v[3];
}

int gf_spatial_build_curve_verts(const GFSpatialUI *ui, const bool *room_vis,
                                 GFVertex *out_verts, int max_verts) {
    if (!ui || !out_verts || max_verts < 6) return 0;
    int cnt = 0;

    for (int s = 0; s < GF_MAX_CURVES_3D; ++s) {
        const GFCurveDisplay3D *cd = &ui->curves[s];
        if (!cd->active || cd->count < 1) continue;
        if (room_vis && cd->room_id >= 1 && cd->room_id <= 7 && !room_vis[cd->room_id]) continue;
        const GFRack3D *rk = gf_spatial_get_rack(ui, cd->rack_id);
        if (!rk) continue;

        GFVec3 r, u, f;
        gf_rack_basis(rk, &r, &u, &f);

        /* Recessed dark glass backing plate for holographic contrast */
        GFVec3 bg0 = gf_rack_local_to_world(rk, gf_vec3(cd->cx - cd->w * 0.5f, cd->cy - cd->h * 0.5f, 0.006f));
        GFVec3 bg1 = gf_rack_local_to_world(rk, gf_vec3(cd->cx + cd->w * 0.5f, cd->cy - cd->h * 0.5f, 0.006f));
        GFVec3 bg2 = gf_rack_local_to_world(rk, gf_vec3(cd->cx + cd->w * 0.5f, cd->cy + cd->h * 0.5f, 0.006f));
        GFVec3 bg3 = gf_rack_local_to_world(rk, gf_vec3(cd->cx - cd->w * 0.5f, cd->cy + cd->h * 0.5f, 0.006f));
        push_quad_3d(out_verts, &cnt, max_verts, bg0, bg1, bg2, bg3, f, 0, 0, 1, 1,
                     (GFVec4){0.03f, 0.06f, 0.08f, 0.94f});

        if (cd->is_bars) {
            float bw = (cd->w / (float)cd->count) * 0.76f;
            float y_base = cd->cy - cd->h * 0.46f;
            for (int i = 0; i < cd->count; ++i) {
                float x = (cd->cx - cd->w * 0.48f) + ((float)i + 0.5f) * (cd->w * 0.96f / (float)cd->count);
                float bh = 0.008f + cd->pts_y[i] * (cd->h * 0.90f);
                GFVec3 p0 = gf_rack_local_to_world(rk, gf_vec3(x - bw * 0.5f, y_base, 0.014f));
                GFVec3 p1 = gf_rack_local_to_world(rk, gf_vec3(x + bw * 0.5f, y_base, 0.014f));
                GFVec3 p2 = gf_rack_local_to_world(rk, gf_vec3(x + bw * 0.5f, y_base + bh, 0.014f));
                GFVec3 p3 = gf_rack_local_to_world(rk, gf_vec3(x - bw * 0.5f, y_base + bh, 0.014f));
                push_quad_3d(out_verts, &cnt, max_verts, p0, p1, p2, p3, f, 0, 0, 1, 1, cd->color);
            }
        } else if (cd->count >= 2) {
            float thick = 0.014f;
            for (int i = 0; i + 1 < cd->count; ++i) {
                float lx0 = (cd->cx - cd->w * 0.47f) + cd->pts_x[i] * (cd->w * 0.94f);
                float ly0 = (cd->cy - cd->h * 0.45f) + cd->pts_y[i] * (cd->h * 0.90f);
                float lx1 = (cd->cx - cd->w * 0.47f) + cd->pts_x[i + 1] * (cd->w * 0.94f);
                float ly1 = (cd->cy - cd->h * 0.45f) + cd->pts_y[i + 1] * (cd->h * 0.90f);

                float dx = lx1 - lx0, dy = ly1 - ly0;
                float len = sqrtf(dx * dx + dy * dy);
                float nx = (len > 1e-5f) ? (-dy / len) * thick * 0.5f : 0.0f;
                float ny = (len > 1e-5f) ? ( dx / len) * thick * 0.5f : thick * 0.5f;

                GFVec3 p0 = gf_rack_local_to_world(rk, gf_vec3(lx0 - nx, ly0 - ny, 0.015f));
                GFVec3 p1 = gf_rack_local_to_world(rk, gf_vec3(lx1 - nx, ly1 - ny, 0.015f));
                GFVec3 p2 = gf_rack_local_to_world(rk, gf_vec3(lx1 + nx, ly1 + ny, 0.015f));
                GFVec3 p3 = gf_rack_local_to_world(rk, gf_vec3(lx0 + nx, ly0 + ny, 0.015f));
                push_quad_3d(out_verts, &cnt, max_verts, p0, p1, p2, p3, f, 0, 0, 1, 1, cd->color);
            }
        }
    }
    return cnt;
}

static void push_world_string(const GFFontMetrics *font, GFVec3 center, GFVec3 axis_r, GFVec3 axis_u, GFVec3 axis_f,
                              float char_h, const char *str, GFVec4 col, GFVertex *out, int *cnt, int max_v) {
    if (!font || !str || !str[0] || font->cols <= 0 || font->rows <= 0) return;
    size_t len = strlen(str);
    float char_w = char_h * ((float)font->cell_w / (float)(font->cell_h > 0 ? font->cell_h : 8));
    float total_w = (float)len * char_w;
    float start_x = -total_w * 0.5f;
    float du = 1.0f / (float)font->cols;
    float dv = 1.0f / (float)font->rows;

    for (size_t i = 0; i < len; ++i) {
        int ch = (unsigned char)str[i];
        if (ch <= 32) continue;
        int idx = ch - font->first_char;
        if (idx < 0 || idx >= font->char_count) continue;

        int gc = idx % font->cols;
        int gr = idx / font->cols;
        float u0 = (float)gc * du;
        float v0 = (float)gr * dv;
        float u1 = u0 + du;
        float v1 = v0 + dv;

        float x0 = start_x + (float)i * char_w;
        float x1 = x0 + char_w;
        float y0 = -char_h * 0.5f;
        float y1 =  char_h * 0.5f;

        GFVec3 p0 = gf_vec3_add(center, gf_vec3_add(gf_vec3_scale(axis_r, x0), gf_vec3_scale(axis_u, y0)));
        GFVec3 p1 = gf_vec3_add(center, gf_vec3_add(gf_vec3_scale(axis_r, x1), gf_vec3_scale(axis_u, y0)));
        GFVec3 p2 = gf_vec3_add(center, gf_vec3_add(gf_vec3_scale(axis_r, x1), gf_vec3_scale(axis_u, y1)));
        GFVec3 p3 = gf_vec3_add(center, gf_vec3_add(gf_vec3_scale(axis_r, x0), gf_vec3_scale(axis_u, y1)));
        /* Bottom of quad gets v1 (bottom of glyph row), top of quad gets v0 (top of glyph row) */
        push_quad_3d(out, cnt, max_v, p0, p1, p2, p3, axis_f, u0, v1, u1, v0, col);
    }
}

int gf_spatial_build_world_text_verts(const GFSpatialUI *ui, const GFFontMetrics *font,
                                      const bool *room_vis, GFVertex *out_verts, int max_verts) {
    if (!ui || !font || !out_verts || max_verts < 6) return 0;
    int cnt = 0;

    /* 1. Rack Title Plaques on illuminated header bars */
    for (int i = 0; i < ui->rack_count; ++i) {
        const GFRack3D *rk = &ui->racks[i];
        if (!rk->active) continue;
        if (room_vis && rk->room_id >= 1 && rk->room_id <= 7 && !room_vis[rk->room_id]) continue;
        GFVec3 r, u, f;
        gf_rack_basis(rk, &r, &u, &f);
        GFVec3 hpos = gf_rack_local_to_world(rk, gf_vec3(0.0f, rk->height * 0.452f, 0.036f));
        push_world_string(font, hpos, r, u, f, 0.058f, rk->title,
                          (GFVec4){0.94f, 1.0f, 0.96f, 1.0f}, out_verts, &cnt, max_verts);
    }

    /* 2. Per-Widget Diegetic Labels on 3D rack faceplates */
    for (int i = 0; i < ui->widget_count; ++i) {
        const GFWidget3D *w = &ui->widgets[i];
        if (!w->active || !w->visible || !w->label[0]) continue;
        if (room_vis && w->room_id >= 1 && w->room_id <= 7 && !room_vis[w->room_id]) continue;

        float offset_y = -(w->size.y * 0.62f + 0.026f);
        if (w->kind == GF_W3D_FADER) {
            offset_y = -(w->track_len * 0.5f + 0.045f);
        } else if (w->kind == GF_W3D_BUTTON) {
            offset_y = 0.0f;
        }
        GFVec3 lpos = gf_vec3_add(w->world_pos, gf_vec3_scale(w->axis_u, offset_y));
        lpos = gf_vec3_add(lpos, gf_vec3_scale(w->axis_f, (w->kind == GF_W3D_BUTTON) ? 0.052f : 0.012f));

        bool hi = (w->id == ui->hovered_widget_id || w->id == ui->active_widget_id);
        GFVec4 col = hi ? (GFVec4){1.0f, 1.0f, 0.85f, 1.0f}
                        : (GFVec4){0.76f, 0.94f, 0.88f, 0.92f};
        push_world_string(font, lpos, w->axis_r, w->axis_u, w->axis_f, 0.030f, w->label,
                          col, out_verts, &cnt, max_verts);
    }

    /* 3. Floating 3D Diegetic Value Readout above hovered/active widget */
    if (ui->tooltip_visible && ui->tooltip_text[0]) {
        const GFWidget3D *aw = gf_spatial_get_widget_const(ui,
            ui->active_widget_id >= 0 ? ui->active_widget_id : ui->hovered_widget_id);
        GFVec3 r = aw ? aw->axis_r : gf_vec3(1, 0, 0);
        GFVec3 u = aw ? aw->axis_u : gf_vec3(0, 1, 0);
        GFVec3 f = aw ? aw->axis_f : gf_vec3(0, 0, 1);
        GFVec3 tpos = gf_vec3_add(ui->tooltip_pos, gf_vec3_add(gf_vec3_scale(u, 0.11f), gf_vec3_scale(f, 0.09f)));
        push_world_string(font, tpos, r, u, f, 0.044f, ui->tooltip_text,
                          (GFVec4){1.0f, 0.98f, 0.65f, 1.0f}, out_verts, &cnt, max_verts);
    }

    return cnt;
}
