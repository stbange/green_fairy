#include "render/scene.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void gf_scene_init(GFScene *sc) {
    if (!sc) return;
    memset(sc, 0, sizeof(*sc));
    gf_camera_rig_init(&sc->camera);
    gf_spatial_init(&sc->spatial);
    gf_cable_batch_init(&sc->cable_batch);
    gf_cable_drag_init(&sc->cable_drag);

    sc->use_camera_rig = true;
    sc->cam_pos = sc->camera.pos;
    sc->cam_target = sc->camera.target;
    sc->cam_up = gf_vec3(0.0f, 1.0f, 0.0f);
    sc->cam_fov_deg = sc->camera.fov_deg;
    sc->cam_near = 0.08f;
    sc->cam_far = 135.0f;
    sc->light_dir = gf_vec3_normalize(gf_vec3(0.55f, 0.88f, 0.45f));
    sc->custom_atmosphere = false;
    sc->sky_zenith  = gf_vec3(0.015f, 0.045f, 0.055f);
    sc->sky_horizon = gf_vec3(0.035f, 0.095f, 0.110f);
    sc->fog_color   = gf_vec3(0.025f, 0.075f, 0.088f);
    sc->fog_density = 0.014f;
    sc->music_energy = 0.0f;
    sc->music_pitch  = 0.5f;
    sc->music_strike = 0.0f;
    sc->snap_res = 360.0f;
    sc->color_steps = 32.0f;
    sc->dither_enabled = true;
    sc->voxels_enabled = true;
    sc->ribbon_enabled = true;
    sc->next_id = 1;
}

void gf_scene_clear_entities(GFScene *sc) {
    if (!sc) return;
    memset(sc->entities, 0, sizeof(sc->entities));
}

int gf_scene_add_entity(GFScene *sc, GFEntityKind kind, const char *asset_path) {
    if (!sc || !asset_path || asset_path[0] == '\0') return -1;
    for (int i = 0; i < GF_MAX_SCENE_ENTITIES; ++i) {
        if (!sc->entities[i].active) {
            GFSceneEntity *e = &sc->entities[i];
            memset(e, 0, sizeof(*e));
            e->id = sc->next_id++;
            e->active = true;
            e->visible = true;
            e->emissive = false;
            e->room_id = 0;
            e->kind = kind;
            snprintf(e->asset_path, sizeof(e->asset_path), "%s", asset_path);
            e->scale = gf_vec3(1.0f, 1.0f, 1.0f);
            e->tint = (GFVec4){1.0f, 1.0f, 1.0f, 1.0f};
            e->sprite_size = (GFVec2){0.8f, 0.8f};
            e->sprite_frames = 1;
            e->sprite_frame = 0;
            return e->id;
        }
    }
    return -1;
}

GFSceneEntity *gf_scene_find(GFScene *sc, int id) {
    if (!sc || id <= 0) return NULL;
    for (int i = 0; i < GF_MAX_SCENE_ENTITIES; ++i) {
        if (sc->entities[i].active && sc->entities[i].id == id) {
            return &sc->entities[i];
        }
    }
    return NULL;
}

bool gf_scene_set_entity_room(GFScene *sc, int id, int room_id) {
    GFSceneEntity *e = gf_scene_find(sc, id);
    if (!e) return false;
    e->room_id = room_id;
    return true;
}

bool gf_scene_set_visible(GFScene *sc, int id, bool visible) {
    GFSceneEntity *e = gf_scene_find(sc, id);
    if (!e) return false;
    e->visible = visible;
    return true;
}

bool gf_scene_set_emissive(GFScene *sc, int id, bool emissive) {
    GFSceneEntity *e = gf_scene_find(sc, id);
    if (!e) return false;
    e->emissive = emissive;
    return true;
}

bool gf_scene_set_transform(GFScene *sc, int id, GFVec3 pos, GFVec3 rot, GFVec3 scale) {
    GFSceneEntity *e = gf_scene_find(sc, id);
    if (!e) return false;
    e->pos = pos;
    e->rot = rot;
    e->scale = scale;
    return true;
}

bool gf_scene_set_tint(GFScene *sc, int id, GFVec4 tint) {
    GFSceneEntity *e = gf_scene_find(sc, id);
    if (!e) return false;
    e->tint = tint;
    return true;
}

bool gf_scene_set_sprite_anim(GFScene *sc, int id, GFVec2 size, int frames, int cur_frame) {
    GFSceneEntity *e = gf_scene_find(sc, id);
    if (!e) return false;
    e->sprite_size = size;
    e->sprite_frames = frames > 0 ? frames : 1;
    e->sprite_frame = ((cur_frame % e->sprite_frames) + e->sprite_frames) % e->sprite_frames;
    return true;
}

