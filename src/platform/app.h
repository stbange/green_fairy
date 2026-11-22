#ifndef GF_PLATFORM_APP_H
#define GF_PLATFORM_APP_H

#include "assets/asset_manager.h"
#include "assets/hot_reload.h"
#include "dsp/synth.h"
#include "lua/lua_host.h"
#include "render/gl_renderer.h"
#include "render/scene.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct GFAppConfig {
    const char *assets_dir;
    const char *exe_path;
    const char *e2e_script;
    bool headless;
    int width;
    int height;
    int max_frames;
    GFInputMode initial_mode;
} GFAppConfig;

typedef struct GFApp {
    GFAssetManager assets;
    GFHotReloader hot_reload;
    GFSynth synth;
    GFScene scene;
    GFHudBatch hud;
    GFGlRenderer renderer;
    GFLuaHost lua;

    int width;
    int height;
    bool running;
    bool xr_active;
    bool xr_trig_held[4];
    uint64_t frame_count;
    uint64_t xr_frame_count;
    bool initialized;
} GFApp;

void gf_app_config_default(GFAppConfig *cfg);
bool gf_app_parse_args(int argc, char **argv, GFAppConfig *cfg);
int gf_app_map_ascii_to_midi(int ascii_char);
bool gf_app_is_nav_key(int key_code);

bool gf_app_init(GFApp *app, const GFAppConfig *cfg, const GFGlApi *gl_api, bool is_gles);
void gf_app_destroy(GFApp *app);
void gf_app_resize(GFApp *app, int w, int h);
void gf_app_step(GFApp *app, float dt, bool render_offline_audio);

void gf_app_handle_key(GFApp *app, int ascii_char, bool pressed);
void gf_app_handle_pointer(GFApp *app, const char *type, int id, float x, float y);
void gf_app_handle_pointer_ex(GFApp *app, const char *type, int id, float x, float y, float extra);
void gf_app_handle_midi(GFApp *app, uint8_t status, uint8_t data1, uint8_t data2);
void gf_app_set_input_mode(GFApp *app, GFInputMode mode);
bool gf_app_trigger_reload(GFApp *app);

/* WebXR frame & 6DoF controller ray lifecycle */
void gf_app_xr_begin(GFApp *app, float dt);
void gf_app_xr_render_eye(GFApp *app, const float *view_m16, const float *proj_m16,
                          int vp_x, int vp_y, int vp_w, int vp_h);
void gf_app_xr_controller(GFApp *app, int ctrl_id, float trigger_value);
void gf_app_xr_controller_ray(GFApp *app, int ctrl_id,
                              float ox, float oy, float oz,
                              float dx, float dy, float dz,
                              float trigger_value, int teleport_room);
void gf_app_xr_end(GFApp *app);

/* Headless E2E script runner & artifact writers */
bool gf_app_run_e2e_script(GFApp *app, const char *script_path);
bool gf_app_write_ppm(const char *path, int w, int h, const uint8_t *rgba_bottom_up);
bool gf_app_write_wav(const char *path, uint32_t sample_rate, const float *interleaved_lr, uint32_t frames);
int gf_app_get_status_json(const GFApp *app, char *out_buf, size_t out_cap);

#endif
