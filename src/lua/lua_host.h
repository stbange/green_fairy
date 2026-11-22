#ifndef GF_LUA_HOST_H
#define GF_LUA_HOST_H

#include "assets/asset_manager.h"
#include "dsp/synth.h"
#include "render/scene.h"

#include <lua.h>
#include <stdbool.h>

typedef enum GFInputMode {
    GF_INPUT_DESKTOP = 0,
    GF_INPUT_TOUCH   = 1,
    GF_INPUT_XR      = 2
} GFInputMode;

typedef struct GFLuaHost {
    lua_State *L;
    const GFAssetManager *am;
    GFSynth *synth;
    GFScene *scene;
    GFHudBatch *hud;
    bool allow_debug_lib;
    GFInputMode input_mode;
    int viewport_w;
    int viewport_h;
    char entry_script[128];
    char active_preset_name[64];
    char active_preset_realm[40];
    char looper_state[24];
    char active_melody_name[64];
    int looper_event_count;
    char last_error[512];
    int reload_count;
} GFLuaHost;

bool gf_lua_init(GFLuaHost *h, const GFAssetManager *am, GFSynth *synth,
                 GFScene *scene, GFHudBatch *hud, bool allow_debug_lib);
void gf_lua_destroy(GFLuaHost *h);

bool gf_lua_load_entry(GFLuaHost *h, const char *rel_script_path);
bool gf_lua_reload(GFLuaHost *h);
bool gf_lua_apply_preset(GFLuaHost *h, const char *rel_preset_path);
bool gf_lua_call_mutator(GFLuaHost *h, const char *action, float amount, float randomness);
bool gf_lua_apply_melody(GFLuaHost *h, const char *rel_melody_path);
bool gf_lua_looper_action(GFLuaHost *h, const char *action);

bool gf_lua_update(GFLuaHost *h, float dt, const GFAudioTelemetry *tel);
bool gf_lua_render_ui(GFLuaHost *h, int vp_w, int vp_h);
bool gf_lua_on_input(GFLuaHost *h, const char *event_type, int id, float x, float y, float extra);
bool gf_lua_on_xr_ray(GFLuaHost *h, const char *event_type, int ctrl_id,
                      GFVec3 origin, GFVec3 dir, float value);
bool gf_lua_set_input_mode(GFLuaHost *h, GFInputMode mode);
const char *gf_input_mode_name(GFInputMode mode);

#endif
