#ifndef GF_RENDER_SCENE_H
#define GF_RENDER_SCENE_H

#include "assets/loaders.h"
#include "core/math3d.h"
#include "dsp/synth.h"
#include "render/cable_renderer.h"
#include "render/camera_rig.h"
#include "render/spatial_ui.h"

#include <stdbool.h>
#include <stdint.h>

#define GF_MAX_SCENE_ENTITIES 640
#define GF_HUD_MAX_VERTS      16384
#define GF_HUD_MAX_CMDS       256

typedef enum GFEntityKind {
    GF_ENT_MESH   = 1,
    GF_ENT_SPRITE = 2,
    GF_ENT_VOXEL  = 3
} GFEntityKind;

typedef struct GFSceneEntity {
    int id;
    bool active;
    bool visible;
    bool emissive;
    int room_id; /* 0 = global/corridor/surroundings, 1..7 = room-culled */
    GFEntityKind kind;
    char asset_path[128];
    GFVec3 pos;
    GFVec3 rot;
    GFVec3 scale;
    GFVec4 tint;
    GFVec2 sprite_size;
    int sprite_frames;
    int sprite_frame;
} GFSceneEntity;

typedef struct GFScene {
    GFVec3 cam_pos;
    GFVec3 cam_target;
    GFVec3 cam_up;
    float cam_fov_deg;
    float cam_near;
    float cam_far;
    GFVec3 light_dir;

    /* Preset-driven sky gradient, atmospheric distance fog & live music-reactive state */
    bool custom_atmosphere;
    GFVec3 sky_zenith;
    GFVec3 sky_horizon;
    GFVec3 fog_color;
    float fog_density;
    float music_energy;
    float music_pitch;  /* [0..1] smoothed active note height */
    float music_strike; /* [0..1] decaying impulse on every keystroke */

    float snap_res;
    float color_steps;
    bool dither_enabled;
    bool voxels_enabled;
    bool ribbon_enabled;
    float anim_time;

    GFCameraRig camera;
    bool use_camera_rig;

    GFSpatialUI spatial;
    GFCableBatch3D cable_batch;
    GFCableDrag3D cable_drag;
    const GFPatchPlan *active_plan;

    GFFontMetrics world_font;
    bool has_world_font;

    bool xr_laser_active;
    GFVec3 xr_laser_origin;
    GFVec3 xr_laser_end;

    GFSceneEntity entities[GF_MAX_SCENE_ENTITIES];
    int next_id;
} GFScene;

typedef struct GFHudVertex {
    float x, y;
    float u, v;
    float r, g, b, a;
} GFHudVertex;

typedef struct GFHudCmd {
    uint32_t first_vert;
    uint32_t vert_count;
    bool textured;
} GFHudCmd;

typedef struct GFHudBatch {
    GFHudVertex verts[GF_HUD_MAX_VERTS];
    uint32_t vert_count;
    GFHudCmd cmds[GF_HUD_MAX_CMDS];
    uint32_t cmd_count;
    GFFontMetrics font;
    bool has_font;
} GFHudBatch;

void gf_scene_init(GFScene *sc);
void gf_scene_clear_entities(GFScene *sc);
int gf_scene_add_entity(GFScene *sc, GFEntityKind kind, const char *asset_path);
GFSceneEntity *gf_scene_find(GFScene *sc, int id);
bool gf_scene_set_entity_room(GFScene *sc, int id, int room_id);
bool gf_scene_set_visible(GFScene *sc, int id, bool visible);
bool gf_scene_set_emissive(GFScene *sc, int id, bool emissive);
bool gf_scene_set_transform(GFScene *sc, int id, GFVec3 pos, GFVec3 rot, GFVec3 scale);
bool gf_scene_set_tint(GFScene *sc, int id, GFVec4 tint);
bool gf_scene_set_sprite_anim(GFScene *sc, int id, GFVec2 size, int frames, int cur_frame);
bool gf_scene_remove(GFScene *sc, int id);
void gf_scene_set_atmosphere(GFScene *sc, GFVec3 sky_zenith, GFVec3 sky_horizon,
                             GFVec3 fog_color, float fog_density);
void gf_scene_set_music_state(GFScene *sc, float energy, float pitch_norm, float strike_pulse);
void gf_scene_set_camera(GFScene *sc, GFVec3 pos, GFVec3 target, float fov_deg);
void gf_scene_step_camera(GFScene *sc, float dt);
void gf_scene_compute_matrices(const GFScene *sc, float aspect, GFMat4 *out_view, GFMat4 *out_proj);

/* Builds audio-reactive 3D oscilloscope ribbon triangle strip (2 * N verts). */
int gf_scene_build_ribbon(const GFAudioTelemetry *tel, GFVertex *out_verts, int max_verts);

void gf_hud_init(GFHudBatch *hb, const GFFontMetrics *font);
void gf_hud_clear(GFHudBatch *hb);
bool gf_hud_push_rect(GFHudBatch *hb, float x, float y, float w, float h, GFVec4 col);
bool gf_hud_push_line(GFHudBatch *hb, float x0, float y0, float x1, float y1, float thickness, GFVec4 col);
bool gf_hud_push_text(GFHudBatch *hb, float x, float y, float scale, const char *text, GFVec4 col);

#endif
