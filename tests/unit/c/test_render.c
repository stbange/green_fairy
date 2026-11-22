#include "test_harness.h"

#include "assets/asset_manager.h"
#include "assets/loaders.h"
#include "render/gl_renderer.h"
#include "render/scene.h"
#include "render/voxel_mesher.h"

/* ---------------- Mock OpenGL Dispatch Table ---------------- */
typedef struct MockGlState {
    GFGLuint next_id;
    int fail_compile_at;
    int compile_calls;
    int fail_link_at;
    int link_calls;
    int draw_elements_calls;
    int draw_arrays_calls;
    int read_pixels_calls;
} MockGlState;

static MockGlState g_mgl;

static GFGLuint GF_APIENTRY m_CreateShader(GFGLenum t) { (void)t; return ++g_mgl.next_id; }
static void GF_APIENTRY m_ShaderSource(GFGLuint s, GFGLsizei c, const char *const *str, const GFGLint *l) { (void)s;(void)c;(void)str;(void)l; }
static void GF_APIENTRY m_CompileShader(GFGLuint s) { (void)s; g_mgl.compile_calls++; }
static void GF_APIENTRY m_GetShaderiv(GFGLuint s, GFGLenum p, GFGLint *out) {
    (void)s; (void)p;
    *out = (g_mgl.fail_compile_at > 0 && g_mgl.compile_calls == g_mgl.fail_compile_at) ? 0 : 1;
}
static void GF_APIENTRY m_GetShaderInfoLog(GFGLuint s, GFGLsizei m, GFGLsizei *l, char *buf) {
    (void)s; snprintf(buf, (size_t)m, "mock compile error"); if (l) *l = 18;
}
static void GF_APIENTRY m_DeleteShader(GFGLuint s) { (void)s; }
static GFGLuint GF_APIENTRY m_CreateProgram(void) { return ++g_mgl.next_id; }
static void GF_APIENTRY m_AttachShader(GFGLuint p, GFGLuint s) { (void)p;(void)s; }
static void GF_APIENTRY m_LinkProgram(GFGLuint p) { (void)p; g_mgl.link_calls++; }
static void GF_APIENTRY m_GetProgramiv(GFGLuint p, GFGLenum pn, GFGLint *out) {
    (void)p; (void)pn;
    *out = (g_mgl.fail_link_at > 0 && g_mgl.link_calls == g_mgl.fail_link_at) ? 0 : 1;
}
static void GF_APIENTRY m_GetProgramInfoLog(GFGLuint p, GFGLsizei m, GFGLsizei *l, char *buf) {
    (void)p; snprintf(buf, (size_t)m, "mock link error"); if (l) *l = 15;
}
static void GF_APIENTRY m_DeleteProgram(GFGLuint p) { (void)p; }
static void GF_APIENTRY m_UseProgram(GFGLuint p) { (void)p; }
static GFGLint GF_APIENTRY m_GetUniformLocation(GFGLuint p, const char *n) { (void)p;(void)n; return 1; }
static void GF_APIENTRY m_Uniform1f(GFGLint l, float v) { (void)l;(void)v; }
static void GF_APIENTRY m_Uniform1i(GFGLint l, GFGLint v) { (void)l;(void)v; }
static void GF_APIENTRY m_Uniform2f(GFGLint l, float a, float b) { (void)l;(void)a;(void)b; }
static void GF_APIENTRY m_Uniform3f(GFGLint l, float a, float b, float c) { (void)l;(void)a;(void)b;(void)c; }
static void GF_APIENTRY m_Uniform4f(GFGLint l, float a, float b, float c, float d) { (void)l;(void)a;(void)b;(void)c;(void)d; }
static void GF_APIENTRY m_UniformMatrix4fv(GFGLint l, GFGLsizei c, GFGLboolean t, const float *v) { (void)l;(void)c;(void)t;(void)v; }
static void GF_APIENTRY m_GenIds(GFGLsizei n, GFGLuint *ids) { for (GFGLsizei i = 0; i < n; ++i) ids[i] = ++g_mgl.next_id; }
static void GF_APIENTRY m_BindId(GFGLuint id) { (void)id; }
static void GF_APIENTRY m_BindTargetId(GFGLenum t, GFGLuint id) { (void)t;(void)id; }
static void GF_APIENTRY m_DelIds(GFGLsizei n, const GFGLuint *ids) { (void)n;(void)ids; }
static void GF_APIENTRY m_BufferData(GFGLenum t, GFGLsizeiptr s, const void *d, GFGLenum u) { (void)t;(void)s;(void)d;(void)u; }
static void GF_APIENTRY m_EnableVAA(GFGLuint i) { (void)i; }
static void GF_APIENTRY m_VAPtr(GFGLuint i, GFGLint s, GFGLenum t, GFGLboolean n, GFGLsizei st, const void *p) { (void)i;(void)s;(void)t;(void)n;(void)st;(void)p; }
static void GF_APIENTRY m_TexImage2D(GFGLenum t, GFGLint l, GFGLint i, GFGLsizei w, GFGLsizei h, GFGLint b, GFGLenum f, GFGLenum ty, const void *d) { (void)t;(void)l;(void)i;(void)w;(void)h;(void)b;(void)f;(void)ty;(void)d; }
static void GF_APIENTRY m_TexParami(GFGLenum t, GFGLenum p, GFGLint v) { (void)t;(void)p;(void)v; }
static void GF_APIENTRY m_ActiveTex(GFGLenum t) { (void)t; }
static void GF_APIENTRY m_Viewport(GFGLint x, GFGLint y, GFGLsizei w, GFGLsizei h) { (void)x;(void)y;(void)w;(void)h; }
static void GF_APIENTRY m_Cap(GFGLenum c) { (void)c; }
static void GF_APIENTRY m_BlendFunc(GFGLenum s, GFGLenum d) { (void)s;(void)d; }
static void GF_APIENTRY m_ClearColor(float r, float g, float b, float a) { (void)r;(void)g;(void)b;(void)a; }
static void GF_APIENTRY m_Clear(GFGLenum m) { (void)m; }
static void GF_APIENTRY m_DrawElements(GFGLenum m, GFGLsizei c, GFGLenum t, const void *p) { (void)m;(void)c;(void)t;(void)p; g_mgl.draw_elements_calls++; }
static void GF_APIENTRY m_DrawArrays(GFGLenum m, GFGLint f, GFGLsizei c) { (void)m;(void)f;(void)c; g_mgl.draw_arrays_calls++; }
static void GF_APIENTRY m_ReadPixels(GFGLint x, GFGLint y, GFGLsizei w, GFGLsizei h, GFGLenum f, GFGLenum t, void *px) {
    (void)x;(void)y;(void)f;(void)t;
    memset(px, 0x77, (size_t)w * (size_t)h * 4u);
    g_mgl.read_pixels_calls++;
}

