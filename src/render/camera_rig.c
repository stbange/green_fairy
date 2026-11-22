#include "render/camera_rig.h"

#include <string.h>

static const GFRoomDef k_rooms[GF_SANCTUM_ROOM_COUNT] = {
    {1, "Oscillator Conservatory",      "R1 OSC",  {  0.0f, 0.0f, -18.0f}, 6.1f, { 0.0f, 0.0f,  3.2f},  0.00f, {0.25f, 0.98f, 0.72f}},
    {2, "Filter & Shaper Foundry",      "R2 FLT",  { 16.0f, 0.0f, -10.0f}, 6.1f, {-2.6f, 0.0f,  1.8f},  1.05f, {0.98f, 0.65f, 0.24f}},
    {3, "68-Stage Envelope Observatory","R3 ENV",  { 16.0f, 0.0f,  10.0f}, 6.1f, {-2.6f, 0.0f, -1.8f},  2.09f, {0.30f, 0.84f, 1.00f}},
    {4, "Clockwork Mod & Patchbay",     "R4 MOD",  {  0.0f, 0.0f,  18.0f}, 6.1f, { 0.0f, 0.0f, -3.2f},  3.14159f, {0.95f, 0.86f, 0.32f}},
    {5, "Aetherium & Acoustics",        "R5 FX",   {-16.0f, 0.0f,  10.0f}, 6.1f, { 2.6f, 0.0f, -1.8f}, -2.09f, {0.78f, 0.44f, 1.00f}},
    {6, "Performance Bridge & Mutator", "R6 BRDG", {  0.0f, 0.0f,   0.0f}, 6.4f, { 0.0f, 0.0f,  3.4f},  0.00f, {0.35f, 1.00f, 0.82f}},
    {7, "Astral Preset Archive",        "R7 VAULT",{-16.0f, 0.0f, -10.0f}, 6.1f, { 2.6f, 0.0f,  1.8f}, -1.05f, {0.48f, 0.68f, 1.00f}},
};

static const GFCorridorDef k_corridors[GF_SANCTUM_CORRIDOR_COUNT] = {
    /* 6 Radial spokes from Central Bridge (Room 6) */
    {6, 1, 1.50f},
    {6, 2, 1.50f},
    {6, 3, 1.50f},
    {6, 4, 1.50f},
    {6, 5, 1.50f},
    {6, 7, 1.50f},
    /* 6 Outer hexagonal ring corridors */
    {1, 2, 1.45f},
    {2, 3, 1.45f},
    {3, 4, 1.45f},
    {4, 5, 1.45f},
    {5, 7, 1.45f},
    {7, 1, 1.45f},
};

const GFRoomDef *gf_sanctum_room_def(int room_id) {
    if (room_id < 1 || room_id > GF_SANCTUM_ROOM_COUNT) return NULL;
    return &k_rooms[room_id - 1];
}

const GFCorridorDef *gf_sanctum_corridor_def(int idx) {
    if (idx < 0 || idx >= GF_SANCTUM_CORRIDOR_COUNT) return NULL;
    return &k_corridors[idx];
}

static GFVec3 dir_from_yaw_pitch(float yaw, float pitch) {
    float cp = cosf(pitch);
    return gf_vec3_normalize(gf_vec3(sinf(yaw) * cp, sinf(pitch), -cosf(yaw) * cp));
}

GFVec3 gf_camera_forward(const GFCameraRig *rig) {
    if (!rig) return gf_vec3(0.0f, 0.0f, -1.0f);
    if (rig->mode == GF_CAM_RACK_FOCUS || rig->animating) {
        GFVec3 d = gf_vec3_sub(rig->target, rig->pos);
        if (gf_vec3_len(d) > 1e-5f) return gf_vec3_normalize(d);
    }
    return dir_from_yaw_pitch(rig->yaw, rig->pitch);
}

