#include "test_harness.h"

#include "platform/app.h"

/* Minimal mock GL table for pure-C platform app controller unit tests */
static GFGLuint g_mid = 0;
static GFGLuint GF_APIENTRY p_id(void) { return ++g_mid; }
static GFGLuint GF_APIENTRY p_sh(GFGLenum t) { (void)t; return ++g_mid; }
static void GF_APIENTRY p_ss(GFGLuint s, GFGLsizei c, const char *const *str, const GFGLint *l) { (void)s;(void)c;(void)str;(void)l; }
static void GF_APIENTRY p_u1(GFGLuint x) { (void)x; }
static void GF_APIENTRY p_iv(GFGLuint s, GFGLenum p, GFGLint *o) { (void)s;(void)p; *o = 1; }
static void GF_APIENTRY p_log(GFGLuint s, GFGLsizei m, GFGLsizei *l, char *b) { (void)s;(void)m;(void)l; b[0] = 0; }
static void GF_APIENTRY p_att(GFGLuint p, GFGLuint s) { (void)p;(void)s; }
static GFGLint GF_APIENTRY p_loc(GFGLuint p, const char *n) { (void)p;(void)n; return 0; }
static void GF_APIENTRY p_f1(GFGLint l, float a) { (void)l;(void)a; }
static void GF_APIENTRY p_i1(GFGLint l, GFGLint a) { (void)l;(void)a; }
static void GF_APIENTRY p_f2(GFGLint l, float a, float b) { (void)l;(void)a;(void)b; }
static void GF_APIENTRY p_f3(GFGLint l, float a, float b, float c) { (void)l;(void)a;(void)b;(void)c; }
static void GF_APIENTRY p_f4(GFGLint l, float a, float b, float c, float d) { (void)l;(void)a;(void)b;(void)c;(void)d; }
static void GF_APIENTRY p_m4(GFGLint l, GFGLsizei c, GFGLboolean t, const float *v) { (void)l;(void)c;(void)t;(void)v; }
static void GF_APIENTRY p_gen(GFGLsizei n, GFGLuint *ids) { for (GFGLsizei i = 0; i < n; ++i) ids[i] = ++g_mid; }
static void GF_APIENTRY p_del(GFGLsizei n, const GFGLuint *ids) { (void)n;(void)ids; }
static void GF_APIENTRY p_bt(GFGLenum t, GFGLuint id) { (void)t;(void)id; }
static void GF_APIENTRY p_bd(GFGLenum t, GFGLsizeiptr s, const void *d, GFGLenum u) { (void)t;(void)s;(void)d;(void)u; }
static void GF_APIENTRY p_vp(GFGLuint i, GFGLint s, GFGLenum t, GFGLboolean n, GFGLsizei st, const void *p) { (void)i;(void)s;(void)t;(void)n;(void)st;(void)p; }
static void GF_APIENTRY p_ti(GFGLenum t, GFGLint l, GFGLint i, GFGLsizei w, GFGLsizei h, GFGLint b, GFGLenum f, GFGLenum ty, const void *d) { (void)t;(void)l;(void)i;(void)w;(void)h;(void)b;(void)f;(void)ty;(void)d; }
static void GF_APIENTRY p_tp(GFGLenum t, GFGLenum p, GFGLint v) { (void)t;(void)p;(void)v; }
static void GF_APIENTRY p_vw(GFGLint x, GFGLint y, GFGLsizei w, GFGLsizei h) { (void)x;(void)y;(void)w;(void)h; }
static void GF_APIENTRY p_bf(GFGLenum s, GFGLenum d) { (void)s;(void)d; }
static void GF_APIENTRY p_cc(float r, float g, float b, float a) { (void)r;(void)g;(void)b;(void)a; }
static void GF_APIENTRY p_de(GFGLenum m, GFGLsizei c, GFGLenum t, const void *p) { (void)m;(void)c;(void)t;(void)p; }
static void GF_APIENTRY p_da(GFGLenum m, GFGLint f, GFGLsizei c) { (void)m;(void)f;(void)c; }
static void GF_APIENTRY p_rp(GFGLint x, GFGLint y, GFGLsizei w, GFGLsizei h, GFGLenum f, GFGLenum t, void *px) {
    (void)x;(void)y;(void)f;(void)t; memset(px, 0x44, (size_t)w * (size_t)h * 4u);
}