static GFGlApi make_mock_gl(void) {
    memset(&g_mgl, 0, sizeof(g_mgl));
    GFGlApi api = {
        .CreateShader = m_CreateShader, .ShaderSource = m_ShaderSource, .CompileShader = m_CompileShader,
        .GetShaderiv = m_GetShaderiv, .GetShaderInfoLog = m_GetShaderInfoLog, .DeleteShader = m_DeleteShader,
        .CreateProgram = m_CreateProgram, .AttachShader = m_AttachShader, .LinkProgram = m_LinkProgram,
        .GetProgramiv = m_GetProgramiv, .GetProgramInfoLog = m_GetProgramInfoLog, .DeleteProgram = m_DeleteProgram,
        .UseProgram = m_UseProgram, .GetUniformLocation = m_GetUniformLocation,
        .Uniform1f = m_Uniform1f, .Uniform1i = m_Uniform1i, .Uniform2f = m_Uniform2f,
        .Uniform3f = m_Uniform3f, .Uniform4f = m_Uniform4f, .UniformMatrix4fv = m_UniformMatrix4fv,
        .GenVertexArrays = m_GenIds, .BindVertexArray = m_BindId, .DeleteVertexArrays = m_DelIds,
        .GenBuffers = m_GenIds, .BindBuffer = m_BindTargetId, .BufferData = m_BufferData, .DeleteBuffers = m_DelIds,
        .EnableVertexAttribArray = m_EnableVAA, .VertexAttribPointer = m_VAPtr,
        .GenTextures = m_GenIds, .BindTexture = m_BindTargetId, .TexImage2D = m_TexImage2D,
        .TexParameteri = m_TexParami, .DeleteTextures = m_DelIds, .ActiveTexture = m_ActiveTex,
        .Viewport = m_Viewport, .Enable = m_Cap, .Disable = m_Cap, .BlendFunc = m_BlendFunc,
        .ClearColor = m_ClearColor, .Clear = m_Clear, .DrawElements = m_DrawElements,
        .DrawArrays = m_DrawArrays, .ReadPixels = m_ReadPixels
    };
    return api;
}

