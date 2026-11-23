#include "platform/app.h"

#include "assets/loaders.h"
#include "core/log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gf_app_config_default(GFAppConfig *cfg) {
    if (!cfg) return;
    memset(cfg, 0, sizeof(*cfg));
    cfg->width = 1280;
    cfg->height = 720;
    cfg->max_frames = 0;
    cfg->initial_mode = GF_INPUT_DESKTOP;
}

bool gf_app_parse_args(int argc, char **argv, GFAppConfig *cfg) {
    if (!cfg) return false;
    gf_app_config_default(cfg);
    if (argc > 0 && argv && argv[0]) cfg->exe_path = argv[0];

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--assets") == 0 && i + 1 < argc) {
            cfg->assets_dir = argv[++i];
        } else if (strcmp(arg, "--headless") == 0) {
            cfg->headless = true;
        } else if (strcmp(arg, "--e2e-script") == 0 && i + 1 < argc) {
            cfg->e2e_script = argv[++i];
            cfg->headless = true;
        } else if (strcmp(arg, "--width") == 0 && i + 1 < argc) {
            cfg->width = atoi(argv[++i]);
        } else if (strcmp(arg, "--height") == 0 && i + 1 < argc) {
            cfg->height = atoi(argv[++i]);
        } else if (strcmp(arg, "--frames") == 0 && i + 1 < argc) {
            cfg->max_frames = atoi(argv[++i]);
        } else if (strcmp(arg, "--mode") == 0 && i + 1 < argc) {
            const char *m = argv[++i];
            if (strcmp(m, "touch") == 0) cfg->initial_mode = GF_INPUT_TOUCH;
            else if (strcmp(m, "xr") == 0) cfg->initial_mode = GF_INPUT_XR;
            else cfg->initial_mode = GF_INPUT_DESKTOP;
        } else {
            GF_LOGW("app", "Unknown CLI argument: %s", arg);
            return false;
        }
    }
    if (cfg->width < 64) cfg->width = 64;
    if (cfg->height < 64) cfg->height = 64;
    return true;
}

bool gf_app_is_nav_key(int key_code) {
    int c = (key_code >= 'A' && key_code <= 'Z') ? tolower(key_code) : key_code;
    if (c == 27) return true; /* Escape: exit Rack-Focus */
    if (c >= '1' && c <= '7') return true; /* 1..7: instant room teleport */
    if (c == 'w' || c == 'a' || c == 's' || c == 'd') return true; /* WASD walk */
    if (c == ' ' || c == 'l' || c == '[' || c == ']' || c == '0') return true; /* Looper & Melody transport */
    return false;
}

int gf_app_map_ascii_to_midi(int ascii_char) {
    int c = tolower(ascii_char);
    if (c == 0 || gf_app_is_nav_key(c)) return -1;
    /* Keep 'z' -> 48 (C3) and 'q' -> 60 (C4) while freeing WASD & 1..7 for 3D room navigation */
    static const struct { char ch; int note; } k_map[] = {
        {'z', 48}, {'x', 50}, {'c', 52}, {'v', 53}, {'b', 55}, {'n', 57}, {'m', 59}, {',', 60},
        {'q', 60}, {'e', 62}, {'r', 64}, {'t', 65}, {'y', 67}, {'u', 69}, {'i', 71}, {'o', 72}, {'p', 74}
    };
    for (size_t i = 0; i < sizeof(k_map) / sizeof(k_map[0]); ++i) {
        if (k_map[i].ch == c) return k_map[i].note;
    }
    return -1;
}

static void on_asset_changed(const char *rel_path, void *user_data) {
    GFApp *app = (GFApp *)user_data;
    if (strstr(rel_path, ".glsl") != NULL) {
        gf_gl_renderer_reload_shaders(&app->renderer);
    } else if (strstr(rel_path, ".lua") != NULL) {
        gf_lua_reload(&app->lua);
    }
}