GFVec3 gf_camera_right(const GFCameraRig *rig) {
    GFVec3 f = gf_camera_forward(rig);
    GFVec3 r = gf_vec3_cross(f, gf_vec3(0.0f, 1.0f, 0.0f));
    if (gf_vec3_len(r) <= 1e-5f) return gf_vec3(1.0f, 0.0f, 0.0f);
    return gf_vec3_normalize(r);
}

int gf_camera_detect_room(GFVec3 pos) {
    int best_id = 6;
    float best_d2 = 1e30f;
    for (int i = 0; i < GF_SANCTUM_ROOM_COUNT; ++i) {
        float dx = pos.x - k_rooms[i].center.x;
        float dz = pos.z - k_rooms[i].center.z;
        float d2 = dx * dx + dz * dz;
        if (d2 < best_d2) {
            best_d2 = d2;
            best_id = k_rooms[i].id;
        }
    }
    return best_id;
}

GFVec3 gf_camera_clamp_walkable(GFVec3 pos) {
    /* Check if already inside any room disk or corridor capsule in XZ */
    for (int i = 0; i < GF_SANCTUM_ROOM_COUNT; ++i) {
        float dx = pos.x - k_rooms[i].center.x;
        float dz = pos.z - k_rooms[i].center.z;
        if (dx * dx + dz * dz <= k_rooms[i].walk_radius * k_rooms[i].walk_radius) {
            return pos;
        }
    }
    for (int c = 0; c < GF_SANCTUM_CORRIDOR_COUNT; ++c) {
        const GFRoomDef *ra = gf_sanctum_room_def(k_corridors[c].room_a);
        const GFRoomDef *rb = gf_sanctum_room_def(k_corridors[c].room_b);
        float ax = ra->center.x, az = ra->center.z;
        float bx = rb->center.x, bz = rb->center.z;
        float abx = bx - ax, abz = bz - az;
        float ab2 = abx * abx + abz * abz;
        float t = gf_clampf(((pos.x - ax) * abx + (pos.z - az) * abz) / ab2, 0.0f, 1.0f);
        float px = ax + abx * t;
        float pz = az + abz * t;
        float dx = pos.x - px, dz = pos.z - pz;
        if (dx * dx + dz * dz <= k_corridors[c].half_width * k_corridors[c].half_width) {
            return pos;
        }
    }

    /* Outside walkable floorplan: project smoothly onto nearest room boundary or corridor capsule */
    GFVec3 best = pos;
    float best_d2 = 1e30f;

    for (int i = 0; i < GF_SANCTUM_ROOM_COUNT; ++i) {
        float dx = pos.x - k_rooms[i].center.x;
        float dz = pos.z - k_rooms[i].center.z;
        float d = sqrtf(dx * dx + dz * dz);
        float r = k_rooms[i].walk_radius;
        float cx = (d > 1e-5f) ? (k_rooms[i].center.x + (dx / d) * r) : k_rooms[i].center.x;
        float cz = (d > 1e-5f) ? (k_rooms[i].center.z + (dz / d) * r) : k_rooms[i].center.z;
        float ex = pos.x - cx, ez = pos.z - cz;
        float e2 = ex * ex + ez * ez;
        if (e2 < best_d2) {
            best_d2 = e2;
            best = gf_vec3(cx, pos.y, cz);
        }
    }

    for (int c = 0; c < GF_SANCTUM_CORRIDOR_COUNT; ++c) {
        const GFRoomDef *ra = gf_sanctum_room_def(k_corridors[c].room_a);
        const GFRoomDef *rb = gf_sanctum_room_def(k_corridors[c].room_b);
        float ax = ra->center.x, az = ra->center.z;
        float bx = rb->center.x, bz = rb->center.z;
        float abx = bx - ax, abz = bz - az;
        float ab2 = abx * abx + abz * abz;
        float t = gf_clampf(((pos.x - ax) * abx + (pos.z - az) * abz) / ab2, 0.0f, 1.0f);
        float px = ax + abx * t;
        float pz = az + abz * t;
        float dx = pos.x - px, dz = pos.z - pz;
        float d = sqrtf(dx * dx + dz * dz);
        float hw = k_corridors[c].half_width;
        float cx = (d > 1e-5f) ? (px + (dx / d) * hw) : px;
        float cz = (d > 1e-5f) ? (pz + (dz / d) * hw) : pz;
        float ex = pos.x - cx, ez = pos.z - cz;
        float e2 = ex * ex + ez * ez;
        if (e2 < best_d2) {
            best_d2 = e2;
            best = gf_vec3(cx, pos.y, cz);
        }
    }
    return best;
}