static void test_voxel_mesher_culling(void) {
    /* Two adjacent voxels share 2 hidden faces -> 10 exposed faces -> 20 triangles -> 60 indices */
    const char *vox =
        "GFVOX 1\n"
        "SIZE 2 1 1\n"
        "PALETTE 1 50 250 120 0\n"
        "VOX 0 0 0 1\n"
        "VOX 1 0 0 1\n";
    GFVoxelGrid g;
    GF_ASSERT(gf_load_voxel_memory(vox, strlen(vox), &g));
    GFMeshData mesh;
    GF_ASSERT(gf_voxel_build_mesh(&g, 0.5f, &mesh));
    GF_ASSERT(mesh.index_count == 60);
    GF_ASSERT(mesh.vertex_count == 40);
    gf_mesh_data_free(&mesh);
    gf_voxel_grid_free(&g);

    /* Empty voxel grid returns false */
    const char *empty_vox = "GFVOX 1\nSIZE 2 2 2\n";
    GF_ASSERT(gf_load_voxel_memory(empty_vox, strlen(empty_vox), &g));
    GF_ASSERT(!gf_voxel_build_mesh(&g, 0.5f, &mesh));
    gf_voxel_grid_free(&g);
    GF_ASSERT(!gf_voxel_build_mesh(NULL, 0.5f, &mesh));
}

static void test_scene_and_hud_batch(void) {
    GFScene sc;
    gf_scene_init(NULL);
    gf_scene_init(&sc);
    gf_scene_clear_entities(NULL);

    int m1 = gf_scene_add_entity(&sc, GF_ENT_MESH, "models/crystal_octa.obj");
    int s1 = gf_scene_add_entity(&sc, GF_ENT_SPRITE, "sprites/fairy_wisp.tga");
    int v1 = gf_scene_add_entity(&sc, GF_ENT_VOXEL, "voxels/fairy_spire.gfvox");
    GF_ASSERT(m1 > 0 && s1 > 0 && v1 > 0);
    GF_ASSERT(gf_scene_add_entity(&sc, GF_ENT_MESH, "") == -1);

    GF_ASSERT(gf_scene_set_transform(&sc, m1, gf_vec3(0, 1, 0), gf_vec3(0, 0.5f, 0), gf_vec3(1, 1, 1)));
    GF_ASSERT(!gf_scene_set_transform(&sc, 9999, gf_vec3(0, 0, 0), gf_vec3(0, 0, 0), gf_vec3(1, 1, 1)));
    GF_ASSERT(gf_scene_set_tint(&sc, m1, (GFVec4){0.2f, 1.0f, 0.5f, 1.0f}));
    GF_ASSERT(!gf_scene_set_tint(&sc, 9999, (GFVec4){1, 1, 1, 1}));
    GF_ASSERT(gf_scene_set_sprite_anim(&sc, s1, (GFVec2){1.0f, 1.0f}, 4, -1));
    GF_ASSERT(!gf_scene_set_sprite_anim(&sc, 9999, (GFVec2){1, 1}, 4, 0));

    gf_scene_set_camera(&sc, gf_vec3(0, 2, 6), gf_vec3(0, 0, 0), 60.0f);
    GFMat4 view, proj;
    gf_scene_compute_matrices(&sc, 16.0f / 9.0f, &view, &proj);
    GF_ASSERT(proj.m[11] == -1.0f);

    GFAudioTelemetry tel;
    memset(&tel, 0, sizeof(tel));
    tel.rms_l = 0.4f;
    for (int i = 0; i < GF_TELEMETRY_SCOPE; ++i) tel.scope[i] = sinf((float)i * 0.1f);
    GFVertex rverts[512];
    int rv = gf_scene_build_ribbon(&tel, rverts, 512);
    GF_ASSERT(rv > 0);
    GF_ASSERT(gf_scene_build_ribbon(&tel, rverts, 6) == 0);

    GF_ASSERT(gf_scene_remove(&sc, v1));
    GF_ASSERT(!gf_scene_remove(&sc, v1));
    gf_scene_clear_entities(&sc);

    /* HUD batching */
    GFFontMetrics fm = {8, 8, 16, 6, 32, 96, "fonts/retro_font.tga"};
    GFHudBatch hb;
    gf_hud_init(&hb, &fm);
    GF_ASSERT(gf_hud_push_rect(&hb, 10, 10, 100, 40, (GFVec4){0, 1, 0, 0.8f}));
    GF_ASSERT(gf_hud_push_line(&hb, 0, 0, 50, 50, 2.0f, (GFVec4){1, 1, 1, 1}));
    GF_ASSERT(gf_hud_push_line(&hb, 5, 5, 5, 5, 2.0f, (GFVec4){1, 1, 1, 1}));
    GF_ASSERT(gf_hud_push_text(&hb, 12, 14, 2.0f, "GREEN FAIRY!", (GFVec4){1, 1, 1, 1}));
    GF_ASSERT(gf_hud_push_text(&hb, 12, 14, 2.0f, "", (GFVec4){1, 1, 1, 1}));
    GF_ASSERT(!gf_hud_push_text(&hb, 12, 14, 0.0f, "bad", (GFVec4){1, 1, 1, 1}));
    GF_ASSERT(hb.cmd_count == 2); /* 1 untextured batch + 1 textured text batch */
    gf_hud_clear(&hb);
    GF_ASSERT(hb.vert_count == 0);
}

