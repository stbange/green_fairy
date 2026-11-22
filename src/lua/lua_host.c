#include "lua/lua_host.h"

#include "assets/loaders.h"
#include "core/log.h"

#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *GF_HOST_REG_KEY = "gf_lua_host_ptr";

static GFLuaHost *get_host(lua_State *L) {
    lua_getfield(L, LUA_REGISTRYINDEX, GF_HOST_REG_KEY);
    GFLuaHost *h = (GFLuaHost *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return h;
}

const char *gf_input_mode_name(GFInputMode mode) {
    switch (mode) {
        case GF_INPUT_TOUCH: return "touch";
        case GF_INPUT_XR:    return "xr";
        default:             return "desktop";
    }
}

static int pcall_with_trace(GFLuaHost *h, lua_State *L, int nargs, int nresults) {
    int base = lua_gettop(L) - nargs;
    int err = lua_pcall(L, nargs, nresults, 0);
    if (err != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        snprintf(h->last_error, sizeof(h->last_error), "%s", msg ? msg : "unknown lua error");
        GF_LOGE("lua", "%s", h->last_error);
        lua_settop(L, base - 1);
    }
    return err;
}

static GFPortId resolve_port_arg(lua_State *L, int arg_idx) {
    if (lua_type(L, arg_idx) == LUA_TNUMBER) {
        int id = (int)lua_tointeger(L, arg_idx);
        return (id >= 0 && id < GF_PORT_COUNT) ? (GFPortId)id : (GFPortId)-1;
    }
    const char *s = luaL_checkstring(L, arg_idx);
    return (GFPortId)gf_port_from_name(s);
}

static float tbl_num(lua_State *L, int idx, const char *k, float def_v) {
    lua_getfield(L, idx, k);
    float r = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : def_v;
    lua_pop(L, 1);
    return r;
}

static int tbl_int(lua_State *L, int idx, const char *k, int def_v) {
    lua_getfield(L, idx, k);
    int r = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : def_v;
    lua_pop(L, 1);
    return r;
}

static GFVec3 tbl_vec3(lua_State *L, int idx, const char *k, GFVec3 def_v) {
    lua_getfield(L, idx, k);
    GFVec3 r = def_v;
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1); if (lua_isnumber(L, -1)) r.x = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2); if (lua_isnumber(L, -1)) r.y = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 3); if (lua_isnumber(L, -1)) r.z = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return r;
}

static GFVec4 tbl_vec4(lua_State *L, int idx, const char *k, GFVec4 def_v) {
    lua_getfield(L, idx, k);
    GFVec4 r = def_v;
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1); if (lua_isnumber(L, -1)) r.x = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 2); if (lua_isnumber(L, -1)) r.y = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 3); if (lua_isnumber(L, -1)) r.z = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, -1, 4); if (lua_isnumber(L, -1)) r.w = (float)lua_tonumber(L, -1); lua_pop(L, 1);
    }
    lua_pop(L, 1);
    return r;
}

/* ---------------- Asset-confined Lua require & dofile ---------------- */

static int load_asset_lua_chunk(lua_State *L, const GFAssetManager *am, const char *rel_path) {
    uint8_t *data = NULL;
    size_t sz = 0;
    if (!gf_assets_read_file(am, rel_path, &data, &sz)) {
        lua_pushfstring(L, "cannot open asset lua file '%s'", rel_path);
        return LUA_ERRFILE;
    }
    char chunk_name[160];
    snprintf(chunk_name, sizeof(chunk_name), "@%s", rel_path);
    int st = luaL_loadbuffer(L, (const char *)data, sz, chunk_name);
    free(data);
    return st;
}

static int l_asset_searcher(lua_State *L) {
    GFLuaHost *h = get_host(L);
    const char *mod = luaL_checkstring(L, 1);
    char rel[256];
    char mod_path[192];
    size_t n = strlen(mod);
    if (n >= sizeof(mod_path) - 5) {
        lua_pushstring(L, "\n\tmodule name too long");
        return 1;
    }
    for (size_t i = 0; i < n; ++i) {
        mod_path[i] = (mod[i] == '.') ? '/' : mod[i];
    }
    mod_path[n] = '\0';

    snprintf(rel, sizeof(rel), "scripts/%s.lua", mod_path);
    int st = load_asset_lua_chunk(L, h->am, rel);
    if (st == LUA_OK) {
        lua_pushstring(L, rel);
        return 2;
    }
    if (st != LUA_ERRFILE) {
        return lua_error(L);
    }
    lua_pop(L, 1);

    snprintf(rel, sizeof(rel), "%s.lua", mod_path);
    st = load_asset_lua_chunk(L, h->am, rel);
    if (st == LUA_OK) {
        lua_pushstring(L, rel);
        return 2;
    }
    if (st != LUA_ERRFILE) {
        return lua_error(L);
    }
    return 1;
}

/* ---------------- gf.synth bindings ---------------- */

static int l_synth_note_on(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int note = (int)luaL_checkinteger(L, 1);
    float vel = (float)luaL_optnumber(L, 2, 0.8);
    lua_pushboolean(L, gf_synth_note_on(h->synth, note, vel));
    return 1;
}

static int l_synth_note_off(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int note = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, gf_synth_note_off(h->synth, note));
    return 1;
}

static int l_synth_all_notes_off(lua_State *L) {
    lua_pushboolean(L, gf_synth_all_notes_off(get_host(L)->synth));
    return 1;
}

static int l_synth_reset_default(lua_State *L) {
    gf_synth_reset_default_patch(get_host(L)->synth);
    return 0;
}

static int l_synth_set_param(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int id = (lua_type(L, 1) == LUA_TNUMBER)
        ? (int)lua_tointeger(L, 1)
        : gf_synth_param_from_name(luaL_checkstring(L, 1));
    float val = (float)luaL_checknumber(L, 2);
    if (id < 0 || id >= GF_P_COUNT) {
        lua_pushboolean(L, 0);
        return 1;
    }
    lua_pushboolean(L, gf_synth_set_param(h->synth, (GFParamId)id, val));
    return 1;
}

static int l_synth_get_param(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int id = (lua_type(L, 1) == LUA_TNUMBER)
        ? (int)lua_tointeger(L, 1)
        : gf_synth_param_from_name(luaL_checkstring(L, 1));
    if (id < 0 || id >= GF_P_COUNT) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, (lua_Number)gf_synth_get_param(h->synth, (GFParamId)id));
    return 1;
}

static int l_synth_param_list(lua_State *L) {
    lua_newtable(L);
    int out_i = 1;
    for (int i = 0; i < GF_P_COUNT; ++i) {
        const char *nm = gf_synth_param_name((GFParamId)i);
        if (nm && nm[0]) {
            lua_pushstring(L, nm);
            lua_rawseti(L, -2, out_i++);
        }
    }
    return 1;
}

static int l_synth_set_xy(lua_State *L) {
    GFLuaHost *h = get_host(L);
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    lua_pushboolean(L, gf_synth_set_xy(h->synth, x, y));
    return 1;
}

static int l_synth_set_xyz(lua_State *L) {
    GFLuaHost *h = get_host(L);
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float z = (float)luaL_optnumber(L, 3, h->synth->xy_z);
    lua_pushboolean(L, gf_synth_set_xyz(h->synth, x, y, z));
    return 1;
}

static int l_synth_set_macro(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int idx = (int)luaL_checkinteger(L, 1) - 1;
    float val = (float)luaL_checknumber(L, 2);
    lua_pushboolean(L, gf_synth_set_macro(h->synth, idx, val));
    return 1;
}