bool gf_app_init(GFApp *app, const GFAppConfig *cfg, const GFGlApi *gl_api, bool is_gles) {
    if (!app || !cfg || !gl_api) return false;
    memset(app, 0, sizeof(*app));
    app->width = cfg->width > 0 ? cfg->width : 1280;
    app->height = cfg->height > 0 ? cfg->height : 720;

    if (!gf_assets_init(&app->assets, cfg->assets_dir, cfg->exe_path)) {
        return false;
    }

    gf_synth_init(&app->synth, 44100.0f, 0xA85174E1u);
    gf_scene_init(&app->scene);

    GFFontMetrics fm;
    bool has_fm = false;
    uint8_t *fdata = NULL;
    size_t fsz = 0;
    if (gf_assets_read_file(&app->assets, "fonts/retro_font.fnt", &fdata, &fsz)) {
        has_fm = gf_load_font_metrics_memory((const char *)fdata, fsz, &fm);
        free(fdata);
    }
    gf_hud_init(&app->hud, has_fm ? &fm : NULL);
    if (has_fm) {
        app->scene.world_font = fm;
        app->scene.has_world_font = true;
    }

    if (!gf_gl_renderer_init(&app->renderer, gl_api, &app->assets, is_gles)) {
        gf_synth_destroy(&app->synth);
        return false;
    }

    if (!gf_lua_init(&app->lua, &app->assets, &app->synth, &app->scene, &app->hud, false)) {
        gf_gl_renderer_destroy(&app->renderer);
        gf_synth_destroy(&app->synth);
        return false;
    }
    app->lua.input_mode = cfg->initial_mode;
    app->lua.viewport_w = app->width;
    app->lua.viewport_h = app->height;

    if (!gf_lua_load_entry(&app->lua, "scripts/main.lua")) {
        gf_lua_destroy(&app->lua);
        gf_gl_renderer_destroy(&app->renderer);
        gf_synth_destroy(&app->synth);
        return false;
    }

    gf_hot_reload_init(&app->hot_reload, &app->assets);
    gf_hot_reload_watch(&app->hot_reload, "scripts/main.lua", on_asset_changed, app);
    gf_hot_reload_watch(&app->hot_reload, "shaders/mesh.frag.glsl", on_asset_changed, app);
    gf_hot_reload_watch(&app->hot_reload, "shaders/cable.frag.glsl", on_asset_changed, app);

    app->running = true;
    app->initialized = true;
    return true;
}

void gf_app_destroy(GFApp *app) {
    if (!app || !app->initialized) return;
    gf_lua_destroy(&app->lua);
    gf_gl_renderer_destroy(&app->renderer);
    gf_synth_destroy(&app->synth);
    app->running = false;
    app->initialized = false;
}

void gf_app_resize(GFApp *app, int w, int h) {
    if (!app || w <= 0 || h <= 0) return;
    app->width = w;
    app->height = h;
    app->lua.viewport_w = w;
    app->lua.viewport_h = h;
}

void gf_app_step(GFApp *app, float dt, bool render_offline_audio) {
    if (!app || !app->initialized) return;

    if (render_offline_audio) {
        float tmp[1024];
        gf_synth_render_block(&app->synth, tmp, 512);
    }

    if ((app->frame_count % 30u) == 0u) {
        gf_hot_reload_poll(&app->hot_reload);
    }

    GFAudioTelemetry tel;
    gf_synth_get_telemetry(&app->synth, &tel);
    gf_lua_update(&app->lua, dt, &tel);
    gf_scene_step_camera(&app->scene, dt);
    app->scene.active_plan = gf_synth_active_plan(&app->synth);

    if (!app->xr_active) {
        gf_lua_render_ui(&app->lua, app->width, app->height);
        GFMat4 view, proj;
        float aspect = (float)app->width / (float)(app->height > 0 ? app->height : 1);
        gf_scene_compute_matrices(&app->scene, aspect, &view, &proj);
        gf_gl_renderer_render_view(&app->renderer, &app->scene, &tel, &view, &proj,
                                   0, 0, app->width, app->height, true);
        gf_gl_renderer_render_hud(&app->renderer, &app->hud, app->width, app->height);
    }
    app->frame_count++;
}

void gf_app_handle_key(GFApp *app, int ascii_char, bool pressed) {
    if (!app || !app->initialized) return;
    int lower = (ascii_char >= 'A' && ascii_char <= 'Z') ? tolower(ascii_char) : ascii_char;
    if (gf_app_is_nav_key(lower)) {
        gf_lua_on_input(&app->lua, "nav_key", lower, 0, 0, pressed ? 1.0f : 0.0f);
        return;
    }
    int note = gf_app_map_ascii_to_midi(lower);
    if (note >= 0) {
        gf_lua_on_input(&app->lua, pressed ? "note_on" : "note_off", note, 0, 0, pressed ? 0.85f : 0.0f);
    }
}

void gf_app_handle_pointer_ex(GFApp *app, const char *type, int id, float x, float y, float extra) {
    if (!app || !app->initialized || !type) return;
    gf_lua_on_input(&app->lua, type, id, x, y, extra);
}

