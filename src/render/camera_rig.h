#ifndef GF_RENDER_CAMERA_RIG_H
#define GF_RENDER_CAMERA_RIG_H

#include "core/math3d.h"

#include <stdbool.h>

#define GF_SANCTUM_ROOM_COUNT     7
#define GF_SANCTUM_CORRIDOR_COUNT 12

typedef enum GFCameraMode {
    GF_CAM_FREE_WALK  = 0,
    GF_CAM_RACK_FOCUS = 1
} GFCameraMode;

typedef struct GFRoomDef {
    int id;                 /* 1..7 */
    const char *name;
    const char *short_tag;
    GFVec3 center;          /* Floor center in world space (y = 0) */
    float walk_radius;      /* Walkable octagonal/circular interior radius */
    GFVec3 spawn_offset;    /* Default vantage offset from center */
    float spawn_yaw;        /* Radians: 0 looks along -Z, +PI/2 looks along +X */
    GFVec3 theme_rgb;       /* Room accent illumination color */
} GFRoomDef;

typedef struct GFCorridorDef {
    int room_a;
    int room_b;
    float half_width;
} GFCorridorDef;

typedef struct GFCameraRig {
    GFCameraMode mode;
    GFVec3 pos;
    GFVec3 target;
    float yaw;          /* Radians: 0 => forward = (0, 0, -1), +yaw turns right (+X) */
    float pitch;        /* Radians: [-1.25, +1.25] */
    float fov_deg;
    float move_speed;
    float eye_height;
    int current_room;   /* 1..7 */

    /* Saved free-walk pose restored when exiting Rack-Focus */
    GFVec3 saved_walk_pos;
    float saved_walk_yaw;
    float saved_walk_pitch;
    int focused_rack_id;

    /* Smooth cubic ease-out transition state */
    bool animating;
    bool anim_to_walk;
    float anim_t;
    float anim_duration;
    GFVec3 anim_from_pos;
    GFVec3 anim_to_pos;
    GFVec3 anim_from_target;
    GFVec3 anim_to_target;
    float anim_from_fov;
    float anim_to_fov;
} GFCameraRig;

const GFRoomDef *gf_sanctum_room_def(int room_id);
const GFCorridorDef *gf_sanctum_corridor_def(int idx);

void gf_camera_rig_init(GFCameraRig *rig);

/* Clamps (x, z) to the union of the 7 room interiors and 12 connecting arch corridors. */
GFVec3 gf_camera_clamp_walkable(GFVec3 pos);
int gf_camera_detect_room(GFVec3 pos);

/* Free-walk controls (automatically exits Rack-Focus if substantial walk input occurs). */
void gf_camera_walk(GFCameraRig *rig, float forward_input, float right_input, float dt);
void gf_camera_look(GFCameraRig *rig, float delta_yaw_rad, float delta_pitch_rad);
bool gf_camera_teleport_room(GFCameraRig *rig, int room_id);

/* Smooth cubic Rack-Focus framing & restoration. */
void gf_camera_focus_rack(GFCameraRig *rig, int rack_id, GFVec3 rack_center,
                          GFVec3 rack_normal, float rack_w, float rack_h, float aspect);
void gf_camera_exit_focus(GFCameraRig *rig);

/* Advances camera animations and recomputes look target & active room. */
void gf_camera_step(GFCameraRig *rig, float dt);

GFVec3 gf_camera_forward(const GFCameraRig *rig);
GFVec3 gf_camera_right(const GFCameraRig *rig);
void gf_camera_compute_matrices(const GFCameraRig *rig, float aspect, float near_z, float far_z,
                                GFMat4 *out_view, GFMat4 *out_proj);

/* Transforms a local WebXR headset eye view matrix into the active sanctum room's world pose. */
GFMat4 gf_camera_apply_xr_view(const GFCameraRig *rig, GFMat4 xr_eye_view);

/* Frustum + Rack-Focus room visibility culling for rooms 1..7 (room_id <= 0 is always visible). */
bool gf_camera_is_room_visible(const GFCameraRig *rig, const GFMat4 *view,
                               float fov_deg, float aspect, float far_z, int room_id);

#endif
