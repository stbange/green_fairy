#include "platform/sdl_runtime.h"

#include "core/log.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten.h>
#endif

static GFApp g_app;
static SDL_Window *g_window = NULL;
static SDL_GLContext g_gl_ctx = NULL;
static SDL_AudioStream *g_audio_stream = NULL;
static int g_max_frames = 0;

static bool load_gl_api(GFGlApi *api) {
    memset(api, 0, sizeof(*api));
#ifdef __EMSCRIPTEN__
    api->CreateShader = glCreateShader;
    api->ShaderSource = glShaderSource;
    api->CompileShader = glCompileShader;
    api->GetShaderiv = glGetShaderiv;
    api->GetShaderInfoLog = glGetShaderInfoLog;
    api->DeleteShader = glDeleteShader;
    api->CreateProgram = glCreateProgram;
    api->AttachShader = glAttachShader;
    api->LinkProgram = glLinkProgram;
    api->GetProgramiv = glGetProgramiv;
    api->GetProgramInfoLog = glGetProgramInfoLog;
    api->DeleteProgram = glDeleteProgram;
    api->UseProgram = glUseProgram;
    api->GetUniformLocation = glGetUniformLocation;
    api->Uniform1f = glUniform1f;
    api->Uniform1i = glUniform1i;
    api->Uniform2f = glUniform2f;
    api->Uniform3f = glUniform3f;
    api->Uniform4f = glUniform4f;
    api->UniformMatrix4fv = glUniformMatrix4fv;
    api->GenVertexArrays = glGenVertexArrays;
    api->BindVertexArray = glBindVertexArray;
    api->DeleteVertexArrays = glDeleteVertexArrays;
    api->GenBuffers = glGenBuffers;
    api->BindBuffer = glBindBuffer;
    api->BufferData = glBufferData;
    api->DeleteBuffers = glDeleteBuffers;
    api->EnableVertexAttribArray = glEnableVertexAttribArray;
    api->VertexAttribPointer = glVertexAttribPointer;
    api->GenTextures = glGenTextures;
    api->BindTexture = glBindTexture;
    api->TexImage2D = glTexImage2D;
    api->TexParameteri = glTexParameteri;
    api->DeleteTextures = glDeleteTextures;
    api->ActiveTexture = glActiveTexture;
    api->Viewport = glViewport;
    api->Enable = glEnable;
    api->Disable = glDisable;
    api->BlendFunc = glBlendFunc;
    api->ClearColor = glClearColor;
    api->Clear = glClear;
    api->DrawElements = glDrawElements;
    api->DrawArrays = glDrawArrays;
    api->ReadPixels = glReadPixels;
    return true;
#else
#define LOAD_GL(field, sym) do { \
    SDL_FunctionPointer _fp = SDL_GL_GetProcAddress(sym); \
    if (!_fp) { GF_LOGE("sdl", "Missing GL symbol: %s", sym); return false; } \
    memcpy(&api->field, &_fp, sizeof(_fp)); \
} while (0)
    LOAD_GL(CreateShader, "glCreateShader");
    LOAD_GL(ShaderSource, "glShaderSource");
    LOAD_GL(CompileShader, "glCompileShader");
    LOAD_GL(GetShaderiv, "glGetShaderiv");
    LOAD_GL(GetShaderInfoLog, "glGetShaderInfoLog");
    LOAD_GL(DeleteShader, "glDeleteShader");
    LOAD_GL(CreateProgram, "glCreateProgram");
    LOAD_GL(AttachShader, "glAttachShader");
    LOAD_GL(LinkProgram, "glLinkProgram");
    LOAD_GL(GetProgramiv, "glGetProgramiv");
    LOAD_GL(GetProgramInfoLog, "glGetProgramInfoLog");
    LOAD_GL(DeleteProgram, "glDeleteProgram");
    LOAD_GL(UseProgram, "glUseProgram");
    LOAD_GL(GetUniformLocation, "glGetUniformLocation");
    LOAD_GL(Uniform1f, "glUniform1f");
    LOAD_GL(Uniform1i, "glUniform1i");
    LOAD_GL(Uniform2f, "glUniform2f");
    LOAD_GL(Uniform3f, "glUniform3f");
    LOAD_GL(Uniform4f, "glUniform4f");
    LOAD_GL(UniformMatrix4fv, "glUniformMatrix4fv");
    LOAD_GL(GenVertexArrays, "glGenVertexArrays");
    LOAD_GL(BindVertexArray, "glBindVertexArray");
    LOAD_GL(DeleteVertexArrays, "glDeleteVertexArrays");
    LOAD_GL(GenBuffers, "glGenBuffers");
    LOAD_GL(BindBuffer, "glBindBuffer");
    LOAD_GL(BufferData, "glBufferData");
    LOAD_GL(DeleteBuffers, "glDeleteBuffers");
    LOAD_GL(EnableVertexAttribArray, "glEnableVertexAttribArray");
    LOAD_GL(VertexAttribPointer, "glVertexAttribPointer");
    LOAD_GL(GenTextures, "glGenTextures");
    LOAD_GL(BindTexture, "glBindTexture");
    LOAD_GL(TexImage2D, "glTexImage2D");
    LOAD_GL(TexParameteri, "glTexParameteri");
    LOAD_GL(DeleteTextures, "glDeleteTextures");
    LOAD_GL(ActiveTexture, "glActiveTexture");
    LOAD_GL(Viewport, "glViewport");
    LOAD_GL(Enable, "glEnable");
    LOAD_GL(Disable, "glDisable");
    LOAD_GL(BlendFunc, "glBlendFunc");
    LOAD_GL(ClearColor, "glClearColor");
    LOAD_GL(Clear, "glClear");
    LOAD_GL(DrawElements, "glDrawElements");
    LOAD_GL(DrawArrays, "glDrawArrays");
    LOAD_GL(ReadPixels, "glReadPixels");