void gf_camera_rig_init(GFCameraRig *rig) {
    if (!rig) return;
    memset(rig, 0, sizeof(*rig));
    rig->mode = GF_CAM_FREE_WALK;
    rig->eye_height = 1.62f;
    rig->move_speed = 6.5f;
    rig->fov_deg = 62.0f;
    rig->focused_rack_id = -1;
    gf_camera_teleport_room(rig, 6);
}

bool gf_camera_teleport_room(GFCameraRig *rig, int room_id) {
    if (!rig) return false;
    const GFRoomDef *rm = gf_sanctum_room_def(room_id);
    if (!rm) return false;

    rig->mode = GF_CAM_FREE_WALK;
    rig->animating = false;
    rig->focused_rack_id = -1;
    rig->fov_deg = 62.0f;
    rig->pos = gf_vec3(rm->center.x + rm->spawn_offset.x,
                       rig->eye_height,
                       rm->center.z + rm->spawn_offset.z);
    rig->pos = gf_camera_clamp_walkable(rig->pos);
    rig->yaw = rm->spawn_yaw;
    rig->pitch = -0.06f;
    rig->current_room = room_id;
    rig->target = gf_vec3_add(rig->pos, dir_from_yaw_pitch(rig->yaw, rig->pitch));
    rig->saved_walk_pos = rig->pos;
    rig->saved_walk_yaw = rig->yaw;
    rig->saved_walk_pitch = rig->pitch;
    return true;
}

void gf_camera_walk(GFCameraRig *rig, float forward_input, float right_input, float dt) {
    if (!rig || dt <= 0.0f) return;
    if (fabsf(forward_input) < 1e-4f && fabsf(right_input) < 1e-4f) return;

    if (rig->mode == GF_CAM_RACK_FOCUS) {
        /* Stepping away cancels Rack-Focus smoothly */
        gf_camera_exit_focus(rig);
    }

    GFVec3 fwd = gf_vec3(sinf(rig->yaw), 0.0f, -cosf(rig->yaw));
    GFVec3 rgt = gf_vec3(cosf(rig->yaw), 0.0f,  sinf(rig->yaw));
    GFVec3 wish = gf_vec3_add(gf_vec3_scale(fwd, forward_input), gf_vec3_scale(rgt, right_input));
    float len = gf_vec3_len(wish);
    if (len > 1.0f) wish = gf_vec3_scale(wish, 1.0f / len);

    GFVec3 next = gf_vec3_add(rig->pos, gf_vec3_scale(wish, rig->move_speed * dt));
    next.y = rig->eye_height;
    rig->pos = gf_camera_clamp_walkable(next);
    rig->current_room = gf_camera_detect_room(rig->pos);
    rig->target = gf_vec3_add(rig->pos, dir_from_yaw_pitch(rig->yaw, rig->pitch));
}

void gf_camera_look(GFCameraRig *rig, float delta_yaw_rad, float delta_pitch_rad) {
    if (!rig) return;
    if (rig->mode == GF_CAM_RACK_FOCUS) return; /* Locked cleanly onto focused rack */
    rig->yaw += delta_yaw_rad;
    while (rig->yaw > GF_PI) rig->yaw -= GF_TAU;
    while (rig->yaw < -GF_PI) rig->yaw += GF_TAU;
    rig->pitch = gf_clampf(rig->pitch + delta_pitch_rad, -1.22f, 1.22f);
    rig->target = gf_vec3_add(rig->pos, dir_from_yaw_pitch(rig->yaw, rig->pitch));
}