void gf_app_handle_pointer(GFApp *app, const char *type, int id, float x, float y) {
    gf_app_handle_pointer_ex(app, type, id, x, y, 0.0f);
}

void gf_app_handle_midi(GFApp *app, uint8_t status, uint8_t data1, uint8_t data2) {
    if (!app || !app->initialized) return;
    uint8_t cmd = status & 0xF0u;
    if (cmd == 0x90u && data2 > 0) {
        gf_lua_on_input(&app->lua, "note_on", (int)data1, 0, 0, (float)data2 / 127.0f);
    } else if (cmd == 0x80u || (cmd == 0x90u && data2 == 0)) {
        gf_lua_on_input(&app->lua, "note_off", (int)data1, 0, 0, 0.0f);
    } else if (cmd == 0xB0u) {
        float norm = (float)data2 / 127.0f;
        if (data1 == 1) gf_synth_set_param(&app->synth, GF_P_XY_X, norm);
        else if (data1 == 74) gf_synth_set_param(&app->synth, GF_P_MASTER_FILTER_CUTOFF, 80.0f + norm * 15000.0f);
        else if (data1 == 91) gf_synth_set_param(&app->synth, GF_P_AETHER_MIX, norm);
        gf_spatial_sync_from_synth(&app->scene.spatial, &app->synth);
    }
}

void gf_app_set_input_mode(GFApp *app, GFInputMode mode) {
    if (!app || !app->initialized) return;
    gf_lua_set_input_mode(&app->lua, mode);
}

bool gf_app_trigger_reload(GFApp *app) {
    if (!app || !app->initialized) return false;
    bool sh_ok = gf_gl_renderer_reload_shaders(&app->renderer);
    bool lua_ok = gf_lua_reload(&app->lua);
    return sh_ok && lua_ok;
}

void gf_app_xr_begin(GFApp *app, float dt) {
    if (!app || !app->initialized) return;
    app->xr_active = true;
    if (app->lua.input_mode != GF_INPUT_XR) {
        gf_lua_set_input_mode(&app->lua, GF_INPUT_XR);
    }
    GFAudioTelemetry tel;
    gf_synth_get_telemetry(&app->synth, &tel);
    gf_lua_update(&app->lua, dt, &tel);
    gf_scene_step_camera(&app->scene, dt);
    app->scene.active_plan = gf_synth_active_plan(&app->synth);
    gf_lua_render_ui(&app->lua, 960, 720);
}

void gf_app_xr_render_eye(GFApp *app, const float *view_m16, const float *proj_m16,
                          int vp_x, int vp_y, int vp_w, int vp_h) {
    if (!app || !app->initialized || !view_m16 || !proj_m16) return;
    GFMat4 view, proj;
    memcpy(view.m, view_m16, sizeof(view.m));
    memcpy(proj.m, proj_m16, sizeof(proj.m));
    GFAudioTelemetry tel;
    gf_synth_get_telemetry(&app->synth, &tel);
    gf_gl_renderer_render_xr_eye(&app->renderer, &app->scene, &app->hud, &tel,
                                 &view, &proj, vp_x, vp_y, vp_w, vp_h);
}

void gf_app_xr_controller(GFApp *app, int ctrl_id, float trigger_value) {
    if (!app || !app->initialized) return;
    gf_lua_on_input(&app->lua, "xr_trigger", ctrl_id, 0, 0, trigger_value);
}

void gf_app_xr_controller_ray(GFApp *app, int ctrl_id,
                              float ox, float oy, float oz,
                              float dx, float dy, float dz,
                              float trigger_value, int teleport_room) {
    if (!app || !app->initialized) return;
    if (teleport_room >= 1 && teleport_room <= 7) {
        gf_lua_on_xr_ray(&app->lua, "xr_teleport", teleport_room, gf_vec3(0, 1.5f, 0), gf_vec3(0, 0, -1), 1.0f);
        return;
    }

    /* Transform XR local stage ray into active room world coordinates */
    const GFCameraRig *cam = &app->scene.camera;
    float cy = cosf(cam->yaw), sy = sinf(cam->yaw);
    GFVec3 w_orig = gf_vec3(
        cam->pos.x + ox * cy + oz * sy,
        oy,
        cam->pos.z - ox * sy + oz * cy
    );
    GFVec3 w_dir = gf_vec3_normalize(gf_vec3(
        dx * cy + dz * sy,
        dy,
        -dx * sy + dz * cy
    ));

    int slot = (ctrl_id >= 0 && ctrl_id < 4) ? ctrl_id : 0;
    bool pressed = (trigger_value > 0.45f);
    bool was_pressed = app->xr_trig_held[slot];
    app->xr_trig_held[slot] = pressed;

    const char *ev_type = "xr_move";
    if (pressed && !was_pressed) ev_type = "xr_down";
    else if (!pressed && was_pressed) ev_type = "xr_up";

    gf_lua_on_xr_ray(&app->lua, ev_type, ctrl_id, w_orig, w_dir, trigger_value);
}