static GFGlApi make_gl(void) {
    GFGlApi api = {
        .CreateShader = p_sh, .ShaderSource = p_ss, .CompileShader = p_u1,
        .GetShaderiv = p_iv, .GetShaderInfoLog = p_log, .DeleteShader = p_u1,
        .CreateProgram = p_id, .AttachShader = p_att, .LinkProgram = p_u1,
        .GetProgramiv = p_iv, .GetProgramInfoLog = p_log, .DeleteProgram = p_u1,
        .UseProgram = p_u1, .GetUniformLocation = p_loc,
        .Uniform1f = p_f1, .Uniform1i = p_i1, .Uniform2f = p_f2, .Uniform3f = p_f3, .Uniform4f = p_f4, .UniformMatrix4fv = p_m4,
        .GenVertexArrays = p_gen, .BindVertexArray = p_u1, .DeleteVertexArrays = p_del,
        .GenBuffers = p_gen, .BindBuffer = p_bt, .BufferData = p_bd, .DeleteBuffers = p_del,
        .EnableVertexAttribArray = p_u1, .VertexAttribPointer = p_vp,
        .GenTextures = p_gen, .BindTexture = p_bt, .TexImage2D = p_ti, .TexParameteri = p_tp,
        .DeleteTextures = p_del, .ActiveTexture = p_u1,
        .Viewport = p_vw, .Enable = p_u1, .Disable = p_u1, .BlendFunc = p_bf,
        .ClearColor = p_cc, .Clear = p_u1, .DrawElements = p_de, .DrawArrays = p_da, .ReadPixels = p_rp
    };
    return api;
}

static void test_cli_parsing_and_keymap(void) {
    GFAppConfig cfg;
    gf_app_config_default(NULL);
    GF_ASSERT(!gf_app_parse_args(0, NULL, NULL));

    char *argv[] = {
        "green_fairy", "--assets", "assets", "--headless",
        "--width", "32", "--height", "32", "--frames", "10", "--mode", "touch"
    };
    GF_ASSERT(gf_app_parse_args(12, argv, &cfg));
    GF_ASSERT(cfg.headless && cfg.width == 64 && cfg.height == 64 && cfg.max_frames == 10);
    GF_ASSERT(cfg.initial_mode == GF_INPUT_TOUCH);

    char *argv_xr[] = {"gf", "--mode", "xr", "--e2e-script", "s.txt"};
    GF_ASSERT(gf_app_parse_args(5, argv_xr, &cfg));
    GF_ASSERT(cfg.initial_mode == GF_INPUT_XR && cfg.headless);

    char *argv_bad[] = {"gf", "--unknown-flag"};
    GF_ASSERT(!gf_app_parse_args(2, argv_bad, &cfg));

    GF_ASSERT(gf_app_map_ascii_to_midi('z') == 48);
    GF_ASSERT(gf_app_map_ascii_to_midi('Z') == 48);
    GF_ASSERT(gf_app_map_ascii_to_midi('q') == 60);
    GF_ASSERT(gf_app_map_ascii_to_midi('w') == -1);
    GF_ASSERT(gf_app_map_ascii_to_midi('!') == -1);
    GF_ASSERT(gf_app_is_nav_key('W') && gf_app_is_nav_key('3') && gf_app_is_nav_key(27));
}