void gf_camera_focus_rack(GFCameraRig *rig, int rack_id, GFVec3 rack_center,
                          GFVec3 rack_normal, float rack_w, float rack_h, float aspect) {
    if (!rig) return;
    if (rig->mode == GF_CAM_FREE_WALK && !rig->animating) {
        rig->saved_walk_pos = rig->pos;
        rig->saved_walk_yaw = rig->yaw;
        rig->saved_walk_pitch = rig->pitch;
    }

    float focus_fov = 48.0f;
    float half_v = tanf((focus_fov * GF_PI / 180.0f) * 0.5f);
    float safe_aspect = aspect > 0.25f ? aspect : 1.0f;
    float half_h = half_v * safe_aspect;

    float dist_v = (rack_h * 0.58f) / half_v;
    float dist_h = (rack_w * 0.60f) / half_h;
    float dist = dist_v > dist_h ? dist_v : dist_h;
    dist = gf_clampf(dist, 0.95f, 4.80f);

    GFVec3 nrm = gf_vec3_normalize(rack_normal);
    if (gf_vec3_len(nrm) < 1e-4f) nrm = gf_vec3(0.0f, 0.0f, 1.0f);

    rig->anim_from_pos = rig->pos;
    rig->anim_from_target = rig->target;
    rig->anim_from_fov = rig->fov_deg;

    rig->anim_to_pos = gf_vec3_add(rack_center, gf_vec3_scale(nrm, dist));
    rig->anim_to_target = rack_center;
    rig->anim_to_fov = focus_fov;

    rig->mode = GF_CAM_RACK_FOCUS;
    rig->focused_rack_id = rack_id;
    rig->animating = true;
    rig->anim_to_walk = false;
    rig->anim_t = 0.0f;
    rig->anim_duration = 0.30f;
}

void gf_camera_exit_focus(GFCameraRig *rig) {
    if (!rig || (rig->mode == GF_CAM_FREE_WALK && !rig->animating)) return;
    rig->anim_from_pos = rig->pos;
    rig->anim_from_target = rig->target;
    rig->anim_from_fov = rig->fov_deg;

    rig->yaw = rig->saved_walk_yaw;
    rig->pitch = rig->saved_walk_pitch;
    rig->anim_to_pos = rig->saved_walk_pos;
    rig->anim_to_target = gf_vec3_add(rig->saved_walk_pos, dir_from_yaw_pitch(rig->yaw, rig->pitch));
    rig->anim_to_fov = 62.0f;

    rig->mode = GF_CAM_FREE_WALK;
    rig->focused_rack_id = -1;
    rig->animating = true;
    rig->anim_to_walk = true;
    rig->anim_t = 0.0f;
    rig->anim_duration = 0.26f;
}

void gf_camera_step(GFCameraRig *rig, float dt) {
    if (!rig) return;
    if (rig->animating) {
        float dur = rig->anim_duration > 0.01f ? rig->anim_duration : 0.25f;
        rig->anim_t += (dt > 0.0f ? dt : 0.0f) / dur;
        if (rig->anim_t >= 1.0f) {
            rig->anim_t = 1.0f;
            rig->animating = false;
        }
        /* Smooth cubic ease-out: 1 - (1 - t)^3 */
        float inv = 1.0f - rig->anim_t;
        float ease = 1.0f - inv * inv * inv;
        rig->pos = gf_vec3_lerp(rig->anim_from_pos, rig->anim_to_pos, ease);
        rig->target = gf_vec3_lerp(rig->anim_from_target, rig->anim_to_target, ease);
        rig->fov_deg = gf_lerpf(rig->anim_from_fov, rig->anim_to_fov, ease);
    } else if (rig->mode == GF_CAM_FREE_WALK) {
        rig->target = gf_vec3_add(rig->pos, dir_from_yaw_pitch(rig->yaw, rig->pitch));
    }
    rig->current_room = gf_camera_detect_room(rig->pos);
}