#undef LOAD_GL
    return true;
#endif
}

static void SDLCALL audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    (void)total_amount;
    GFApp *app = (GFApp *)userdata;
    if (!app || !app->initialized || additional_amount <= 0) return;

    float buf[512 * 2];
    int remaining = additional_amount / (int)(2 * sizeof(float));
    while (remaining > 0) {
        int chunk = remaining > 512 ? 512 : remaining;
        gf_synth_render_block(&app->synth, buf, chunk);
        SDL_PutAudioStreamData(stream, buf, chunk * 2 * (int)sizeof(float));
        remaining -= chunk;
    }
}

static void pump_sdl_events(void) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
#ifdef __EMSCRIPTEN__
        /* On Web, bridge.js owns DOM touch/mouse/key/resize input to avoid duplicate conflicting events */
        if (ev.type == SDL_EVENT_QUIT) {
            g_app.running = false;
        }
#else
        switch (ev.type) {
            case SDL_EVENT_QUIT:
                g_app.running = false;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                gf_app_resize(&g_app, ev.window.data1, ev.window.data2);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (ev.key.key == SDLK_F5) {
                    gf_app_trigger_reload(&g_app);
                } else if (!ev.key.repeat) {
                    if (ev.key.key == SDLK_ESCAPE) gf_app_handle_key(&g_app, 27, true);
                    else if (ev.key.key == SDLK_UP) gf_app_handle_key(&g_app, 'w', true);
                    else if (ev.key.key == SDLK_DOWN) gf_app_handle_key(&g_app, 's', true);
                    else if (ev.key.key == SDLK_LEFT) gf_app_handle_key(&g_app, 'a', true);
                    else if (ev.key.key == SDLK_RIGHT) gf_app_handle_key(&g_app, 'd', true);
                    else if (ev.key.key < 128) gf_app_handle_key(&g_app, (int)ev.key.key, true);
                }
                break;
            case SDL_EVENT_KEY_UP:
                if (ev.key.key == SDLK_UP) gf_app_handle_key(&g_app, 'w', false);
                else if (ev.key.key == SDLK_DOWN) gf_app_handle_key(&g_app, 's', false);
                else if (ev.key.key == SDLK_LEFT) gf_app_handle_key(&g_app, 'a', false);
                else if (ev.key.key == SDLK_RIGHT) gf_app_handle_key(&g_app, 'd', false);
                else if (ev.key.key < 128) gf_app_handle_key(&g_app, (int)ev.key.key, false);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (ev.button.which == SDL_TOUCH_MOUSEID) break;
                float ex = (ev.button.button == SDL_BUTTON_RIGHT) ? 2.0f
                         : ((SDL_GetModState() & SDL_KMOD_SHIFT) ? 1.0f : 0.0f);
                gf_app_handle_pointer_ex(&g_app, "pointer_down", 0, ev.button.x, ev.button.y, ex);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (ev.button.which == SDL_TOUCH_MOUSEID) break;
                gf_app_handle_pointer_ex(&g_app, "pointer_up", 0, ev.button.x, ev.button.y, 0.0f);
                break;
            case SDL_EVENT_MOUSE_MOTION: {
                if (ev.motion.which == SDL_TOUCH_MOUSEID) break;
                float ex = (ev.motion.state & SDL_BUTTON_RMASK) ? 2.0f : 0.0f;
                gf_app_handle_pointer_ex(&g_app, "pointer_move", 0, ev.motion.x, ev.motion.y, ex);
                break;
            }
            case SDL_EVENT_FINGER_DOWN:
                gf_app_handle_pointer(&g_app, "pointer_down", (int)(ev.tfinger.fingerID & 0x7FFF) + 10,
                                      ev.tfinger.x * (float)g_app.width, ev.tfinger.y * (float)g_app.height);
                break;
            case SDL_EVENT_FINGER_MOTION:
                gf_app_handle_pointer(&g_app, "pointer_move", (int)(ev.tfinger.fingerID & 0x7FFF) + 10,
                                      ev.tfinger.x * (float)g_app.width, ev.tfinger.y * (float)g_app.height);
                break;
            case SDL_EVENT_FINGER_UP:
                gf_app_handle_pointer(&g_app, "pointer_up", (int)(ev.tfinger.fingerID & 0x7FFF) + 10,
                                      ev.tfinger.x * (float)g_app.width, ev.tfinger.y * (float)g_app.height);
                break;
            default:
                break;
        }