void gf_app_xr_end(GFApp *app) {
    if (!app || !app->initialized) return;
    app->xr_frame_count++;
}

bool gf_app_write_ppm(const char *path, int w, int h, const uint8_t *rgba_bottom_up) {
    if (!path || !rgba_bottom_up || w <= 0 || h <= 0) return false;
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;
    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; --y) {
        for (int x = 0; x < w; ++x) {
            const uint8_t *px = rgba_bottom_up + ((size_t)y * (size_t)w + (size_t)x) * 4u;
            fwrite(px, 1, 3, fp);
        }
    }
    fclose(fp);
    return true;
}

static void put_u16_le(FILE *fp, uint16_t v) {
    uint8_t b[2] = {(uint8_t)(v & 0xFFu), (uint8_t)(v >> 8)};
    fwrite(b, 1, 2, fp);
}

static void put_u32_le(FILE *fp, uint32_t v) {
    uint8_t b[4] = {
        (uint8_t)(v & 0xFFu), (uint8_t)((v >> 8) & 0xFFu),
        (uint8_t)((v >> 16) & 0xFFu), (uint8_t)((v >> 24) & 0xFFu)
    };
    fwrite(b, 1, 4, fp);
}

bool gf_app_write_wav(const char *path, uint32_t sample_rate, const float *interleaved_lr, uint32_t frames) {
    if (!path || !interleaved_lr || frames == 0 || sample_rate == 0) return false;
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;
    uint32_t data_bytes = frames * 2u * 2u;
    fwrite("RIFF", 1, 4, fp);
    put_u32_le(fp, 36u + data_bytes);
    fwrite("WAVEfmt ", 1, 8, fp);
    put_u32_le(fp, 16u);
    put_u16_le(fp, 1u);
    put_u16_le(fp, 2u);
    put_u32_le(fp, sample_rate);
    put_u32_le(fp, sample_rate * 4u);
    put_u16_le(fp, 4u);
    put_u16_le(fp, 16u);
    fwrite("data", 1, 4, fp);
    put_u32_le(fp, data_bytes);

    for (uint32_t i = 0; i < frames * 2u; ++i) {
        float s = gf_clampf(interleaved_lr[i], -1.0f, 1.0f);
        int16_t pcm = (int16_t)(s * 32767.0f);
        put_u16_le(fp, (uint16_t)pcm);
    }
    fclose(fp);
    return true;
}