bool gf_scene_remove(GFScene *sc, int id) {
    GFSceneEntity *e = gf_scene_find(sc, id);
    if (!e) return false;
    e->active = false;
    return true;
}

void gf_scene_set_atmosphere(GFScene *sc, GFVec3 sky_zenith, GFVec3 sky_horizon,
                             GFVec3 fog_color, float fog_density) {
    if (!sc) return;
    sc->custom_atmosphere = true;
    sc->sky_zenith = sky_zenith;
    sc->sky_horizon = sky_horizon;
    sc->fog_color = fog_color;
    sc->fog_density = gf_clampf(fog_density, 0.0f, 0.08f);
}

void gf_scene_set_music_state(GFScene *sc, float energy, float pitch_norm, float strike_pulse) {
    if (!sc) return;
    sc->music_energy = gf_clampf(energy, 0.0f, 1.5f);
    sc->music_pitch  = gf_clampf(pitch_norm, 0.0f, 1.0f);
    sc->music_strike = gf_clampf(strike_pulse, 0.0f, 1.0f);
}

void gf_scene_set_camera(GFScene *sc, GFVec3 pos, GFVec3 target, float fov_deg) {
    if (!sc) return;
    sc->use_camera_rig = false;
    sc->cam_pos = pos;
    sc->cam_target = target;
    sc->cam_fov_deg = gf_clampf(fov_deg, 20.0f, 120.0f);
}

void gf_scene_step_camera(GFScene *sc, float dt) {
    if (!sc) return;
    sc->anim_time += (dt > 0.0f ? dt : 0.0f);
    if (sc->use_camera_rig) {
        gf_camera_step(&sc->camera, dt);
        sc->cam_pos = sc->camera.pos;
        sc->cam_target = sc->camera.target;
        sc->cam_fov_deg = sc->camera.fov_deg;
    }
}

void gf_scene_compute_matrices(const GFScene *sc, float aspect, GFMat4 *out_view, GFMat4 *out_proj) {
    if (!sc) return;
    if (sc->use_camera_rig && out_view && out_proj) {
        gf_camera_compute_matrices(&sc->camera, aspect, sc->cam_near, sc->cam_far, out_view, out_proj);
        return;
    }
    if (out_view) {
        *out_view = gf_mat4_look_at(sc->cam_pos, sc->cam_target, sc->cam_up);
    }
    if (out_proj) {
        *out_proj = gf_mat4_perspective(sc->cam_fov_deg * (GF_PI / 180.0f), aspect, sc->cam_near, sc->cam_far);
    }
}

int gf_scene_build_ribbon(const GFAudioTelemetry *tel, GFVertex *out_verts, int max_verts) {
    if (!tel || !out_verts || max_verts < 12) return 0;
    int segs = 64;
    if (segs * 6 > max_verts) segs = max_verts / 6;
    int count = 0;

    for (int i = 0; i < segs - 1; ++i) {
        int s0 = (i * GF_TELEMETRY_SCOPE) / segs;
        int s1 = ((i + 1) * GF_TELEMETRY_SCOPE) / segs;
        float t0 = (float)i / (float)(segs - 1);
        float t1 = (float)(i + 1) / (float)(segs - 1);
        float x0 = (t0 - 0.5f) * 5.2f;
        float x1 = (t1 - 0.5f) * 5.2f;
        float y0 = 3.85f + tel->scope[s0] * 0.75f;
        float y1 = 3.85f + tel->scope[s1] * 0.75f;
        float z0 = -2.2f + sinf(t0 * GF_TAU) * 0.45f;
        float z1 = -2.2f + sinf(t1 * GF_TAU) * 0.45f;
        float half_h = 0.038f + tel->rms_l * 0.07f;

        GFVertex v00 = {x0, y0 - half_h, z0, 0, 0, 1, t0, 0, 0.15f, 0.95f, 0.55f, 0.9f};
        GFVertex v01 = {x0, y0 + half_h, z0, 0, 0, 1, t0, 1, 0.55f, 1.00f, 0.80f, 0.9f};
        GFVertex v10 = {x1, y1 - half_h, z1, 0, 0, 1, t1, 0, 0.15f, 0.95f, 0.55f, 0.9f};
        GFVertex v11 = {x1, y1 + half_h, z1, 0, 0, 1, t1, 1, 0.55f, 1.00f, 0.80f, 0.9f};

        out_verts[count++] = v00;
        out_verts[count++] = v10;
        out_verts[count++] = v11;
        out_verts[count++] = v00;
        out_verts[count++] = v11;
        out_verts[count++] = v01;
    }
    return count;
}

/* ---------------- HUD Command Batcher ---------------- */