#endif
    }
}

static void frame_tick(void) {
    pump_sdl_events();
    if (!g_app.running) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }
    bool offline = (g_audio_stream == NULL);
    gf_app_step(&g_app, 1.0f / 60.0f, offline);
    if (!g_app.xr_active) {
        SDL_GL_SwapWindow(g_window);
    }
    if (g_max_frames > 0 && (int)g_app.frame_count >= g_max_frames) {
        g_app.running = false;
    }
}

#ifdef __EMSCRIPTEN__
static char g_status_json[1024];

EMSCRIPTEN_KEEPALIVE int gf_web_start(const char *assets_dir, int w, int h, int initial_mode) {
    GFAppConfig cfg;
    gf_app_config_default(&cfg);
    cfg.assets_dir = assets_dir ? assets_dir : "/assets";
    cfg.width = w > 0 ? w : 1280;
    cfg.height = h > 0 ? h : 720;
    cfg.initial_mode = (GFInputMode)initial_mode;
    return gf_sdl_run(&cfg);
}

EMSCRIPTEN_KEEPALIVE void gf_platform_set_input_mode(int mode) {
    gf_app_set_input_mode(&g_app, (GFInputMode)mode);
}

EMSCRIPTEN_KEEPALIVE void gf_web_resize(int w, int h) {
    if (g_window) SDL_SetWindowSize(g_window, w, h);
    gf_app_resize(&g_app, w, h);
}

EMSCRIPTEN_KEEPALIVE void gf_web_pointer(int kind, int id, float x, float y) {
    const char *t = (kind == 0) ? "pointer_down" : ((kind == 1) ? "pointer_move" : "pointer_up");
    gf_app_handle_pointer(&g_app, t, id, x, y);
}