static int l_synth_set_wavetable(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int ch = (int)luaL_checkinteger(L, 1) - 1;
    bool slot_b = lua_toboolean(L, 2) != 0;
    luaL_checktype(L, 3, LUA_TTABLE);
    lua_Integer len = (lua_Integer)luaL_len(L, 3);
    if (len < 2 || len > 4096) {
        lua_pushboolean(L, 0);
        return 1;
    }
    float *buf = (float *)malloc((size_t)len * sizeof(float));
    if (!buf) {
        lua_pushboolean(L, 0);
        return 1;
    }
    for (lua_Integer i = 1; i <= len; ++i) {
        lua_rawgeti(L, 3, i);
        buf[i - 1] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    bool ok = gf_synth_set_wavetable(h->synth, ch, slot_b, buf, (int)len);
    free(buf);
    lua_pushboolean(L, ok);
    return 1;
}

static int l_synth_set_harmonics(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int ch = (int)luaL_checkinteger(L, 1) - 1;
    bool slot_b = lua_toboolean(L, 2) != 0;
    luaL_checktype(L, 3, LUA_TTABLE);
    int count = (int)luaL_len(L, 3);
    if (count < 1) { lua_pushboolean(L, 0); return 1; }
    if (count > GF_MAX_HARMONICS) count = GF_MAX_HARMONICS;
    float harms[GF_MAX_HARMONICS];
    for (int i = 0; i < count; ++i) {
        lua_rawgeti(L, 3, i + 1);
        harms[i] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pushboolean(L, gf_synth_set_harmonics(h->synth, ch, slot_b, harms, count));
    return 1;
}

static int l_synth_get_wavetable_preview(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int ch = (int)luaL_optinteger(L, 1, 1) - 1;
    bool slot_b = lua_toboolean(L, 2) != 0;
    int pts = (int)luaL_optinteger(L, 3, 64);
    if (ch < 0 || ch >= GF_NUM_CHANNELS) ch = 0;
    if (pts < 4) pts = 4;
    if (pts > 128) pts = 128;

    const GFWavetable *wt = slot_b ? &h->synth->wt_b[ch] : &h->synth->wt_a[ch];
    lua_newtable(L);
    for (int i = 0; i < pts; ++i) {
        int idx = (i * GF_WAVETABLE_SIZE) / pts;
        float v = 0.5f + 0.48f * wt->mips[0][idx];
        lua_pushnumber(L, gf_clampf(v, 0.0f, 1.0f));
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int l_synth_set_shaper_curve(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int ch = (int)luaL_checkinteger(L, 1) - 1; /* 1..3 for ch, 0 or 4 for master */
    if (ch == 3) ch = -1;
    luaL_checktype(L, 2, LUA_TTABLE);
    int count = (int)luaL_len(L, 2);
    if (count < 2) { lua_pushboolean(L, 0); return 1; }
    if (count > GF_TRANSFER_SIZE) count = GF_TRANSFER_SIZE;
    float pts[GF_TRANSFER_SIZE];
    for (int i = 0; i < count; ++i) {
        lua_rawgeti(L, 2, i + 1);
        pts[i] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pushboolean(L, gf_synth_set_shaper_curve(h->synth, ch, pts, count));
    return 1;
}

static int resolve_env_slot(lua_State *L, int idx) {
    if (lua_type(L, idx) == LUA_TNUMBER) {
        int s = (int)lua_tointeger(L, idx) - 1;
        return (s >= 0 && s < GF_NUM_ENVS) ? s : -1;
    }
    const char *nm = luaL_checkstring(L, idx);
    if (strcmp(nm, "amp") == 0) return 0;
    if (strcmp(nm, "filter") == 0) return 1;
    if (strcmp(nm, "mod") == 0) return 2;
    if (strncmp(nm, "env", 3) == 0) {
        int s = atoi(nm + 3) - 1;
        return (s >= 0 && s < GF_NUM_ENVS) ? s : -1;
    }
    return -1;
}

static int l_synth_set_envelope(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int slot = resolve_env_slot(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    int sus = (int)luaL_optinteger(L, 3, -1);
    int ls  = (int)luaL_optinteger(L, 4, -1);
    int le  = (int)luaL_optinteger(L, 5, -1);

    lua_Integer len = (lua_Integer)luaL_len(L, 2);
    if (slot < 0 || len < 2 || len > GF_ENV_MAX_POINTS) {
        lua_pushboolean(L, 0);
        return 1;
    }
    GFEnvPoint pts[GF_ENV_MAX_POINTS];
    memset(pts, 0, sizeof(pts));
    for (lua_Integer i = 1; i <= len; ++i) {
        lua_rawgeti(L, 2, i);
        if (lua_istable(L, -1)) {
            lua_rawgeti(L, -1, 1); pts[i - 1].time_sec = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_rawgeti(L, -1, 2); pts[i - 1].level    = (float)lua_tonumber(L, -1); lua_pop(L, 1);
            lua_rawgeti(L, -1, 3); pts[i - 1].curve    = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    int c_sus = (sus >= 1) ? (sus - 1) : -1;
    int c_ls  = (ls  >= 1) ? (ls  - 1) : -1;
    int c_le  = (le  >= 1) ? (le  - 1) : -1;
    lua_pushboolean(L, gf_synth_set_envelope(h->synth, slot, pts, (int)len, c_sus, c_ls, c_le));
    return 1;
}

static int l_synth_get_envelope(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int slot = resolve_env_slot(L, 1);
    if (slot < 0 || slot >= GF_NUM_ENVS) {
        lua_pushnil(L);
        return 1;
    }
    const GFEnvDef *ed = &h->synth->env_defs[slot];
    lua_newtable(L);
    lua_pushinteger(L, ed->sustain_index >= 0 ? ed->sustain_index + 1 : -1); lua_setfield(L, -2, "sustain");
    lua_pushinteger(L, ed->loop_start >= 0 ? ed->loop_start + 1 : -1);       lua_setfield(L, -2, "loop_start");
    lua_pushinteger(L, ed->loop_end >= 0 ? ed->loop_end + 1 : -1);           lua_setfield(L, -2, "loop_end");
    lua_pushinteger(L, (int)ed->mode);                                       lua_setfield(L, -2, "mode");
    lua_newtable(L);
    for (int i = 0; i < ed->count; ++i) {
        lua_newtable(L);
        lua_pushnumber(L, ed->points[i].time_sec); lua_rawseti(L, -2, 1);
        lua_pushnumber(L, ed->points[i].level);    lua_rawseti(L, -2, 2);
        lua_pushnumber(L, ed->points[i].curve);    lua_rawseti(L, -2, 3);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "points");
    return 1;
}

static int l_synth_set_seq_step(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int step = (int)luaL_checkinteger(L, 1) - 1;
    float pitch = (float)luaL_checknumber(L, 2);
    float mod_v = (float)luaL_optnumber(L, 3, 0.5);
    bool gate = lua_isnoneornil(L, 4) ? true : (lua_toboolean(L, 4) != 0);
    lua_pushboolean(L, gf_synth_set_seq_step(h->synth, step, pitch, mod_v, gate));
    return 1;
}

static int l_synth_get_seq_step(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int step = (int)luaL_checkinteger(L, 1) - 1;
    if (step < 0 || step >= GF_SEQ_MAX_STEPS) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    lua_pushnumber(L, h->synth->sequencer.steps[step].pitch_semi); lua_setfield(L, -2, "pitch");
    lua_pushnumber(L, h->synth->sequencer.steps[step].mod_val);    lua_setfield(L, -2, "mod");
    lua_pushboolean(L, h->synth->sequencer.steps[step].gate);      lua_setfield(L, -2, "gate");
    return 1;
}

static int l_synth_load_sample(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int slot = (int)luaL_checkinteger(L, 1) - 1;
    const char *rel = luaL_checkstring(L, 2);
    uint8_t *raw = NULL;
    size_t sz = 0;
    if (!gf_assets_read_file(h->am, rel, &raw, &sz)) {
        lua_pushboolean(L, 0);
        return 1;
    }
    GFAudioSample samp;
    bool ok = gf_load_wav_memory(raw, sz, &samp);
    free(raw);
    if (!ok) {
        lua_pushboolean(L, 0);
        return 1;
    }
    ok = gf_synth_load_sample(h->synth, slot, &samp);
    gf_audio_sample_free(&samp);
    lua_pushboolean(L, ok);
    return 1;
}

/* ---------------- gf.patchbay bindings ---------------- */

static int l_pb_clear(lua_State *L) {
    gf_synth_clear_cables(get_host(L)->synth);
    return 0;
}

static int l_pb_wire_default(lua_State *L) {
    gf_synth_wire_default_cables(get_host(L)->synth);
    return 0;
}

static int l_pb_connect(lua_State *L) {
    GFLuaHost *h = get_host(L);
    GFPortId src = resolve_port_arg(L, 1);
    GFPortId dst = resolve_port_arg(L, 2);
    float gain = (float)luaL_optnumber(L, 3, 0.75);
    lua_pushinteger(L, gf_synth_connect_cable(h->synth, src, dst, gain));
    return 1;
}

static int l_pb_disconnect(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int cid = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, gf_synth_disconnect_cable(h->synth, cid));
    return 1;
}

static int l_pb_set_gain(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int cid = (int)luaL_checkinteger(L, 1);
    float gain = (float)luaL_checknumber(L, 2);
    lua_pushboolean(L, gf_synth_set_cable_gain(h->synth, cid, gain));
    return 1;
}

static int l_pb_list_cables(lua_State *L) {
    GFLuaHost *h = get_host(L);
    const GFPatchPlan *plan = gf_synth_active_plan(h->synth);
    lua_newtable(L);
    if (!plan) return 1;
    for (int i = 0; i < plan->cable_count; ++i) {
        const GFCable *c = &plan->cables[i];
        const GFPortInfo *si = gf_port_info(c->src);
        const GFPortInfo *di = gf_port_info(c->dst);
        lua_newtable(L);
        lua_pushinteger(L, c->id);                          lua_setfield(L, -2, "id");
        lua_pushstring(L, si ? si->name : "");              lua_setfield(L, -2, "src");
        lua_pushstring(L, di ? di->name : "");              lua_setfield(L, -2, "dst");
        lua_pushinteger(L, (int)c->src);                    lua_setfield(L, -2, "src_id");
        lua_pushinteger(L, (int)c->dst);                    lua_setfield(L, -2, "dst_id");
        lua_pushnumber(L, c->gain);                         lua_setfield(L, -2, "gain");
        lua_pushboolean(L, c->is_feedback);                 lua_setfield(L, -2, "is_feedback");
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int l_pb_ports(lua_State *L) {
    lua_newtable(L);
    int out_i = 1;
    for (int i = 0; i < GF_PORT_COUNT; ++i) {
        const GFPortInfo *pi = gf_port_info((GFPortId)i);
        if (!pi) continue;
        lua_newtable(L);
        lua_pushinteger(L, (int)pi->id);      lua_setfield(L, -2, "id");
        lua_pushstring(L, pi->name);          lua_setfield(L, -2, "name");
        lua_pushboolean(L, pi->is_output);    lua_setfield(L, -2, "is_output");
        lua_pushinteger(L, pi->room_id);      lua_setfield(L, -2, "room");
        const char *ks = (pi->kind == GF_SIG_AUDIO) ? "audio"
                       : (pi->kind == GF_SIG_TRIGGER) ? "trigger" : "cv";
        lua_pushstring(L, ks);                lua_setfield(L, -2, "kind");
        lua_rawseti(L, -2, out_i++);
    }
    return 1;
}

/* ---------------- gf.spatial bindings (7-Room 3D Controls & Camera Rig) ---------------- */

static int l_sp_clear(lua_State *L) {
    gf_spatial_clear(&get_host(L)->scene->spatial);
    return 0;
}

static int l_sp_rooms(lua_State *L) {
    lua_newtable(L);
    for (int i = 1; i <= GF_SANCTUM_ROOM_COUNT; ++i) {
        const GFRoomDef *rm = gf_sanctum_room_def(i);
        lua_newtable(L);
        lua_pushinteger(L, rm->id);         lua_setfield(L, -2, "id");
        lua_pushstring(L, rm->name);        lua_setfield(L, -2, "name");
        lua_pushstring(L, rm->short_tag);   lua_setfield(L, -2, "tag");
        lua_pushnumber(L, rm->center.x);    lua_setfield(L, -2, "x");
        lua_pushnumber(L, rm->center.y);    lua_setfield(L, -2, "y");
        lua_pushnumber(L, rm->center.z);    lua_setfield(L, -2, "z");
        lua_newtable(L);
        lua_pushnumber(L, rm->theme_rgb.x); lua_rawseti(L, -2, 1);
        lua_pushnumber(L, rm->theme_rgb.y); lua_rawseti(L, -2, 2);
        lua_pushnumber(L, rm->theme_rgb.z); lua_rawseti(L, -2, 3);
        lua_setfield(L, -2, "theme");
        lua_rawseti(L, -2, i);
    }
    return 1;
}

static int l_sp_add_rack(lua_State *L) {
    GFLuaHost *h = get_host(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    int room_id = tbl_int(L, 1, "room", 6);
    lua_getfield(L, 1, "title");
    const char *title = lua_isstring(L, -1) ? lua_tostring(L, -1) : "SYNTH RACK";
    GFVec3 pos = tbl_vec3(L, 1, "pos", gf_vec3(0, 1.5f, 0));
    float rot_y = tbl_num(L, 1, "rot_y", 0.0f);
    float tilt_x = tbl_num(L, 1, "tilt_x", 0.0f);
    float width = tbl_num(L, 1, "width", 2.2f);
    float height = tbl_num(L, 1, "height", 1.7f);
    GFVec4 col = tbl_vec4(L, 1, "color", (GFVec4){0.30f, 0.85f, 0.72f, 1.0f});
    int rk = gf_spatial_add_rack(&h->scene->spatial, room_id, title, pos, rot_y, tilt_x, width, height, col);
    lua_pop(L, 1);
    lua_pushinteger(L, rk);
    return 1;
}

static GFWidgetKind3D parse_widget_kind(const char *s) {
    if (!s) return GF_W3D_KNOB;
    if (strcmp(s, "knob") == 0)         return GF_W3D_KNOB;
    if (strcmp(s, "fader") == 0)        return GF_W3D_FADER;
    if (strcmp(s, "switch") == 0)       return GF_W3D_SWITCH;
    if (strcmp(s, "step_dial") == 0)    return GF_W3D_STEP_DIAL;
    if (strcmp(s, "jack_out") == 0)     return GF_W3D_JACK_OUT;
    if (strcmp(s, "jack_in") == 0)      return GF_W3D_JACK_IN;
    if (strcmp(s, "env_node") == 0)     return GF_W3D_ENV_NODE;
    if (strcmp(s, "harmonic_bar") == 0) return GF_W3D_HARMONIC_BAR;
    if (strcmp(s, "xyz_orb") == 0)      return GF_W3D_XYZ_ORB;
    if (strcmp(s, "piano_key") == 0)    return GF_W3D_PIANO_KEY;
    if (strcmp(s, "button") == 0)       return GF_W3D_BUTTON;
    if (strcmp(s, "rack_header") == 0)  return GF_W3D_RACK_HEADER;
    return GF_W3D_KNOB;
}

static const char *widget_kind_name(GFWidgetKind3D k) {
    switch (k) {
        case GF_W3D_RACK_HEADER:  return "rack_header";
        case GF_W3D_KNOB:         return "knob";
        case GF_W3D_FADER:        return "fader";
        case GF_W3D_SWITCH:       return "switch";
        case GF_W3D_STEP_DIAL:    return "step_dial";
        case GF_W3D_JACK_OUT:     return "jack_out";
        case GF_W3D_JACK_IN:      return "jack_in";
        case GF_W3D_ENV_NODE:     return "env_node";
        case GF_W3D_HARMONIC_BAR: return "harmonic_bar";
        case GF_W3D_XYZ_ORB:      return "xyz_orb";
        case GF_W3D_PIANO_KEY:    return "piano_key";
        case GF_W3D_BUTTON:       return "button";
        default:                  return "knob";
    }
}

static int l_sp_add_widget(lua_State *L) {
    GFLuaHost *h = get_host(L);
    luaL_checktype(L, 1, LUA_TTABLE);

    GFWidgetDesc3D d;
    memset(&d, 0, sizeof(d));
    d.room_id = tbl_int(L, 1, "room", 6);
    d.rack_id = tbl_int(L, 1, "rack", -1);

    lua_getfield(L, 1, "kind");
    d.kind = parse_widget_kind(lua_tostring(L, -1));
    lua_pop(L, 1);

    d.local_pos = tbl_vec3(L, 1, "pos", gf_vec3(0, 0, 0.015f));
    GFVec3 sz3 = tbl_vec3(L, 1, "size", gf_vec3(0.11f, 0.11f, 0));
    d.size = (GFVec2){sz3.x, sz3.y};
    d.track_len = tbl_num(L, 1, "track_len", 0.30f);

    d.param_id = -1;
    lua_getfield(L, 1, "param");
    if (lua_isstring(L, -1)) d.param_id = gf_synth_param_from_name(lua_tostring(L, -1));
    lua_pop(L, 1);

    d.port_id = -1;
    lua_getfield(L, 1, "port");
    if (lua_isstring(L, -1)) d.port_id = gf_port_from_name(lua_tostring(L, -1));
    else if (lua_isnumber(L, -1)) d.port_id = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);

    d.aux_idx = tbl_int(L, 1, "aux", 0);
    d.step_count = tbl_int(L, 1, "steps", 2);
    d.min_val = tbl_num(L, 1, "min", 0.0f);
    d.max_val = tbl_num(L, 1, "max", 1.0f);
    d.default_val = tbl_num(L, 1, "default", d.min_val);
    d.color = tbl_vec4(L, 1, "color", (GFVec4){0.35f, 0.92f, 0.78f, 1.0f});

    char lbl_buf[28] = "";
    char act_buf[40] = "";
    lua_getfield(L, 1, "label");
    if (lua_isstring(L, -1)) snprintf(lbl_buf, sizeof(lbl_buf), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_getfield(L, 1, "action");
    if (lua_isstring(L, -1)) snprintf(act_buf, sizeof(act_buf), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);
    d.label = lbl_buf;
    d.action_tag = act_buf;

    int wid = gf_spatial_add_widget(&h->scene->spatial, &d);
    lua_pushinteger(L, wid);
    return 1;
}

static int l_sp_set_widget_state(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int wid = (int)luaL_checkinteger(L, 1);
    GFWidget3D *w = gf_spatial_get_widget(&h->scene->spatial, wid);
    if (!w) { lua_pushboolean(L, 0); return 1; }
    if (!lua_isnoneornil(L, 2)) w->norm_val = gf_clampf((float)lua_tonumber(L, 2), 0.0f, 1.0f);
    if (lua_isstring(L, 3)) snprintf(w->label, sizeof(w->label), "%s", lua_tostring(L, 3));
    if (!lua_isnoneornil(L, 4)) w->visible = lua_toboolean(L, 4) != 0;
    gf_spatial_update_widget_bounds(&h->scene->spatial, w);
    lua_pushboolean(L, 1);
    return 1;
}

static int l_sp_set_widget_local_pos(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int wid = (int)luaL_checkinteger(L, 1);
    GFWidget3D *w = gf_spatial_get_widget(&h->scene->spatial, wid);
    if (!w) { lua_pushboolean(L, 0); return 1; }
    w->local_pos.x = (float)luaL_checknumber(L, 2);
    w->local_pos.y = (float)luaL_checknumber(L, 3);
    w->local_pos.z = (float)luaL_optnumber(L, 4, w->local_pos.z);
    gf_spatial_update_widget_bounds(&h->scene->spatial, w);
    lua_pushboolean(L, 1);
    return 1;
}

static int l_sp_sync_widgets(lua_State *L) {
    GFLuaHost *h = get_host(L);
    gf_spatial_sync_from_synth(&h->scene->spatial, h->synth);
    return 0;
}

static int l_sp_set_hover(lua_State *L) {
    GFSpatialUI *ui = &get_host(L)->scene->spatial;
    ui->hovered_widget_id = (int)luaL_optinteger(L, 1, -1);
    ui->active_widget_id  = (int)luaL_optinteger(L, 2, -1);
    return 0;
}

static int l_sp_set_tooltip(lua_State *L) {
    GFSpatialUI *ui = &get_host(L)->scene->spatial;
    ui->tooltip_visible = lua_toboolean(L, 1) != 0;
    if (ui->tooltip_visible) {
        const char *txt = luaL_optstring(L, 2, "");
        snprintf(ui->tooltip_text, sizeof(ui->tooltip_text), "%s", txt);
        int wid = (int)luaL_optinteger(L, 3, ui->active_widget_id >= 0 ? ui->active_widget_id : ui->hovered_widget_id);
        const GFWidget3D *w = gf_spatial_get_widget_const(ui, wid);
        if (w) {
            ui->tooltip_pos = w->world_pos;
        }
    }
    return 0;
}

static int l_sp_set_curve(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int slot = (int)luaL_checkinteger(L, 1) - 1;
    int rack_id = (int)luaL_checkinteger(L, 2);
    float cx = (float)luaL_checknumber(L, 3);
    float cy = (float)luaL_checknumber(L, 4);
    float w  = (float)luaL_checknumber(L, 5);
    float ht = (float)luaL_checknumber(L, 6);
    bool has_xs = lua_istable(L, 7);
    luaL_checktype(L, 8, LUA_TTABLE);
    bool is_bars = lua_toboolean(L, 9) != 0;
    GFVec4 col = {
        (float)luaL_optnumber(L, 10, 0.35),
        (float)luaL_optnumber(L, 11, 0.98),
        (float)luaL_optnumber(L, 12, 0.78),
        (float)luaL_optnumber(L, 13, 0.95)
    };

    int count = (int)luaL_len(L, 8);
    if (count > GF_CURVE_MAX_PTS) count = GF_CURVE_MAX_PTS;
    if (count < 1) { lua_pushboolean(L, 0); return 1; }

    float xs[GF_CURVE_MAX_PTS], ys[GF_CURVE_MAX_PTS];
    for (int i = 0; i < count; ++i) {
        if (has_xs) {
            lua_rawgeti(L, 7, i + 1);
            xs[i] = (float)lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
        lua_rawgeti(L, 8, i + 1);
        ys[i] = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    lua_pushboolean(L, gf_spatial_set_curve_xy(&h->scene->spatial, slot, rack_id, cx, cy, w, ht,
                                               has_xs ? xs : NULL, ys, count, is_bars, col));
    return 1;
}

static int l_sp_walk(lua_State *L) {
    GFLuaHost *h = get_host(L);
    h->scene->use_camera_rig = true;
    gf_camera_walk(&h->scene->camera, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2),
                   (float)luaL_optnumber(L, 3, 0.016));
    gf_scene_step_camera(h->scene, 0.0f);
    return 0;
}

static int l_sp_look(lua_State *L) {
    GFLuaHost *h = get_host(L);
    h->scene->use_camera_rig = true;
    gf_camera_look(&h->scene->camera, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    gf_scene_step_camera(h->scene, 0.0f);
    return 0;
}

static int l_sp_teleport_room(lua_State *L) {
    GFLuaHost *h = get_host(L);
    h->scene->use_camera_rig = true;
    bool ok = gf_camera_teleport_room(&h->scene->camera, (int)luaL_checkinteger(L, 1));
    gf_scene_step_camera(h->scene, 0.0f);
    lua_pushboolean(L, ok);
    return 1;
}

static int l_sp_focus_rack(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int rk_id = (int)luaL_checkinteger(L, 1);
    const GFRack3D *rk = gf_spatial_get_rack(&h->scene->spatial, rk_id);
    if (!rk) { lua_pushboolean(L, 0); return 1; }
    GFVec3 r, u, f;
    gf_rack_basis(rk, &r, &u, &f);
    float aspect = (float)h->viewport_w / (float)(h->viewport_h > 0 ? h->viewport_h : 1);
    h->scene->use_camera_rig = true;
    gf_camera_focus_rack(&h->scene->camera, rk_id, rk->pos, f, rk->width, rk->height, aspect);
    lua_pushboolean(L, 1);
    return 1;
}

static int l_sp_exit_focus(lua_State *L) {
    GFLuaHost *h = get_host(L);
    gf_camera_exit_focus(&h->scene->camera);
    return 0;
}

static int l_sp_camera_state(lua_State *L) {
    const GFCameraRig *cam = &get_host(L)->scene->camera;
    const GFRoomDef *rm = gf_sanctum_room_def(cam->current_room);
    lua_newtable(L);
    lua_pushinteger(L, cam->current_room);                                      lua_setfield(L, -2, "room");
    lua_pushstring(L, rm ? rm->name : "Sanctum");                               lua_setfield(L, -2, "room_name");
    lua_pushstring(L, cam->mode == GF_CAM_RACK_FOCUS ? "focus" : "walk");       lua_setfield(L, -2, "mode");
    lua_pushinteger(L, cam->focused_rack_id);                                   lua_setfield(L, -2, "focused_rack");
    lua_pushnumber(L, cam->pos.x);                                              lua_setfield(L, -2, "x");
    lua_pushnumber(L, cam->pos.y);                                              lua_setfield(L, -2, "y");
    lua_pushnumber(L, cam->pos.z);                                              lua_setfield(L, -2, "z");
    lua_pushnumber(L, cam->yaw);                                                lua_setfield(L, -2, "yaw");
    lua_pushnumber(L, cam->pitch);                                              lua_setfield(L, -2, "pitch");
    return 1;
}

static int push_ray_pick_result(lua_State *L, GFLuaHost *h, GFRay ray) {
    bool touch_inf = (h->input_mode == GF_INPUT_TOUCH);
    float wt = 1e30f, rt = 1e30f;
    int wid = gf_spatial_pick(&h->scene->spatial, ray, NULL, touch_inf, &wt);
    int rcid = gf_cable_pick_ring(&h->scene->cable_batch, ray, NULL, touch_inf, &rt);

    lua_newtable(L);
    if (rcid > 0 && rt < wt) {
        lua_pushstring(L, "ring");  lua_setfield(L, -2, "hit");
        lua_pushinteger(L, rcid);   lua_setfield(L, -2, "cable_id");
        lua_pushnumber(L, rt);      lua_setfield(L, -2, "dist");
        for (int i = 0; i < GF_MAX_CABLES; ++i) {
            if (h->synth->patchbay.cables[i].active && h->synth->patchbay.cables[i].id == rcid) {
                lua_pushnumber(L, h->synth->patchbay.cables[i].gain);
                lua_setfield(L, -2, "gain");
                break;
            }
        }
        return 1;
    }

    if (wid >= 0) {
        const GFWidget3D *w = gf_spatial_get_widget_const(&h->scene->spatial, wid);
        lua_pushstring(L, "widget");                    lua_setfield(L, -2, "hit");
        lua_pushinteger(L, wid);                        lua_setfield(L, -2, "id");
        lua_pushstring(L, widget_kind_name(w->kind));   lua_setfield(L, -2, "kind");
        lua_pushinteger(L, w->rack_id);                 lua_setfield(L, -2, "rack");
        lua_pushinteger(L, w->room_id);                 lua_setfield(L, -2, "room");
        lua_pushinteger(L, w->aux_idx);                 lua_setfield(L, -2, "aux");
        lua_pushinteger(L, w->step_count);              lua_setfield(L, -2, "steps");
        lua_pushnumber(L, w->norm_val);                 lua_setfield(L, -2, "norm");
        lua_pushnumber(L, w->min_val);                  lua_setfield(L, -2, "min");
        lua_pushnumber(L, w->max_val);                  lua_setfield(L, -2, "max");
        lua_pushnumber(L, wt);                          lua_setfield(L, -2, "dist");
        lua_pushstring(L, w->label);                    lua_setfield(L, -2, "label");
        lua_pushstring(L, w->action_tag);               lua_setfield(L, -2, "action");
        if (w->param_id >= 0) {
            const char *pn = gf_synth_param_name((GFParamId)w->param_id);
            if (pn) { lua_pushstring(L, pn); lua_setfield(L, -2, "param"); }
        }
        if (w->port_id >= 0) {
            const GFPortInfo *pi = gf_port_info((GFPortId)w->port_id);
            if (pi) { lua_pushstring(L, pi->name); lua_setfield(L, -2, "port"); }
            lua_pushinteger(L, w->port_id); lua_setfield(L, -2, "port_id");
        }
        return 1;
    }

    lua_pushstring(L, "none");
    lua_setfield(L, -2, "hit");
    return 1;
}

static GFRay screen_ray_for_host(const GFLuaHost *h, float sx, float sy) {
    GFMat4 view, proj;
    float w = (float)(h->viewport_w > 1 ? h->viewport_w : 1280);
    float ht = (float)(h->viewport_h > 1 ? h->viewport_h : 720);
    gf_scene_compute_matrices(h->scene, w / ht, &view, &proj);
    return gf_ray_from_screen(sx, sy, w, ht, &view, &proj);
}

static int l_sp_pick_screen(lua_State *L) {
    GFLuaHost *h = get_host(L);
    GFRay ray = screen_ray_for_host(h, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    return push_ray_pick_result(L, h, ray);
}

static int l_sp_pick_ray(lua_State *L) {
    GFLuaHost *h = get_host(L);
    GFRay ray = {
        gf_vec3((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3)),
        gf_vec3_normalize(gf_vec3((float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6)))
    };
    return push_ray_pick_result(L, h, ray);
}

static int l_sp_ray_to_rack(lua_State *L) {
    GFLuaHost *h = get_host(L);
    int rk_id = (int)luaL_checkinteger(L, 1);
    GFRay ray;
    if (lua_gettop(L) >= 7) {
        ray.origin = gf_vec3((float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4));
        ray.dir = gf_vec3_normalize(gf_vec3((float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6), (float)luaL_checknumber(L, 7)));
    } else {
        ray = screen_ray_for_host(h, (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3));
    }
    GFVec2 lxy;
    if (!gf_spatial_ray_to_rack_local(&h->scene->spatial, rk_id, ray, &lxy)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, lxy.x);
    lua_pushnumber(L, lxy.y);
    return 2;
}

static int l_sp_cable_begin(lua_State *L) {
    GFLuaHost *h = get_host(L);
    GFPortId port = resolve_port_arg(L, 1);
    bool stack = lua_toboolean(L, 2) != 0;
    lua_pushboolean(L, gf_cable_begin_from_jack(&h->scene->cable_drag, h->synth, &h->scene->spatial,
                                                h->scene->camera.current_room, port, stack));
    return 1;
}

static int l_sp_cable_update_screen(lua_State *L) {
    GFLuaHost *h = get_host(L);
    GFRay ray = screen_ray_for_host(h, (float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2));
    int hov = (int)luaL_optinteger(L, 3, -1);
    float dist = (float)luaL_optnumber(L, 4, 2.0);
    gf_cable_update_drag(&h->scene->cable_drag, &h->scene->spatial, ray, dist, hov);
    return 0;
}

static int l_sp_cable_update_ray(lua_State *L) {
    GFLuaHost *h = get_host(L);
    GFRay ray = {
        gf_vec3((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3)),
        gf_vec3_normalize(gf_vec3((float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6)))
    };
    int hov = (int)luaL_optinteger(L, 7, -1);
    float dist = (float)luaL_optnumber(L, 8, 2.2);
    gf_cable_update_drag(&h->scene->cable_drag, &h->scene->spatial, ray, dist, hov);
    return 0;
}

static int l_sp_cable_finish(lua_State *L) {
    GFLuaHost *h = get_host(L);
    lua_pushinteger(L, gf_cable_finish_drag(&h->scene->cable_drag, h->synth));
    return 1;
}

static int l_sp_cable_active(lua_State *L) {
    lua_pushboolean(L, get_host(L)->scene->cable_drag.active);
    return 1;
}

static int l_sp_set_ring_hover(lua_State *L) {
    GFCableBatch3D *cb = &get_host(L)->scene->cable_batch;
    cb->hovered_ring_cable_id = (int)luaL_optinteger(L, 1, -1);
    cb->active_ring_cable_id  = (int)luaL_optinteger(L, 2, -1);
    return 0;
}

/* ---------------- gf.presets & gf.mutator helpers ---------------- */

static int l_presets_apply_file(lua_State *L) {
    GFLuaHost *h = get_host(L);
    const char *rel = luaL_checkstring(L, 1);
    bool reset_first = lua_isnoneornil(L, 2) ? true : (lua_toboolean(L, 2) != 0);
    if (reset_first) {
        gf_synth_reset_default_patch(h->synth);
    }
    if (load_asset_lua_chunk(L, h->am, rel) != LUA_OK) {
        lua_pushnil(L);
        lua_insert(L, -2);
        return 2;
    }
    if (pcall_with_trace(h, L, 0, 1) != LUA_OK) {
        lua_pushnil(L);
        lua_pushstring(L, h->last_error);
        return 2;
    }
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "name");
        if (lua_isstring(L, -1)) snprintf(h->active_preset_name, sizeof(h->active_preset_name), "%s", lua_tostring(L, -1));
        lua_pop(L, 1);
        lua_getfield(L, -1, "realm");
        if (lua_isstring(L, -1)) snprintf(h->active_preset_realm, sizeof(h->active_preset_realm), "%s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    gf_spatial_sync_from_synth(&h->scene->spatial, h->synth);
    return 1;
}

static int l_presets_set_active_meta(lua_State *L) {
    GFLuaHost *h = get_host(L);
    snprintf(h->active_preset_name, sizeof(h->active_preset_name), "%s", luaL_optstring(L, 1, "Init Sanctum"));
    snprintf(h->active_preset_realm, sizeof(h->active_preset_realm), "%s", luaL_optstring(L, 2, "fantasy_realms"));
    return 0;
}

static int l_mutator_snapshot(lua_State *L) {
    GFLuaHost *h = get_host(L);
    lua_newtable(L);

    /* 1. All named DSP parameters */
    lua_newtable(L);
    for (int i = 0; i < GF_P_COUNT; ++i) {
        const char *nm = gf_synth_param_name((GFParamId)i);
        if (nm && nm[0]) {
            lua_pushnumber(L, gf_synth_get_param(h->synth, (GFParamId)i));
            lua_setfield(L, -2, nm);
        }
    }
    lua_setfield(L, -2, "params");

    /* 2. All active modular cables */
    const GFPatchPlan *plan = gf_synth_active_plan(h->synth);
    lua_newtable(L);
    if (plan) {
        for (int i = 0; i < plan->cable_count; ++i) {
            const GFCable *c = &plan->cables[i];
            lua_newtable(L);
            lua_pushinteger(L, (int)c->src); lua_setfield(L, -2, "src");
            lua_pushinteger(L, (int)c->dst); lua_setfield(L, -2, "dst");
            lua_pushnumber(L, c->gain);      lua_setfield(L, -2, "gain");
            lua_rawseti(L, -2, i + 1);
        }
    }
    lua_setfield(L, -2, "cables");
    return 1;
}

static int l_mutator_restore(lua_State *L) {
    GFLuaHost *h = get_host(L);
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "params");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (lua_type(L, -2) == LUA_TSTRING && lua_isnumber(L, -1)) {
                int id = gf_synth_param_from_name(lua_tostring(L, -2));
                if (id >= 0) gf_synth_set_param(h->synth, (GFParamId)id, (float)lua_tonumber(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "cables");
    if (lua_istable(L, -1)) {
        gf_synth_clear_cables(h->synth);
        lua_Integer n = (lua_Integer)luaL_len(L, -1);
        for (lua_Integer i = 1; i <= n; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1)) {
                int src = tbl_int(L, -1, "src", -1);
                int dst = tbl_int(L, -1, "dst", -1);
                float g = tbl_num(L, -1, "gain", 0.75f);
                if (src >= 0 && dst >= 0) {
                    gf_synth_connect_cable(h->synth, (GFPortId)src, (GFPortId)dst, g);
                }
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    gf_spatial_sync_from_synth(&h->scene->spatial, h->synth);
    lua_pushboolean(L, 1);
    return 1;
}

/* ---------------- gf.scene bindings ---------------- */

static int l_scene_clear(lua_State *L) {
    gf_scene_clear_entities(get_host(L)->scene);
    return 0;
}

static int l_scene_add_mesh(lua_State *L) {
    GFScene *sc = get_host(L)->scene;
    int id = gf_scene_add_entity(sc, GF_ENT_MESH, luaL_checkstring(L, 1));
    int room_id = (int)luaL_optinteger(L, 2, 0);
    if (id > 0 && room_id > 0) gf_scene_set_entity_room(sc, id, room_id);
    lua_pushinteger(L, id);
    return 1;
}

static int l_scene_add_sprite(lua_State *L) {
    GFScene *sc = get_host(L)->scene;
    int id = gf_scene_add_entity(sc, GF_ENT_SPRITE, luaL_checkstring(L, 1));
    int room_id = (int)luaL_optinteger(L, 2, 0);
    if (id > 0 && room_id > 0) gf_scene_set_entity_room(sc, id, room_id);
    lua_pushinteger(L, id);
    return 1;
}

static int l_scene_add_voxel(lua_State *L) {
    GFScene *sc = get_host(L)->scene;
    int id = gf_scene_add_entity(sc, GF_ENT_VOXEL, luaL_checkstring(L, 1));
    int room_id = (int)luaL_optinteger(L, 2, 0);
    if (id > 0 && room_id > 0) gf_scene_set_entity_room(sc, id, room_id);
    lua_pushinteger(L, id);
    return 1;
}

static int l_scene_set_entity_room(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1);
    int rm = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, gf_scene_set_entity_room(get_host(L)->scene, id, rm));
    return 1;
}

static int l_scene_set_transform(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1);
    GFVec3 pos = gf_vec3((float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3), (float)luaL_checknumber(L, 4));
    GFVec3 rot = gf_vec3((float)luaL_optnumber(L, 5, 0), (float)luaL_optnumber(L, 6, 0), (float)luaL_optnumber(L, 7, 0));
    GFVec3 scl = gf_vec3((float)luaL_optnumber(L, 8, 1), (float)luaL_optnumber(L, 9, 1), (float)luaL_optnumber(L, 10, 1));
    lua_pushboolean(L, gf_scene_set_transform(get_host(L)->scene, id, pos, rot, scl));
    return 1;
}

static int l_scene_set_tint(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1);
    GFVec4 c = {
        (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3),
        (float)luaL_checknumber(L, 4), (float)luaL_optnumber(L, 5, 1.0)
    };
    lua_pushboolean(L, gf_scene_set_tint(get_host(L)->scene, id, c));
    return 1;
}

static int l_scene_set_sprite_anim(lua_State *L) {
    int id = (int)luaL_checkinteger(L, 1);
    GFVec2 sz = {(float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3)};
    int frames = (int)luaL_optinteger(L, 4, 1);
    int cur = (int)luaL_optinteger(L, 5, 0);
    lua_pushboolean(L, gf_scene_set_sprite_anim(get_host(L)->scene, id, sz, frames, cur));
    return 1;
}

static int l_scene_set_camera(lua_State *L) {
    GFVec3 pos = gf_vec3((float)luaL_checknumber(L, 1), (float)luaL_checknumber(L, 2), (float)luaL_checknumber(L, 3));
    GFVec3 tgt = gf_vec3((float)luaL_checknumber(L, 4), (float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6));
    float fov = (float)luaL_optnumber(L, 7, 55.0);
    gf_scene_set_camera(get_host(L)->scene, pos, tgt, fov);
    return 0;
}

static int l_scene_set_voxels_enabled(lua_State *L) {
    get_host(L)->scene->voxels_enabled = lua_toboolean(L, 1) != 0;
    return 0;
}

static int l_scene_get_voxels_enabled(lua_State *L) {
    lua_pushboolean(L, get_host(L)->scene->voxels_enabled);
    return 1;
}

static int l_scene_set_retro(lua_State *L) {
    GFScene *sc = get_host(L)->scene;
    sc->snap_res = (float)luaL_optnumber(L, 1, 360.0);
    sc->color_steps = (float)luaL_optnumber(L, 2, 32.0);
    sc->dither_enabled = lua_toboolean(L, 3) != 0;
    return 0;
}

static int l_scene_set_xr_laser(lua_State *L) {
    GFScene *sc = get_host(L)->scene;
    sc->xr_laser_active = lua_toboolean(L, 1) != 0;
    if (sc->xr_laser_active) {
        sc->xr_laser_origin = gf_vec3((float)luaL_optnumber(L, 2, 0), (float)luaL_optnumber(L, 3, 1.2), (float)luaL_optnumber(L, 4, 0));
        sc->xr_laser_end    = gf_vec3((float)luaL_optnumber(L, 5, 0), (float)luaL_optnumber(L, 6, 1.2), (float)luaL_optnumber(L, 7, -2));
    }
    return 0;
}

/* ---------------- gf.ui bindings ---------------- */

static int l_ui_rect(lua_State *L) {
    GFHudBatch *hb = get_host(L)->hud;
    float x = (float)luaL_checknumber(L, 1), y = (float)luaL_checknumber(L, 2);
    float w = (float)luaL_checknumber(L, 3), h = (float)luaL_checknumber(L, 4);
    GFVec4 c = {
        (float)luaL_checknumber(L, 5), (float)luaL_checknumber(L, 6),
        (float)luaL_checknumber(L, 7), (float)luaL_optnumber(L, 8, 1.0)
    };
    lua_pushboolean(L, gf_hud_push_rect(hb, x, y, w, h, c));
    return 1;
}

static int l_ui_line(lua_State *L) {
    GFHudBatch *hb = get_host(L)->hud;
    float x0 = (float)luaL_checknumber(L, 1), y0 = (float)luaL_checknumber(L, 2);
    float x1 = (float)luaL_checknumber(L, 3), y1 = (float)luaL_checknumber(L, 4);
    float th = (float)luaL_optnumber(L, 5, 2.0);
    GFVec4 c = {
        (float)luaL_checknumber(L, 6), (float)luaL_checknumber(L, 7),
        (float)luaL_checknumber(L, 8), (float)luaL_optnumber(L, 9, 1.0)
    };
    lua_pushboolean(L, gf_hud_push_line(hb, x0, y0, x1, y1, th, c));
    return 1;
}

static int l_ui_text(lua_State *L) {
    GFHudBatch *hb = get_host(L)->hud;
    float x = (float)luaL_checknumber(L, 1), y = (float)luaL_checknumber(L, 2);
    float sc = (float)luaL_checknumber(L, 3);
    const char *s = luaL_checkstring(L, 4);
    GFVec4 c = {
        (float)luaL_optnumber(L, 5, 1.0), (float)luaL_optnumber(L, 6, 1.0),
        (float)luaL_optnumber(L, 7, 1.0), (float)luaL_optnumber(L, 8, 1.0)
    };
    lua_pushboolean(L, gf_hud_push_text(hb, x, y, sc, s, c));
    return 1;
}

static int l_ui_viewport(lua_State *L) {
    GFLuaHost *h = get_host(L);
    lua_pushinteger(L, h->viewport_w);
    lua_pushinteger(L, h->viewport_h);
    return 2;
}

static int l_ui_input_mode(lua_State *L) {
    lua_pushstring(L, gf_input_mode_name(get_host(L)->input_mode));
    return 1;
}

/* ---------------- gf.assets & gf.log bindings ---------------- */

static int l_assets_list(lua_State *L) {
    GFLuaHost *h = get_host(L);
    const char *sub = luaL_optstring(L, 1, ".");
    char names[128][GF_ASSET_NAME_MAX];
    int n = gf_assets_list_dir(h->am, sub, names, 128);
    lua_newtable(L);
    for (int i = 0; i < n; ++i) {
        lua_pushstring(L, names[i]);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int l_assets_read_text(lua_State *L) {
    GFLuaHost *h = get_host(L);
    const char *rel = luaL_checkstring(L, 1);
    uint8_t *data = NULL;
    size_t sz = 0;
    if (!gf_assets_read_file(h->am, rel, &data, &sz)) {
        lua_pushnil(L);
        lua_pushstring(L, "asset read failed");
        return 2;
    }
    lua_pushlstring(L, (const char *)data, sz);
    free(data);
    return 1;
}

static int l_assets_dofile(lua_State *L) {
    GFLuaHost *h = get_host(L);
    const char *rel = luaL_checkstring(L, 1);
    if (load_asset_lua_chunk(L, h->am, rel) != LUA_OK) {
        return lua_error(L);
    }
    lua_call(L, 0, LUA_MULTRET);
    return lua_gettop(L) - 1;
}

static int l_gf_log(lua_State *L) {
    const char *msg = luaL_checkstring(L, 1);
    GF_LOGI("lua", "%s", msg);
    return 0;
}

/* ---------------- VM Construction & Registration ---------------- */

static const luaL_Reg g_synth_lib[] = {
    {"note_on", l_synth_note_on}, {"note_off", l_synth_note_off},
    {"all_notes_off", l_synth_all_notes_off}, {"reset_default", l_synth_reset_default},
    {"set_param", l_synth_set_param}, {"get_param", l_synth_get_param},
    {"param_list", l_synth_param_list},
    {"set_xy", l_synth_set_xy}, {"set_xyz", l_synth_set_xyz}, {"set_macro", l_synth_set_macro},
    {"set_wavetable", l_synth_set_wavetable}, {"set_harmonics", l_synth_set_harmonics},
    {"get_wavetable_preview", l_synth_get_wavetable_preview},
    {"set_shaper_curve", l_synth_set_shaper_curve},
    {"set_envelope", l_synth_set_envelope}, {"get_envelope", l_synth_get_envelope},
    {"set_seq_step", l_synth_set_seq_step}, {"get_seq_step", l_synth_get_seq_step},
    {"load_sample", l_synth_load_sample},
    {NULL, NULL}
};

static const luaL_Reg g_patchbay_lib[] = {
    {"clear", l_pb_clear}, {"wire_default", l_pb_wire_default},
    {"connect", l_pb_connect}, {"disconnect", l_pb_disconnect},
    {"set_gain", l_pb_set_gain}, {"list_cables", l_pb_list_cables},
    {"ports", l_pb_ports},
    {NULL, NULL}
};

static const luaL_Reg g_spatial_lib[] = {
    {"clear", l_sp_clear}, {"rooms", l_sp_rooms},
    {"add_rack", l_sp_add_rack}, {"add_widget", l_sp_add_widget},
    {"set_widget_state", l_sp_set_widget_state},
    {"set_widget_local_pos", l_sp_set_widget_local_pos},
    {"sync_widgets", l_sp_sync_widgets},
    {"set_hover", l_sp_set_hover}, {"set_tooltip", l_sp_set_tooltip},
    {"set_curve", l_sp_set_curve},
    {"walk", l_sp_walk}, {"look", l_sp_look},
    {"teleport_room", l_sp_teleport_room},
    {"focus_rack", l_sp_focus_rack}, {"exit_focus", l_sp_exit_focus},
    {"camera_state", l_sp_camera_state},
    {"pick_screen", l_sp_pick_screen}, {"pick_ray", l_sp_pick_ray},
    {"ray_to_rack", l_sp_ray_to_rack},
    {"cable_begin", l_sp_cable_begin},
    {"cable_update_screen", l_sp_cable_update_screen},
    {"cable_update_ray", l_sp_cable_update_ray},
    {"cable_finish", l_sp_cable_finish}, {"cable_active", l_sp_cable_active},
    {"set_ring_hover", l_sp_set_ring_hover},
    {NULL, NULL}
};

static const luaL_Reg g_presets_lib[] = {
    {"apply_file", l_presets_apply_file},
    {"set_active_meta", l_presets_set_active_meta},
    {NULL, NULL}
};

static const luaL_Reg g_mutator_lib[] = {
    {"snapshot", l_mutator_snapshot},
    {"restore", l_mutator_restore},
    {NULL, NULL}
};

static const luaL_Reg g_scene_lib[] = {
    {"clear", l_scene_clear}, {"add_mesh", l_scene_add_mesh},
    {"add_sprite", l_scene_add_sprite}, {"add_voxel", l_scene_add_voxel},
    {"set_entity_room", l_scene_set_entity_room},
    {"set_transform", l_scene_set_transform}, {"set_tint", l_scene_set_tint},
    {"set_sprite_anim", l_scene_set_sprite_anim}, {"set_camera", l_scene_set_camera},
    {"set_voxels_enabled", l_scene_set_voxels_enabled}, {"get_voxels_enabled", l_scene_get_voxels_enabled},
    {"set_retro", l_scene_set_retro}, {"set_xr_laser", l_scene_set_xr_laser},
    {NULL, NULL}
};

static const luaL_Reg g_ui_lib[] = {
    {"rect", l_ui_rect}, {"line", l_ui_line}, {"text", l_ui_text},
    {"viewport", l_ui_viewport}, {"input_mode", l_ui_input_mode},
    {NULL, NULL}
};

static const luaL_Reg g_assets_lib[] = {
    {"list", l_assets_list}, {"read_text", l_assets_read_text}, {"dofile", l_assets_dofile},
    {NULL, NULL}
};

static lua_State *create_sandboxed_vm(GFLuaHost *h) {
    lua_State *L = luaL_newstate();
    if (!L) return NULL;

    luaL_requiref(L, "_G", luaopen_base, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, 1); lua_pop(L, 1);
    if (h->allow_debug_lib) {
        luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1); lua_pop(L, 1);
    }

    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "loadfile");

    lua_pushlightuserdata(L, h);
    lua_setfield(L, LUA_REGISTRYINDEX, GF_HOST_REG_KEY);

    lua_getglobal(L, "package");
    lua_newtable(L);
    lua_pushcfunction(L, l_asset_searcher);
    lua_rawseti(L, -2, 1);
    lua_setfield(L, -2, "searchers");
    lua_pop(L, 1);

    lua_newtable(L);
    luaL_newlib(L, g_synth_lib);    lua_setfield(L, -2, "synth");
    luaL_newlib(L, g_patchbay_lib); lua_setfield(L, -2, "patchbay");
    luaL_newlib(L, g_spatial_lib);  lua_setfield(L, -2, "spatial");
    luaL_newlib(L, g_presets_lib);  lua_setfield(L, -2, "presets");
    luaL_newlib(L, g_mutator_lib);  lua_setfield(L, -2, "mutator");
    luaL_newlib(L, g_scene_lib);    lua_setfield(L, -2, "scene");
    luaL_newlib(L, g_ui_lib);       lua_setfield(L, -2, "ui");
    luaL_newlib(L, g_assets_lib);   lua_setfield(L, -2, "assets");
    lua_pushcfunction(L, l_gf_log); lua_setfield(L, -2, "log");
    lua_setglobal(L, "gf");

    return L;
}

bool gf_lua_init(GFLuaHost *h, const GFAssetManager *am, GFSynth *synth,
                 GFScene *scene, GFHudBatch *hud, bool allow_debug_lib) {
    if (!h || !am || !synth || !scene || !hud) return false;
    memset(h, 0, sizeof(*h));
    h->am = am;
    h->synth = synth;
    h->scene = scene;
    h->hud = hud;
    h->allow_debug_lib = allow_debug_lib;
    h->input_mode = GF_INPUT_DESKTOP;
    h->viewport_w = 1280;
    h->viewport_h = 720;
    snprintf(h->active_preset_name, sizeof(h->active_preset_name), "Init Sanctum");
    snprintf(h->active_preset_realm, sizeof(h->active_preset_realm), "fantasy_realms");
    h->L = create_sandboxed_vm(h);
    return h->L != NULL;
}

void gf_lua_destroy(GFLuaHost *h) {
    if (!h) return;
    if (h->L) {
        lua_close(h->L);
        h->L = NULL;
    }
}

static bool load_script_into_vm(GFLuaHost *h, lua_State *L, const char *rel_path) {
    int st = load_asset_lua_chunk(L, h->am, rel_path);
    if (st != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        snprintf(h->last_error, sizeof(h->last_error), "%s", msg ? msg : "load error");
        GF_LOGE("lua", "%s", h->last_error);
        lua_pop(L, 1);
        return false;
    }
    if (pcall_with_trace(h, L, 0, 0) != LUA_OK) {
        return false;
    }
    lua_getglobal(L, "gf_init");
    if (lua_isfunction(L, -1)) {
        if (pcall_with_trace(h, L, 0, 0) != LUA_OK) {
            return false;
        }
    } else {
        lua_pop(L, 1);
    }
    return true;
}

bool gf_lua_load_entry(GFLuaHost *h, const char *rel_script_path) {
    if (!h || !h->L || !rel_script_path) return false;
    snprintf(h->entry_script, sizeof(h->entry_script), "%s", rel_script_path);
    h->last_error[0] = '\0';
    return load_script_into_vm(h, h->L, rel_script_path);
}

bool gf_lua_reload(GFLuaHost *h) {
    if (!h || h->entry_script[0] == '\0') return false;
    lua_State *cand = create_sandboxed_vm(h);
    if (!cand) return false;
    if (!load_script_into_vm(h, cand, h->entry_script)) {
        lua_close(cand);
        return false;
    }
    if (h->L) lua_close(h->L);
    h->L = cand;
    h->reload_count++;
    h->last_error[0] = '\0';
    return true;
}

bool gf_lua_apply_preset(GFLuaHost *h, const char *rel_preset_path) {
    if (!h || !h->L || !rel_preset_path) return false;
    /* Prefer Lua-side gf_on_preset_path if registered (updates Room 7 UI + Mutator baseline), else direct apply */
    lua_getglobal(h->L, "gf_on_preset_path");
    if (lua_isfunction(h->L, -1)) {
        lua_pushstring(h->L, rel_preset_path);
        if (pcall_with_trace(h, h->L, 1, 1) != LUA_OK) return false;
        bool ok = lua_toboolean(h->L, -1) != 0;
        lua_pop(h->L, 1);
        return ok;
    }
    lua_pop(h->L, 1);

    gf_synth_reset_default_patch(h->synth);
    if (load_asset_lua_chunk(h->L, h->am, rel_preset_path) != LUA_OK) {
        const char *msg = lua_tostring(h->L, -1);
        snprintf(h->last_error, sizeof(h->last_error), "%s", msg ? msg : "preset load error");
        lua_pop(h->L, 1);
        return false;
    }
    if (pcall_with_trace(h, h->L, 0, 1) != LUA_OK) {
        return false;
    }
    if (lua_istable(h->L, -1)) {
        lua_getfield(h->L, -1, "name");
        if (lua_isstring(h->L, -1)) snprintf(h->active_preset_name, sizeof(h->active_preset_name), "%s", lua_tostring(h->L, -1));
        lua_pop(h->L, 1);
        lua_getfield(h->L, -1, "realm");
        if (lua_isstring(h->L, -1)) snprintf(h->active_preset_realm, sizeof(h->active_preset_realm), "%s", lua_tostring(h->L, -1));
        lua_pop(h->L, 1);
    }
    lua_pop(h->L, 1);
    gf_spatial_sync_from_synth(&h->scene->spatial, h->synth);
    return true;
}

bool gf_lua_call_mutator(GFLuaHost *h, const char *action, float amount, float randomness) {
    if (!h || !h->L || !action) return false;
    lua_getglobal(h->L, "gf_on_mutator");
    if (!lua_isfunction(h->L, -1)) {
        lua_pop(h->L, 1);
        return false;
    }
    lua_pushstring(h->L, action);
    lua_pushnumber(h->L, amount);
    lua_pushnumber(h->L, randomness);
    return pcall_with_trace(h, h->L, 3, 0) == LUA_OK;
}

bool gf_lua_update(GFLuaHost *h, float dt, const GFAudioTelemetry *tel) {
    if (!h || !h->L) return false;
    lua_getglobal(h->L, "gf_update");
    if (!lua_isfunction(h->L, -1)) {
        lua_pop(h->L, 1);
        return true;
    }
    lua_pushnumber(h->L, (lua_Number)dt);
    lua_newtable(h->L);
    if (tel) {
        lua_pushnumber(h->L, tel->rms_l); lua_setfield(h->L, -2, "rms_l");
        lua_pushnumber(h->L, tel->rms_r); lua_setfield(h->L, -2, "rms_r");
        lua_pushnumber(h->L, tel->peak);  lua_setfield(h->L, -2, "peak");
        lua_pushinteger(h->L, tel->active_voices); lua_setfield(h->L, -2, "active_voices");
        lua_pushinteger(h->L, tel->seq_step);      lua_setfield(h->L, -2, "seq_step");
        lua_newtable(h->L);
        for (int b = 0; b < GF_TELEMETRY_BANDS; ++b) {
            lua_pushnumber(h->L, tel->bands[b]);
            lua_rawseti(h->L, -2, b + 1);
        }
        lua_setfield(h->L, -2, "bands");
    }
    return pcall_with_trace(h, h->L, 2, 0) == LUA_OK;
}

bool gf_lua_render_ui(GFLuaHost *h, int vp_w, int vp_h) {
    if (!h || !h->L) return false;
    h->viewport_w = vp_w;
    h->viewport_h = vp_h;
    gf_hud_clear(h->hud);
    lua_getglobal(h->L, "gf_render_ui");
    if (!lua_isfunction(h->L, -1)) {
        lua_pop(h->L, 1);
        return true;
    }
    lua_pushinteger(h->L, vp_w);
    lua_pushinteger(h->L, vp_h);
    return pcall_with_trace(h, h->L, 2, 0) == LUA_OK;
}

bool gf_lua_on_input(GFLuaHost *h, const char *event_type, int id, float x, float y, float extra) {
    if (!h || !h->L || !event_type) return false;
    lua_getglobal(h->L, "gf_on_input");
    if (!lua_isfunction(h->L, -1)) {
        lua_pop(h->L, 1);
        return true;
    }
    lua_newtable(h->L);
    lua_pushstring(h->L, event_type); lua_setfield(h->L, -2, "type");
    lua_pushinteger(h->L, id);        lua_setfield(h->L, -2, "id");
    lua_pushnumber(h->L, x);          lua_setfield(h->L, -2, "x");
    lua_pushnumber(h->L, y);          lua_setfield(h->L, -2, "y");
    lua_pushnumber(h->L, extra);      lua_setfield(h->L, -2, "extra");
    return pcall_with_trace(h, h->L, 1, 0) == LUA_OK;
}

bool gf_lua_on_xr_ray(GFLuaHost *h, const char *event_type, int ctrl_id,
                      GFVec3 origin, GFVec3 dir, float value) {
    if (!h || !h->L || !event_type) return false;
    lua_getglobal(h->L, "gf_on_xr_ray");
    if (!lua_isfunction(h->L, -1)) {
        lua_pop(h->L, 1);
        return true;
    }
    lua_newtable(h->L);
    lua_pushstring(h->L, event_type); lua_setfield(h->L, -2, "type");
    lua_pushinteger(h->L, ctrl_id);   lua_setfield(h->L, -2, "id");
    lua_pushnumber(h->L, origin.x);   lua_setfield(h->L, -2, "ox");
    lua_pushnumber(h->L, origin.y);   lua_setfield(h->L, -2, "oy");
    lua_pushnumber(h->L, origin.z);   lua_setfield(h->L, -2, "oz");
    lua_pushnumber(h->L, dir.x);      lua_setfield(h->L, -2, "dx");
    lua_pushnumber(h->L, dir.y);      lua_setfield(h->L, -2, "dy");
    lua_pushnumber(h->L, dir.z);      lua_setfield(h->L, -2, "dz");
    lua_pushnumber(h->L, value);      lua_setfield(h->L, -2, "value");
    return pcall_with_trace(h, h->L, 1, 0) == LUA_OK;
}

bool gf_lua_set_input_mode(GFLuaHost *h, GFInputMode mode) {
    if (!h || !h->L) return false;
    h->input_mode = mode;
    lua_getglobal(h->L, "gf_on_mode_change");
    if (!lua_isfunction(h->L, -1)) {
        lua_pop(h->L, 1);
        return true;
    }
    lua_pushstring(h->L, gf_input_mode_name(mode));
    return pcall_with_trace(h, h->L, 1, 0) == LUA_OK;
}