static void test_app_lifecycle_midi_xr_and_e2e_script(void) {
    GFGlApi gl = make_gl();
    GFAppConfig cfg;
    gf_app_config_default(&cfg);
    cfg.assets_dir = "assets";
    cfg.width = 320;
    cfg.height = 240;

    GFApp app;
    GF_ASSERT(!gf_app_init(NULL, &cfg, &gl, false));
    GF_ASSERT(gf_app_init(&app, &cfg, &gl, false));

    gf_app_resize(&app, 400, 300);
    GF_ASSERT(app.width == 400 && app.height == 300);

    /* QWERTY notes, WASD/1-7/Esc 3D navigation keys, pointer, MIDI note & CC, mode switching, reload */
    gf_app_handle_key(&app, 'z', true);
    gf_app_handle_key(&app, 'z', false);
    gf_app_handle_key(&app, 'W', true);
    gf_app_handle_key(&app, 'w', false);
    gf_app_handle_key(&app, '2', true);
    gf_app_handle_key(&app, 27, true);
    gf_app_handle_pointer(&app, "pointer_down", 1, 50.0f, 50.0f);
    gf_app_handle_pointer_ex(&app, "pointer_move", 1, 60.0f, 55.0f, 2.0f);
    gf_app_handle_pointer(&app, "pointer_up", 1, 60.0f, 55.0f);
    gf_app_handle_midi(&app, 0x90, 60, 100);
    gf_app_handle_midi(&app, 0x90, 60, 0);
    gf_app_handle_midi(&app, 0x80, 64, 0);
    gf_app_handle_midi(&app, 0xB0, 1, 64);
    gf_app_handle_midi(&app, 0xB0, 74, 80);
    gf_app_handle_midi(&app, 0xB0, 91, 50);

    gf_app_set_input_mode(&app, GF_INPUT_TOUCH);
    gf_app_step(&app, 0.016f, true);
    GF_ASSERT(app.frame_count == 1);
    GF_ASSERT(gf_app_trigger_reload(&app));

    /* WebXR stereo frame & 6DoF controller ray flow */
    GFMat4 id = gf_mat4_identity();
    gf_app_xr_begin(&app, 0.011f);
    gf_app_xr_controller_ray(&app, 0, 0.0f, 1.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1); /* teleport room 1 */
    gf_app_xr_controller_ray(&app, 0, 0.0f, 1.5f, 1.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0); /* xr_down */
    gf_app_xr_controller_ray(&app, 0, 0.0f, 1.6f, 1.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0); /* xr_move */
    gf_app_xr_controller_ray(&app, 0, 0.0f, 1.6f, 1.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0); /* xr_up */
    gf_app_xr_render_eye(&app, id.m, id.m, 0, 0, 160, 240);
    gf_app_xr_render_eye(&app, id.m, id.m, 160, 0, 160, 240);
    gf_app_xr_controller(&app, 1, 1.0f);
    gf_app_xr_controller(&app, 1, 0.0f);
    gf_app_xr_end(&app);
    GF_ASSERT(app.xr_frame_count == 1);
    app.xr_active = false;

    /* E2E script execution covering all 7-room, focus, cable, mutator & preset commands */
    const char *spath = "/tmp/gf_unit_e2e.txt";
    FILE *fp = fopen(spath, "wb");
    fprintf(fp,
        "# unit e2e script\n"
        "MODE desktop\n"
        "MODE touch\n"
        "MODE xr\n"
        "MODE desktop\n"
        "TELEPORT 6\n"
        "WALK 1.0 0.0 2\n"
        "FOCUS_RACK 0\n"
        "STEP 2\n"
        "EXIT_FOCUS\n"
        "POINTER down 100 100 0\n"
        "POINTER move 110 105 0\n"
        "POINTER up 110 105 0\n"
        "SET_PARAM master.gain 0.75\n"
        "CONNECT osc1.audio_out flt1.in 0.85\n"
        "DISCONNECT_PORT flt1.in\n"
        "CONNECT osc1.audio_out flt1.in 0.85\n"
        "MUTATE 0.25 0.40\n"
        "PRESET patches/emerald_aether.lua\n"
        "MELODY melodies/organ_cathedrals/01_cornfield_cathedral_organ.lua\n"
        "STEP 2\n"
        "LOOPER stop\n"
        "NOTE_ON 60 0.9\n"
        "XY 0.7 0.4 0.6\n"
        "XY 0.5 0.5\n"
        "VOXELS 1\n"
        "STEP 2\n"
        "SAVE_PPM /tmp/gf_unit_frame.ppm\n"
        "SAVE_WAV /tmp/gf_unit_audio.wav 512\n"
        "NOTE_OFF 60\n"
        "VOXELS 0\n"
        "RELOAD\n"
        "STEP 1\n"
    );
    fclose(fp);
    GF_ASSERT(gf_app_run_e2e_script(&app, spath));
    GF_ASSERT(!gf_app_run_e2e_script(&app, "/tmp/gf_missing_script_999.txt"));

    char json[1024];
    GF_ASSERT(gf_app_get_status_json(&app, json, sizeof(json)) > 20);
    GF_ASSERT(strstr(json, "\"xrFrames\":1") != NULL);
    GF_ASSERT(strstr(json, "\"cableCount\":") != NULL);
    GF_ASSERT(strstr(json, "\"looperState\":") != NULL);

    GF_ASSERT(!gf_app_write_ppm(NULL, 1, 1, NULL));
    GF_ASSERT(!gf_app_write_wav(NULL, 44100, NULL, 0));

    gf_hot_reload_trigger_all(&app.hot_reload);

    gf_app_destroy(&app);
    gf_app_destroy(&app);

    GFAppConfig bad_cfg = cfg;
    bad_cfg.assets_dir = "/tmp/gf_nonexistent_assets_dir_404";
    GF_ASSERT(!gf_app_init(&app, &bad_cfg, &gl, false));

    GFGlApi bad_gl = gl;
    bad_gl.CreateShader = NULL;
    GF_ASSERT(!gf_app_init(&app, &cfg, &bad_gl, false));
}

int main(void) {
    GF_RUN_TEST(test_cli_parsing_and_keymap);
    GF_RUN_TEST(test_app_lifecycle_midi_xr_and_e2e_script);
    GF_TEST_SUMMARY();
}