EMSCRIPTEN_KEEPALIVE void gf_web_pointer_ex(int kind, int id, float x, float y, float extra) {
    const char *t = (kind == 0) ? "pointer_down" : ((kind == 1) ? "pointer_move" : "pointer_up");
    gf_app_handle_pointer_ex(&g_app, t, id, x, y, extra);
}

EMSCRIPTEN_KEEPALIVE void gf_web_key(int ascii_char, int pressed) {
    gf_app_handle_key(&g_app, ascii_char, pressed != 0);
}

EMSCRIPTEN_KEEPALIVE void gf_web_note_on(int note, float vel) {
    gf_lua_on_input(&g_app.lua, "note_on", note, 0, 0, vel);
}

EMSCRIPTEN_KEEPALIVE void gf_web_note_off(int note) {
    gf_lua_on_input(&g_app.lua, "note_off", note, 0, 0, 0.0f);
}

EMSCRIPTEN_KEEPALIVE void gf_web_set_xy(float x, float y) {
    gf_synth_set_xy(&g_app.synth, x, y);
    gf_spatial_sync_from_synth(&g_app.scene.spatial, &g_app.synth);
}

EMSCRIPTEN_KEEPALIVE void gf_web_set_voxels(int enabled) {
    g_app.scene.voxels_enabled = (enabled != 0);
}

EMSCRIPTEN_KEEPALIVE void gf_web_teleport_room(int room_id) {
    gf_camera_teleport_room(&g_app.scene.camera, room_id);
    gf_scene_step_camera(&g_app.scene, 0.0f);
}

EMSCRIPTEN_KEEPALIVE void gf_web_focus_rack(int rack_id) {
    const GFRack3D *rk = gf_spatial_get_rack(&g_app.scene.spatial, rack_id);
    if (rk) {
        GFVec3 r, u, f;
        gf_rack_basis(rk, &r, &u, &f);
        gf_camera_focus_rack(&g_app.scene.camera, rack_id, rk->pos, f, rk->width, rk->height,
                             (float)g_app.width / (float)(g_app.height > 0 ? g_app.height : 1));
    }
}

EMSCRIPTEN_KEEPALIVE void gf_web_exit_focus(void) {
    gf_camera_exit_focus(&g_app.scene.camera);
}