bool gf_app_run_e2e_script(GFApp *app, const char *script_path) {
    if (!app || !app->initialized || !script_path) return false;
    FILE *fp = fopen(script_path, "rb");
    if (!fp) {
        GF_LOGE("e2e", "Cannot open E2E script: %s", script_path);
        return false;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        char cmd[32] = {0};
        if (sscanf(line, "%31s", cmd) != 1 || cmd[0] == '#') continue;

        if (strcmp(cmd, "NOTE_ON") == 0) {
            int note = 60; float vel = 0.85f;
            sscanf(line, "%*s %d %f", &note, &vel);
            gf_lua_on_input(&app->lua, "note_on", note, 0, 0, vel);
        } else if (strcmp(cmd, "NOTE_OFF") == 0) {
            int note = 60;
            sscanf(line, "%*s %d", &note);
            gf_lua_on_input(&app->lua, "note_off", note, 0, 0, 0.0f);
        } else if (strcmp(cmd, "XY") == 0) {
            float x = 0.5f, y = 0.5f, z = 0.5f;
            int n = sscanf(line, "%*s %f %f %f", &x, &y, &z);
            if (n >= 3) gf_synth_set_xyz(&app->synth, x, y, z);
            else gf_synth_set_xy(&app->synth, x, y);
            gf_spatial_sync_from_synth(&app->scene.spatial, &app->synth);
        } else if (strcmp(cmd, "VOXELS") == 0) {
            int en = 1;
            sscanf(line, "%*s %d", &en);
            app->scene.voxels_enabled = (en != 0);
        } else if (strcmp(cmd, "MODE") == 0) {
            char mode_s[32] = {0};
            sscanf(line, "%*s %31s", mode_s);
            if (strcmp(mode_s, "touch") == 0) gf_app_set_input_mode(app, GF_INPUT_TOUCH);
            else if (strcmp(mode_s, "xr") == 0) gf_app_set_input_mode(app, GF_INPUT_XR);
            else gf_app_set_input_mode(app, GF_INPUT_DESKTOP);
        } else if (strcmp(cmd, "TELEPORT") == 0) {
            int rm = 6;
            sscanf(line, "%*s %d", &rm);
            gf_camera_teleport_room(&app->scene.camera, rm);
            gf_scene_step_camera(&app->scene, 0.0f);
        } else if (strcmp(cmd, "WALK") == 0) {
            float fwd = 1.0f, rgt = 0.0f; int steps = 10;
            sscanf(line, "%*s %f %f %d", &fwd, &rgt, &steps);
            for (int i = 0; i < steps; ++i) {
                gf_camera_walk(&app->scene.camera, fwd, rgt, 0.05f);
                gf_app_step(app, 0.05f, true);
            }
        } else if (strcmp(cmd, "FOCUS_RACK") == 0) {
            int rk_id = 0;
            sscanf(line, "%*s %d", &rk_id);
            const GFRack3D *rk = gf_spatial_get_rack(&app->scene.spatial, rk_id);
            if (rk) {
                GFVec3 r, u, f;
                gf_rack_basis(rk, &r, &u, &f);
                gf_camera_focus_rack(&app->scene.camera, rk_id, rk->pos, f, rk->width, rk->height,
                                     (float)app->width / (float)app->height);
            }
        } else if (strcmp(cmd, "EXIT_FOCUS") == 0) {
            gf_camera_exit_focus(&app->scene.camera);
        } else if (strcmp(cmd, "POINTER") == 0) {
            char pkind[16] = {0};
            float px = 0.0f, py = 0.0f, pex = 0.0f;
            sscanf(line, "%*s %15s %f %f %f", pkind, &px, &py, &pex);
            const char *ev = (strcmp(pkind, "down") == 0) ? "pointer_down"
                           : (strcmp(pkind, "move") == 0) ? "pointer_move" : "pointer_up";
            gf_app_handle_pointer_ex(app, ev, 1, px, py, pex);
        } else if (strcmp(cmd, "SET_PARAM") == 0) {
            char pname[64] = {0}; float pval = 0.0f;
            if (sscanf(line, "%*s %63s %f", pname, &pval) == 2) {
                int pid = gf_synth_param_from_name(pname);
                if (pid >= 0) {
                    gf_synth_set_param(&app->synth, (GFParamId)pid, pval);
                    gf_spatial_sync_from_synth(&app->scene.spatial, &app->synth);
                }
            }
        } else if (strcmp(cmd, "CONNECT") == 0) {
            char s_src[64] = {0}, s_dst[64] = {0}; float gain = 0.8f;
            if (sscanf(line, "%*s %63s %63s %f", s_src, s_dst, &gain) >= 2) {
                int p0 = gf_port_from_name(s_src);
                int p1 = gf_port_from_name(s_dst);
                if (p0 >= 0 && p1 >= 0) gf_synth_connect_cable(&app->synth, (GFPortId)p0, (GFPortId)p1, gain);
            }
        } else if (strcmp(cmd, "DISCONNECT_PORT") == 0) {
            char s_p[64] = {0};
            if (sscanf(line, "%*s %63s", s_p) == 1) {
                int pid = gf_port_from_name(s_p);
                if (pid >= 0) {
                    gf_patchbay_disconnect_port(&app->synth.patchbay, (GFPortId)pid);
                    /* Recompile & publish updated plan */
                    int cur = atomic_load_explicit(&app->synth.active_plan_idx, memory_order_relaxed);
                    int nxt = 1 - cur;
                    gf_patchbay_compile(&app->synth.patchbay, &app->synth.plans[nxt]);
                    atomic_store_explicit(&app->synth.active_plan_idx, nxt, memory_order_release);
                }
            }
        } else if (strcmp(cmd, "MUTATE") == 0) {
            float d = 0.35f, r = 0.5f;
            sscanf(line, "%*s %f %f", &d, &r);
            gf_lua_call_mutator(&app->lua, "mutate", d, r);
        } else if (strcmp(cmd, "PRESET") == 0) {
            char rpath[192] = {0};
            if (sscanf(line, "%*s %191s", rpath) == 1) {
                gf_lua_apply_preset(&app->lua, rpath);
            }
        } else if (strcmp(cmd, "MELODY") == 0) {
            char mpath[192] = {0};
            if (sscanf(line, "%*s %191s", mpath) == 1) {
                gf_lua_apply_melody(&app->lua, mpath);
            }
        } else if (strcmp(cmd, "LOOPER") == 0) {
            char lact[64] = {0};
            if (sscanf(line, "%*s %63s", lact) == 1) {
                gf_lua_looper_action(&app->lua, lact);
            }
        } else if (strcmp(cmd, "STEP") == 0) {
            int frames = 1;
            sscanf(line, "%*s %d", &frames);
            for (int i = 0; i < frames; ++i) gf_app_step(app, 0.016f, true);
        } else if (strcmp(cmd, "RELOAD") == 0) {
            gf_app_trigger_reload(app);
        } else if (strcmp(cmd, "SAVE_PPM") == 0) {
            char path[256] = {0};
            sscanf(line, "%*s %255s", path);
            uint8_t *px = (uint8_t *)malloc((size_t)app->width * (size_t)app->height * 4u);
            if (px) {
                if (gf_gl_renderer_read_rgba(&app->renderer, app->width, app->height, px)) {
                    gf_app_write_ppm(path, app->width, app->height, px);
                }
                free(px);
            }
        } else if (strcmp(cmd, "SAVE_WAV") == 0) {
            char path[256] = {0};
            int frames = 8192;
            sscanf(line, "%*s %255s %d", path, &frames);
            if (frames < 256) frames = 256;
            float *buf = (float *)malloc((size_t)frames * 2u * sizeof(float));
            if (buf) {
                gf_synth_render_block(&app->synth, buf, frames);
                gf_app_write_wav(path, (uint32_t)app->synth.sample_rate, buf, (uint32_t)frames);
                free(buf);
            }
        }
    }
    fclose(fp);
    return true;
}