static void test_gl_renderer_and_error_paths(void) {
    GFAssetManager am;
    GF_ASSERT(gf_assets_init(&am, "assets", NULL));

    GFGlApi api = make_mock_gl();
    GFGlRenderer r;
    GF_ASSERT(!gf_gl_renderer_init(NULL, &api, &am, false));

    /* Shader compile failure path */
    g_mgl.fail_compile_at = 1;
    GF_ASSERT(!gf_gl_renderer_init(&r, &api, &am, false));

    /* Shader link failure path */
    api = make_mock_gl();
    g_mgl.fail_link_at = 1;
    GF_ASSERT(!gf_gl_renderer_init(&r, &api, &am, true));

    /* Clean init + shader reload */
    api = make_mock_gl();
    GF_ASSERT(gf_gl_renderer_init(&r, &api, &am, false));
    GF_ASSERT(gf_gl_renderer_reload_shaders(&r));

    GFScene sc;
    gf_scene_init(&sc);
    gf_scene_add_entity(&sc, GF_ENT_MESH, "models/crystal_octa.obj");
    gf_scene_add_entity(&sc, GF_ENT_VOXEL, "voxels/fairy_spire.gfvox");
    int sp = gf_scene_add_entity(&sc, GF_ENT_SPRITE, "sprites/fairy_wisp.tga");
    gf_scene_set_sprite_anim(&sc, sp, (GFVec2){0.7f, 0.7f}, 4, 2);

    GFAudioTelemetry tel;
    memset(&tel, 0, sizeof(tel));
    tel.rms_l = 0.35f;
    GFMat4 view, proj;
    gf_scene_compute_matrices(&sc, 1.33f, &view, &proj);

    /* Render with voxels enabled -> mesh + voxel + ribbon + sprite = 4 draw calls */
    sc.use_camera_rig = false;
    sc.voxels_enabled = true;
    gf_gl_renderer_render_view(&r, &sc, &tel, &view, &proj, 0, 0, 320, 240, true);
    GF_ASSERT(r.last_draw_calls == 4);

    /* Render again (cache hit!) with voxels disabled -> 3 draw calls */
    sc.voxels_enabled = false;
    gf_gl_renderer_render_view(&r, &sc, &tel, &view, &proj, 0, 0, 320, 240, true);
    GF_ASSERT(r.last_draw_calls == 3);

    /* Populate 3D synth racks, widgets, holographic curves, world-space text, XR laser & catenary cables */
    sc.use_camera_rig = true;
    gf_camera_teleport_room(&sc.camera, 1);
    gf_scene_step_camera(&sc, 0.016f);

    int culled_ent = gf_scene_add_entity(&sc, GF_ENT_MESH, "models/room_shell.obj");
    GF_ASSERT(gf_scene_set_entity_room(&sc, culled_ent, 4)); /* Opposite room (culled) */
    GF_ASSERT(!gf_scene_set_entity_room(&sc, 99999, 4));

    int rk = gf_spatial_add_rack(&sc.spatial, 1, "OSC 1", gf_vec3(0, 1.5f, -21.5f), 0, 0, 2.2f, 1.6f, (GFVec4){0.3f, 1, 0.8f, 1});
    GFWidgetDesc3D wd;
    memset(&wd, 0, sizeof(wd));
    wd.room_id = 1; wd.rack_id = rk; wd.size = (GFVec2){0.1f, 0.1f}; wd.max_val = 1.0f;
    wd.color = (GFVec4){0.4f, 1, 0.8f, 1};
    wd.kind = GF_W3D_KNOB; wd.local_pos = gf_vec3(0, 0.2f, 0.01f); wd.label = "MORPH";
    int wk = gf_spatial_add_widget(&sc.spatial, &wd);
    sc.spatial.hovered_widget_id = wk;
    wd.kind = GF_W3D_JACK_OUT; wd.port_id = GF_PORT_OSC_1_OUT; wd.local_pos = gf_vec3(-0.3f, -0.4f, 0.01f); wd.label = "OUT";
    gf_spatial_add_widget(&sc.spatial, &wd);
    wd.kind = GF_W3D_JACK_IN; wd.port_id = GF_PORT_FLT_1_IN; wd.local_pos = gf_vec3(0.3f, -0.4f, 0.01f); wd.label = "IN";
    gf_spatial_add_widget(&sc.spatial, &wd);

    float ys[4] = {0.1f, 0.9f, 0.4f, 0.2f};
    gf_spatial_set_curve_xy(&sc.spatial, 0, rk, 0, 0, 1.0f, 0.3f, NULL, ys, 4, false, (GFVec4){0, 1, 0.8f, 1});

    GFFontMetrics fm = {8, 8, 16, 6, 32, 96, "fonts/retro_font.tga"};
    sc.world_font = fm;
    sc.has_world_font = true;
    sc.xr_laser_active = true;
    sc.xr_laser_origin = gf_vec3(0, 1.2f, -15.0f);
    sc.xr_laser_end = gf_vec3(0, 1.5f, -21.5f);

    GFPatchbay pb;
    GFPatchPlan plan;
    gf_patchbay_init(&pb);
    gf_patchbay_connect(&pb, GF_PORT_OSC_1_OUT, GF_PORT_FLT_1_IN, 0.8f);
    gf_patchbay_compile(&pb, &plan);
    sc.active_plan = &plan;

    gf_scene_compute_matrices(&sc, 1.33f, &view, &proj);
    gf_gl_renderer_render_view(&r, &sc, &tel, &view, &proj, 0, 0, 320, 240, true);
    GF_ASSERT(r.last_draw_calls >= 9);
    GF_ASSERT(r.last_visible_rooms < 7); /* Distant rooms behind camera were culled! */

    /* HUD + WebXR stereo eye rendering */
    GFHudBatch hb;
    gf_hud_init(&hb, &fm);
    gf_hud_push_rect(&hb, 0, 0, 100, 30, (GFVec4){0, 0, 0, 0.7f});
    gf_hud_push_text(&hb, 4, 4, 1.0f, "XR OK", (GFVec4){0, 1, 0, 1});
    gf_gl_renderer_render_hud(&r, &hb, 320, 240);
    gf_gl_renderer_render_xr_eye(&r, &sc, &hb, &tel, &view, &proj, 0, 0, 160, 240);
    gf_gl_renderer_render_xr_eye(&r, &sc, &hb, &tel, &view, &proj, 160, 0, 160, 240);

    uint8_t px[16];
    GF_ASSERT(gf_gl_renderer_read_rgba(&r, 2, 2, px));
    GF_ASSERT(px[0] == 0x77);
    GF_ASSERT(!gf_gl_renderer_read_rgba(&r, 0, 2, px));

    gf_gl_renderer_destroy(&r);
    gf_gl_renderer_destroy(&r);
}

int main(void) {
    GF_RUN_TEST(test_voxel_mesher_culling);
    GF_RUN_TEST(test_scene_and_hud_batch);
    GF_RUN_TEST(test_gl_renderer_and_error_paths);
    GF_TEST_SUMMARY();
}