EMSCRIPTEN_KEEPALIVE int gf_web_apply_preset_path(const char *rel_path) {
    return gf_lua_apply_preset(&g_app.lua, rel_path) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int gf_web_connect_cable(const char *src_name, const char *dst_name, float gain) {
    int p0 = gf_port_from_name(src_name);
    int p1 = gf_port_from_name(dst_name);
    if (p0 < 0 || p1 < 0) return 0;
    return gf_synth_connect_cable(&g_app.synth, (GFPortId)p0, (GFPortId)p1, gain);
}

EMSCRIPTEN_KEEPALIVE int gf_web_disconnect_port(const char *port_name) {
    int pid = gf_port_from_name(port_name);
    if (pid < 0) return 0;
    int rem = gf_patchbay_disconnect_port(&g_app.synth.patchbay, (GFPortId)pid);
    int cur = atomic_load_explicit(&g_app.synth.active_plan_idx, memory_order_relaxed);
    int nxt = 1 - cur;
    gf_patchbay_compile(&g_app.synth.patchbay, &g_app.synth.plans[nxt]);
    atomic_store_explicit(&g_app.synth.active_plan_idx, nxt, memory_order_release);
    return rem;
}

EMSCRIPTEN_KEEPALIVE void gf_web_mutator(const char *action, float depth, float randomness) {
    gf_lua_call_mutator(&g_app.lua, action ? action : "mutate", depth, randomness);
}

EMSCRIPTEN_KEEPALIVE void gf_web_resume_audio(void) {
    if (g_audio_stream) {
        SDL_ResumeAudioStreamDevice(g_audio_stream);
    }
}

EMSCRIPTEN_KEEPALIVE void gf_webxr_begin_frame(float dt) {
    gf_app_xr_begin(&g_app, dt > 0.0f ? dt : 0.011f);
}

EMSCRIPTEN_KEEPALIVE void gf_webxr_render_eye(const float *view_m, const float *proj_m, int vx, int vy, int vw, int vh) {
    gf_app_xr_render_eye(&g_app, view_m, proj_m, vx, vy, vw, vh);
}

EMSCRIPTEN_KEEPALIVE void gf_webxr_controller_input(int ctrl_id, float trigger) {
    gf_app_xr_controller(&g_app, ctrl_id, trigger);
}

EMSCRIPTEN_KEEPALIVE void gf_webxr_controller_ray(int ctrl_id, float ox, float oy, float oz,
                                                   float dx, float dy, float dz,
                                                   float trigger, int teleport_room) {
    gf_app_xr_controller_ray(&g_app, ctrl_id, ox, oy, oz, dx, dy, dz, trigger, teleport_room);
}

EMSCRIPTEN_KEEPALIVE void gf_webxr_end_frame(void) {
    gf_app_xr_end(&g_app);
}

EMSCRIPTEN_KEEPALIVE void gf_webxr_exit(void) {
    g_app.xr_active = false;
}

EMSCRIPTEN_KEEPALIVE const char *gf_web_get_status_json(void) {
    gf_app_get_status_json(&g_app, g_status_json, sizeof(g_status_json));
    return g_status_json;
}
#endif

int gf_sdl_run(const GFAppConfig *cfg) {
    if (!cfg) return 1;
    g_max_frames = cfg->max_frames;

    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        GF_LOGE("sdl", "SDL_Init(VIDEO) failed: %s", SDL_GetError());
        return 1;
    }
    if (!cfg->headless && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        GF_LOGW("sdl", "SDL audio subsystem unavailable (%s); continuing offline", SDL_GetError());
    }

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    bool is_gles = true;
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    bool is_gles = false;
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    Uint64 win_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (cfg->headless) win_flags |= SDL_WINDOW_HIDDEN;

    g_window = SDL_CreateWindow("Green Fairy Synthesizer", cfg->width, cfg->height, win_flags);
    if (!g_window) {
        GF_LOGE("sdl", "SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    g_gl_ctx = SDL_GL_CreateContext(g_window);
    if (!g_gl_ctx) {
        GF_LOGE("sdl", "SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 1;
    }

    GFGlApi gl_api;
    if (!load_gl_api(&gl_api) || !gf_app_init(&g_app, cfg, &gl_api, is_gles)) {
        SDL_GL_DestroyContext(g_gl_ctx);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 1;
    }

    if (!cfg->headless) {
        SDL_AudioSpec spec;
        memset(&spec, 0, sizeof(spec));
        spec.format = SDL_AUDIO_F32;
        spec.channels = 2;
        spec.freq = 44100;
        g_audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, &g_app);
        if (g_audio_stream) {
            SDL_ResumeAudioStreamDevice(g_audio_stream);
        } else {
            GF_LOGW("sdl", "Audio device unavailable (%s); continuing offline", SDL_GetError());
        }
    }

    if (cfg->e2e_script) {
        bool ok = gf_app_run_e2e_script(&g_app, cfg->e2e_script);
        gf_app_destroy(&g_app);
        if (g_audio_stream) SDL_DestroyAudioStream(g_audio_stream);
        SDL_GL_DestroyContext(g_gl_ctx);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return ok ? 0 : 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame_tick, 0, 0);
    return 0;
#else
    while (g_app.running) {
        frame_tick();
        if (!cfg->headless) SDL_Delay(16);
    }
    gf_app_destroy(&g_app);
    if (g_audio_stream) SDL_DestroyAudioStream(g_audio_stream);
    SDL_GL_DestroyContext(g_gl_ctx);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
#endif
}