int gf_app_get_status_json(const GFApp *app, char *out_buf, size_t out_cap) {
    if (!app || !out_buf || out_cap < 32) return 0;
    GFAudioTelemetry tel;
    memset(&tel, 0, sizeof(tel));
    int cable_cnt = 0;
    if (app->initialized) {
        gf_synth_get_telemetry(&app->synth, &tel);
        const GFPatchPlan *pl = gf_synth_active_plan(&app->synth);
        if (pl) cable_cnt = pl->cable_count;
    }
    return snprintf(out_buf, out_cap,
        "{\"mode\":\"%s\",\"frames\":%llu,\"xrFrames\":%llu,\"activeVoices\":%d,"
        "\"rmsL\":%.4f,\"rmsR\":%.4f,\"voxelsEnabled\":%s,\"drawCalls\":%u,\"reloadCount\":%d,"
        "\"room\":%d,\"camMode\":\"%s\",\"focusedRack\":%d,\"cableCount\":%d,"
        "\"yaw\":%.4f,\"pitch\":%.4f,"
        "\"looperState\":\"%s\",\"melody\":\"%s\",\"loopEvents\":%d,"
        "\"visibleRooms\":%d,\"droneLatch\":%s,\"preset\":\"%s\",\"realm\":\"%s\",\"worldTheme\":\"%s\"}",
        gf_input_mode_name(app->lua.input_mode),
        (unsigned long long)app->frame_count,
        (unsigned long long)app->xr_frame_count,
        tel.active_voices,
        tel.rms_l, tel.rms_r,
        app->scene.voxels_enabled ? "true" : "false",
        app->renderer.last_draw_calls,
        app->lua.reload_count,
        app->scene.camera.current_room,
        app->scene.camera.mode == GF_CAM_RACK_FOCUS ? "focus" : "walk",
        app->scene.camera.focused_rack_id,
        cable_cnt,
        app->scene.camera.yaw,
        app->scene.camera.pitch,
        app->lua.looper_state,
        app->lua.active_melody_name,
        app->lua.looper_event_count,
        app->renderer.last_visible_rooms,
        app->synth.drone_latch ? "true" : "false",
        app->lua.active_preset_name,
        app->lua.active_preset_realm,
        app->lua.active_world_theme);
}