void gf_hud_init(GFHudBatch *hb, const GFFontMetrics *font) {
    if (!hb) return;
    memset(hb, 0, sizeof(*hb));
    if (font) {
        hb->font = *font;
        hb->has_font = true;
    }
}

void gf_hud_clear(GFHudBatch *hb) {
    if (!hb) return;
    hb->vert_count = 0;
    hb->cmd_count = 0;
}

static bool hud_begin_quads(GFHudBatch *hb, bool textured, uint32_t verts_needed) {
    if (!hb || hb->vert_count + verts_needed > GF_HUD_MAX_VERTS) return false;
    if (hb->cmd_count > 0 && hb->cmds[hb->cmd_count - 1].textured == textured) {
        return true;
    }
    if (hb->cmd_count >= GF_HUD_MAX_CMDS) return false;
    GFHudCmd *c = &hb->cmds[hb->cmd_count++];
    c->first_vert = hb->vert_count;
    c->vert_count = 0;
    c->textured = textured;
    return true;
}

static void hud_emit_quad(GFHudBatch *hb, float x0, float y0, float x1, float y1,
                          float u0, float v0, float u1, float v1, GFVec4 col) {
    GFHudVertex *v = &hb->verts[hb->vert_count];
    v[0] = (GFHudVertex){x0, y0, u0, v0, col.x, col.y, col.z, col.w};
    v[1] = (GFHudVertex){x1, y0, u1, v0, col.x, col.y, col.z, col.w};
    v[2] = (GFHudVertex){x1, y1, u1, v1, col.x, col.y, col.z, col.w};
    v[3] = (GFHudVertex){x0, y0, u0, v0, col.x, col.y, col.z, col.w};
    v[4] = (GFHudVertex){x1, y1, u1, v1, col.x, col.y, col.z, col.w};
    v[5] = (GFHudVertex){x0, y1, u0, v1, col.x, col.y, col.z, col.w};
    hb->vert_count += 6;
    hb->cmds[hb->cmd_count - 1].vert_count += 6;
}

bool gf_hud_push_rect(GFHudBatch *hb, float x, float y, float w, float h, GFVec4 col) {
    if (!hud_begin_quads(hb, false, 6)) return false;
    hud_emit_quad(hb, x, y, x + w, y + h, 0, 0, 1, 1, col);
    return true;
}

bool gf_hud_push_line(GFHudBatch *hb, float x0, float y0, float x1, float y1, float thickness, GFVec4 col) {
    if (!hud_begin_quads(hb, false, 6)) return false;
    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-4f) return true;
    float nx = (-dy / len) * (thickness * 0.5f);
    float ny = (dx / len) * (thickness * 0.5f);
    GFHudVertex *v = &hb->verts[hb->vert_count];
    v[0] = (GFHudVertex){x0 - nx, y0 - ny, 0, 0, col.x, col.y, col.z, col.w};
    v[1] = (GFHudVertex){x1 - nx, y1 - ny, 0, 0, col.x, col.y, col.z, col.w};
    v[2] = (GFHudVertex){x1 + nx, y1 + ny, 0, 0, col.x, col.y, col.z, col.w};
    v[3] = (GFHudVertex){x0 - nx, y0 - ny, 0, 0, col.x, col.y, col.z, col.w};
    v[4] = (GFHudVertex){x1 + nx, y1 + ny, 0, 0, col.x, col.y, col.z, col.w};
    v[5] = (GFHudVertex){x0 + nx, y0 + ny, 0, 0, col.x, col.y, col.z, col.w};
    hb->vert_count += 6;
    hb->cmds[hb->cmd_count - 1].vert_count += 6;
    return true;
}

bool gf_hud_push_text(GFHudBatch *hb, float x, float y, float scale, const char *text, GFVec4 col) {
    if (!hb || !text || !hb->has_font || scale <= 0.0f) return false;
    size_t n = strlen(text);
    if (n == 0) return true;
    if (!hud_begin_quads(hb, true, (uint32_t)(n * 6))) return false;

    float cw = (float)hb->font.cell_w * scale;
    float ch = (float)hb->font.cell_h * scale;
    float du = 1.0f / (float)hb->font.cols;
    float dv = 1.0f / (float)hb->font.rows;
    float cx = x;

    for (size_t i = 0; i < n; ++i) {
        int code = (unsigned char)text[i] - hb->font.first_char;
        if (code < 0 || code >= hb->font.char_count) code = 0;
        int col_i = code % hb->font.cols;
        int row_i = code / hb->font.cols;
        float u0 = (float)col_i * du;
        float v0 = (float)row_i * dv;
        hud_emit_quad(hb, cx, y, cx + cw, y + ch, u0, v0, u0 + du, v0 + dv, col);
        cx += cw;
    }
    return true;
}