void gf_camera_compute_matrices(const GFCameraRig *rig, float aspect, float near_z, float far_z,
                                GFMat4 *out_view, GFMat4 *out_proj) {
    if (!rig || !out_view || !out_proj) return;
    GFVec3 up = gf_vec3(0.0f, 1.0f, 0.0f);
    GFVec3 dir = gf_vec3_normalize(gf_vec3_sub(rig->target, rig->pos));
    if (fabsf(dir.y) > 0.99f) {
        up = gf_vec3(0.0f, 0.0f, dir.y > 0.0f ? 1.0f : -1.0f);
    }
    *out_view = gf_mat4_look_at(rig->pos, rig->target, up);
    *out_proj = gf_mat4_perspective(rig->fov_deg * (GF_PI / 180.0f),
                                    aspect > 0.05f ? aspect : 1.0f,
                                    near_z > 0.01f ? near_z : 0.05f,
                                    far_z > 1.0f ? far_z : 80.0f);
}

GFMat4 gf_camera_apply_xr_view(const GFCameraRig *rig, GFMat4 xr_eye_view) {
    if (!rig) return xr_eye_view;
    /* Translate & rotate world into XR stage origin anchored at the active room floor vantage */
    GFVec3 stage_origin = gf_vec3(rig->pos.x, 0.0f, rig->pos.z);
    GFMat4 inv_rot = gf_mat4_rotate_y(-rig->yaw);
    GFMat4 inv_trans = gf_mat4_translate(gf_vec3(-stage_origin.x, -stage_origin.y, -stage_origin.z));
    return gf_mat4_mul(xr_eye_view, gf_mat4_mul(inv_rot, inv_trans));
}

bool gf_camera_is_room_visible(const GFCameraRig *rig, const GFMat4 *view,
                               float fov_deg, float aspect, float far_z, int room_id) {
    if (room_id <= 0 || room_id > GF_SANCTUM_ROOM_COUNT) return true;
    if (rig && room_id == rig->current_room) return true;
    if (!view) return true;

    const GFRoomDef *rm = gf_sanctum_room_def(room_id);
    /* When settled in close-up Rack-Focus, distant rooms behind/across the sanctum are culled */
    if (rig && rig->mode == GF_CAM_RACK_FOCUS && !rig->animating) {
        float dx = rm->center.x - rig->pos.x;
        float dz = rm->center.z - rig->pos.z;
        if (dx * dx + dz * dz > 22.0f * 22.0f) return false;
    }

    /* Bounding sphere of vaulted room shell: center (cx, 2.4, cz), radius 8.5m */
    GFVec4 sc_world = {rm->center.x, 2.4f, rm->center.z, 1.0f};
    GFVec4 sc_view = gf_mat4_mul_vec4(*view, sc_world);
    float radius = 8.5f;

    /* View space looks down -Z: depth = -sc_view.z */
    float depth = -sc_view.z;
    if (depth + radius < 0.05f) return false;   /* Completely behind camera */
    if (depth - radius > far_z) return false;   /* Beyond far plane */

    float half_v_rad = (fov_deg > 5.0f ? fov_deg : 60.0f) * (GF_PI / 180.0f) * 0.5f;
    float tan_v = tanf(half_v_rad);
    float tan_h = tan_v * (aspect > 0.1f ? aspect : 1.0f);
    float sec_v = sqrtf(1.0f + tan_v * tan_v);
    float sec_h = sqrtf(1.0f + tan_h * tan_h);

    float max_y = (depth > 0.0f ? depth : 0.0f) * tan_v + radius * sec_v;
    float max_x = (depth > 0.0f ? depth : 0.0f) * tan_h + radius * sec_h;
    if (fabsf(sc_view.y) > max_y) return false;
    if (fabsf(sc_view.x) > max_x) return false;

    return true;
}
