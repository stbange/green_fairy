#define _POSIX_C_SOURCE 200809L
#include "test_harness.h"

#include "assets/asset_manager.h"
#include "assets/hot_reload.h"
#include "assets/loaders.h"

#include <sys/stat.h>
#include <utime.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <unistd.h>
#define MKDIR(p) mkdir((p), 0755)
#endif

static void write_file(const char *path, const void *data, size_t len) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return;
    fwrite(data, 1, len, fp);
    fclose(fp);
}

static void test_asset_manager_and_hot_reload(void) {
    const char *base = "/tmp/gf_test_assets_root";
    const char *sub = "/tmp/gf_test_assets_root/sub";
    const char *sib_parent = "/tmp/gf_test_sib";
    const char *sib_assets = "/tmp/gf_test_sib/assets";
    MKDIR(base);
    MKDIR(sub);
    MKDIR(sib_parent);
    MKDIR(sib_assets);

    write_file("/tmp/gf_test_assets_root/sub/b.txt", "hello-b", 7);
    write_file("/tmp/gf_test_assets_root/sub/a.txt", "hello-a", 7);

    GFAssetManager am;
    GF_ASSERT(!gf_assets_init(NULL, base, NULL));
    GF_ASSERT(!gf_assets_init(&am, "/tmp/gf_does_not_exist_9999", NULL));
    GF_ASSERT_STR_EQ(gf_assets_root(&am), "");
    GF_ASSERT(gf_assets_init(&am, base, NULL));
    GF_ASSERT_STR_EQ(gf_assets_root(&am), base);

    /* Traversal guard */
    char resolved[256];
    GF_ASSERT(gf_assets_resolve_path(&am, "sub/a.txt", resolved, sizeof(resolved)));
    GF_ASSERT(!gf_assets_resolve_path(&am, "../etc/passwd", resolved, sizeof(resolved)));

    /* File read */
    uint8_t *data = NULL;
    size_t sz = 0;
    GF_ASSERT(!gf_assets_read_file(&am, "sub/a.txt", NULL, &sz));
    GF_ASSERT(!gf_assets_read_file(&am, "sub/missing.txt", &data, &sz));
    GF_ASSERT(gf_assets_read_file(&am, "sub/a.txt", &data, &sz));
    GF_ASSERT(sz == 7);
    GF_ASSERT_STR_EQ((const char *)data, "hello-a");
    free(data);

    /* List directory (sorted) */
    char names[8][GF_ASSET_NAME_MAX];
    int n = gf_assets_list_dir(&am, "sub", names, 8);
    GF_ASSERT(n == 2);
    GF_ASSERT_STR_EQ(names[0], "a.txt");
    GF_ASSERT_STR_EQ(names[1], "b.txt");
    GF_ASSERT(gf_assets_list_dir(&am, ".", names, 8) >= 1);
    GF_ASSERT(gf_assets_list_dir(&am, "../escape", names, 8) == -1);
    GF_ASSERT(gf_assets_list_dir(&am, "missing_dir", names, 8) == -1);
    GF_ASSERT(gf_assets_list_dir(NULL, "sub", names, 8) == -1);

    /* mtime + hot reload */
    uint64_t mt = 0;
    GF_ASSERT(!gf_assets_file_mtime(&am, "sub/a.txt", NULL));
    GF_ASSERT(!gf_assets_file_mtime(&am, "sub/missing.txt", &mt));
    GF_ASSERT(gf_assets_file_mtime(&am, "sub/a.txt", &mt));
    GF_ASSERT(mt > 0);

    GFHotReloader hr;
    gf_hot_reload_init(NULL, &am);
    gf_hot_reload_init(&hr, &am);
    int reload_hits = 0;
    /* callback via static helper below? Use file-scope helper */
    (void)reload_hits;

    /* Sibling <exe_dir>/assets resolution */
    setenv("GREEN_FAIRY_ASSETS", "", 1);
    GF_ASSERT(gf_assets_init(&am, NULL, "/tmp/gf_test_sib/green_fairy"));
    GF_ASSERT_STR_EQ(gf_assets_root(&am), sib_assets);

    /* Env override */
    setenv("GREEN_FAIRY_ASSETS", "/tmp/gf_missing_env_dir", 1);
    GF_ASSERT(!gf_assets_init(&am, NULL, NULL));
    setenv("GREEN_FAIRY_ASSETS", base, 1);
    GF_ASSERT(gf_assets_init(&am, NULL, NULL));
    GF_ASSERT_STR_EQ(gf_assets_root(&am), base);
    unsetenv("GREEN_FAIRY_ASSETS");
}

static void on_reload_cb(const char *rel_path, void *user_data) {
    (void)rel_path;
    int *hits = (int *)user_data;
    (*hits)++;
}

static void test_hot_reload_behavior(void) {
    const char *base = "/tmp/gf_test_assets_root";
    MKDIR(base);
    const char *fpath = "/tmp/gf_test_assets_root/watch.lua";
    write_file(fpath, "return 1", 8);

    GFAssetManager am;
    GF_ASSERT(gf_assets_init(&am, base, NULL));

    GFHotReloader hr;
    gf_hot_reload_init(&hr, &am);
    int hits = 0;
    GF_ASSERT(!gf_hot_reload_watch(NULL, "watch.lua", on_reload_cb, &hits));
    GF_ASSERT(!gf_hot_reload_watch(&hr, "missing.lua", on_reload_cb, &hits));
    GF_ASSERT(gf_hot_reload_watch(&hr, "watch.lua", on_reload_cb, &hits));

    GF_ASSERT(gf_hot_reload_poll(NULL) == 0);
    GF_ASSERT(gf_hot_reload_poll(&hr) == 0);

    struct utimbuf ut = {1700000100, 1700000100};
    utime(fpath, &ut);
    GF_ASSERT(gf_hot_reload_poll(&hr) == 1);
    GF_ASSERT(hits == 1);
    GF_ASSERT(gf_hot_reload_poll(&hr) == 0);

    GF_ASSERT(gf_hot_reload_trigger_all(NULL) == 0);
    GF_ASSERT(gf_hot_reload_trigger_all(&hr) == 1);
    GF_ASSERT(hits == 2);
}

static void test_obj_loader(void) {
    const char *obj =
        "# Retro crystal\n"
        "v 0 0 0 0.1 1.0 0.4\n"
        "v 1 0 0 0.2 0.9 0.5\n"
        "v 1 1 0 0.3 0.8 0.6\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 1 1\n"
        "vt 0 1\n"
        "vn 0 0 1\n"
        "f 1/1/1 2/2/1 3/3/1 4/4/1\n"
        "f -4/-4 -3/-3 -2/-2\n"
        "f 1//1 2//1 3//1\n"
        "f 1 2 3\n";

    GFMeshData mesh;
    GF_ASSERT(gf_load_obj_memory(obj, strlen(obj), &mesh));
    /* Quad (2 tris) + 3 tris = 5 tris = 15 verts/indices */
    GF_ASSERT(mesh.index_count == 15);
    GF_ASSERT(mesh.vertex_count == 15);
    GF_ASSERT_NEAR(mesh.vertices[0].g, 1.0f, 1e-5);
    GF_ASSERT_NEAR(mesh.vertices[0].nz, 1.0f, 1e-5);
    gf_mesh_data_free(&mesh);
    gf_mesh_data_free(NULL);

    GF_ASSERT(!gf_load_obj_memory(NULL, 0, &mesh));
    GF_ASSERT(!gf_load_obj_memory("# empty", 7, &mesh));
}

static void test_image_loaders(void) {
    /* Valid 2x2 P6 PPM with comment */
    uint8_t ppm[64];
    const char *hdr = "P6\n# comment\n2 2 255\n";
    size_t hlen = strlen(hdr);
    memcpy(ppm, hdr, hlen);
    for (int i = 0; i < 12; ++i) ppm[hlen + i] = (uint8_t)(i * 20);
    GFImageData img;
    GF_ASSERT(gf_load_image_memory(ppm, hlen + 12, &img));
    GF_ASSERT(img.width == 2 && img.height == 2);
    GF_ASSERT(img.rgba[0] == 0 && img.rgba[1] == 20 && img.rgba[2] == 40 && img.rgba[3] == 255);
    gf_image_data_free(&img);
    gf_image_data_free(NULL);

    /* Malformed PPM */
    const char *bad_ppm = "P6\n2 2 128\n";
    GF_ASSERT(!gf_load_image_memory((const uint8_t *)bad_ppm, strlen(bad_ppm), &img));

    /* Valid 2x1 32-bit top-origin TGA */
    uint8_t tga32[18 + 8];
    memset(tga32, 0, sizeof(tga32));
    tga32[2] = 2;
    tga32[12] = 2; tga32[14] = 1;
    tga32[16] = 32;
    tga32[17] = 0x20;
    /* BGRA pixel 0 */
    tga32[18] = 10; tga32[19] = 20; tga32[20] = 30; tga32[21] = 200;
    GF_ASSERT(gf_load_image_memory(tga32, sizeof(tga32), &img));
    GF_ASSERT(img.width == 2 && img.height == 1);
    GF_ASSERT(img.rgba[0] == 30 && img.rgba[1] == 20 && img.rgba[2] == 10 && img.rgba[3] == 200);
    gf_image_data_free(&img);

    /* Valid 1x2 24-bit bottom-origin TGA */
    uint8_t tga24[18 + 6];
    memset(tga24, 0, sizeof(tga24));
    tga24[2] = 2;
    tga24[12] = 1; tga24[14] = 2;
    tga24[16] = 24;
    tga24[18] = 1; tga24[19] = 2; tga24[20] = 3; /* bottom row -> dst row 1 */
    tga24[21] = 4; tga24[22] = 5; tga24[23] = 6; /* top row -> dst row 0 */
    GF_ASSERT(gf_load_image_memory(tga24, sizeof(tga24), &img));
    GF_ASSERT(img.rgba[0] == 6 && img.rgba[1] == 5 && img.rgba[2] == 4 && img.rgba[3] == 255);
    GF_ASSERT(img.rgba[4] == 3 && img.rgba[5] == 2 && img.rgba[6] == 1 && img.rgba[7] == 255);
    gf_image_data_free(&img);

    /* Malformed TGA */
    uint8_t tiny[4] = {0, 0, 0, 0};
    GF_ASSERT(!gf_load_image_memory(tiny, sizeof(tiny), &img));
    GF_ASSERT(!gf_load_image_memory(NULL, 0, &img));
    tga24[16] = 8;
    GF_ASSERT(!gf_load_image_memory(tga24, sizeof(tga24), &img));
}

static void test_voxel_loader(void) {
    const char *vox =
        "GFVOX 1\n"
        "# Emerald altar\n"
        "SIZE 4 4 4\n"
        "PALETTE 1 40 240 120 255\n"
        "VOX 1 2 3 1\n";
    GFVoxelGrid g;
    GF_ASSERT(gf_load_voxel_memory(vox, strlen(vox), &g));
    GF_ASSERT(g.sx == 4 && g.sy == 4 && g.sz == 4);
    GF_ASSERT(g.palette[1][1] == 240);
    GF_ASSERT(g.cells[(3 * 4 + 2) * 4 + 1] == 1);
    gf_voxel_grid_free(&g);
    gf_voxel_grid_free(NULL);

    GF_ASSERT(!gf_load_voxel_memory(NULL, 0, &g));
    GF_ASSERT(!gf_load_voxel_memory("BADHEADER\n", 10, &g));
    GF_ASSERT(!gf_load_voxel_memory("GFVOX 1\nSIZE 0 4 4\n", 19, &g));
    GF_ASSERT(!gf_load_voxel_memory("GFVOX 1\n", 8, &g));
}

static void write_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v & 0xFF); p[1] = (uint8_t)(v >> 8); }
static void write_u32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF); p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF); p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void test_wav_loader(void) {
    /* 16-bit PCM mono WAV with an extra JUNK chunk */
    uint8_t wav[128];
    memset(wav, 0, sizeof(wav));
    memcpy(wav, "RIFF", 4);
    memcpy(wav + 8, "WAVE", 4);
    /* fmt chunk */
    memcpy(wav + 12, "fmt ", 4);
    write_u32(wav + 16, 16);
    write_u16(wav + 20, 1);     /* PCM */
    write_u16(wav + 22, 1);     /* mono */
    write_u32(wav + 24, 44100);
    write_u16(wav + 34, 16);
    /* JUNK chunk */
    memcpy(wav + 36, "JUNK", 4);
    write_u32(wav + 40, 4);
    /* data chunk: 2 samples */
    memcpy(wav + 48, "data", 4);
    write_u32(wav + 52, 4);
    write_u16(wav + 56, 16384);          /* +0.5 */
    write_u16(wav + 58, (uint16_t)-16384); /* -0.5 */
    write_u32(wav + 4, 52);

    GFAudioSample s;
    GF_ASSERT(gf_load_wav_memory(wav, 60, &s));
    GF_ASSERT(s.frame_count == 2 && s.channels == 1 && s.sample_rate == 44100);
    GF_ASSERT_NEAR(s.samples[0], 0.5f, 1e-4);
    GF_ASSERT_NEAR(s.samples[1], -0.5f, 1e-4);
    gf_audio_sample_free(&s);
    gf_audio_sample_free(NULL);

    /* 32-bit IEEE float stereo WAV */
    uint8_t fwav[64];
    memset(fwav, 0, sizeof(fwav));
    memcpy(fwav, "RIFF", 4);
    memcpy(fwav + 8, "WAVE", 4);
    memcpy(fwav + 12, "fmt ", 4);
    write_u32(fwav + 16, 16);
    write_u16(fwav + 20, 3); /* IEEE float */
    write_u16(fwav + 22, 2); /* stereo */
    write_u32(fwav + 24, 48000);
    write_u16(fwav + 34, 32);
    memcpy(fwav + 36, "data", 4);
    write_u32(fwav + 40, 8);
    float flr[2] = {0.25f, -0.75f};
    memcpy(fwav + 44, flr, 8);
    write_u32(fwav + 4, 44);

    GF_ASSERT(gf_load_wav_memory(fwav, 52, &s));
    GF_ASSERT(s.frame_count == 1 && s.channels == 2);
    GF_ASSERT_NEAR(s.samples[0], 0.25f, 1e-6);
    GF_ASSERT_NEAR(s.samples[1], -0.75f, 1e-6);
    gf_audio_sample_free(&s);

    /* Invalid WAVs */
    GF_ASSERT(!gf_load_wav_memory(NULL, 0, &s));
    GF_ASSERT(!gf_load_wav_memory(fwav, 10, &s));
    fwav[0] = 'X';
    GF_ASSERT(!gf_load_wav_memory(fwav, 52, &s));
    fwav[0] = 'R';
    write_u16(fwav + 20, 99);
    GF_ASSERT(!gf_load_wav_memory(fwav, 52, &s));
}

static void test_shader_and_font_loaders(void) {
    char *out = NULL;
    GF_ASSERT(!gf_shader_preprocess(NULL, false, &out));
    GF_ASSERT(gf_shader_preprocess("#version 330 core\nvoid main(){}", true, &out));
    GF_ASSERT(strstr(out, "#version 300 es") != NULL);
    GF_ASSERT(strstr(out, "precision highp float;") != NULL);
    GF_ASSERT(strstr(out, "void main(){}") != NULL);
    free(out);

    GF_ASSERT(gf_shader_preprocess("void main(){}", false, &out));
    GF_ASSERT(strstr(out, "#version 330 core") != NULL);
    free(out);

    const char *fnt =
        "atlas fonts/retro_font.tga\n"
        "cell_w 8\n"
        "cell_h 8\n"
        "cols 16\n"
        "rows 6\n"
        "first_char 32\n"
        "char_count 96\n";
    GFFontMetrics fm;
    GF_ASSERT(gf_load_font_metrics_memory(fnt, strlen(fnt), &fm));
    GF_ASSERT(fm.cell_w == 8 && fm.cell_h == 8 && fm.cols == 16 && fm.first_char == 32);
    GF_ASSERT_STR_EQ(fm.atlas_path, "fonts/retro_font.tga");
    GF_ASSERT(!gf_load_font_metrics_memory(NULL, 0, &fm));
    GF_ASSERT(!gf_load_font_metrics_memory("cell_w 8\n", 9, &fm));
}

static void test_real_external_assets_pack(void) {
    GFAssetManager am;
    unsetenv("GREEN_FAIRY_ASSETS");
    GF_ASSERT(gf_assets_init(&am, NULL, NULL)); /* Uses working dir ./assets */

    uint8_t *data = NULL;
    size_t sz = 0;

    static const char *const k_models[] = {
        "models/crystal_octa.obj",
        "models/room_shell.obj",
        "models/archway_conduit.obj",
        "models/rack_cabinet.obj",
        "models/knob_rotary.obj",
        "models/fader_cap.obj",
        "models/switch_toggle.obj",
        "models/jack_socket.obj",
        "models/piano_key_3d.obj",
        "models/morph_pedestal.obj",
        "models/attenuverter_ring.obj"
    };
    for (size_t i = 0; i < sizeof(k_models) / sizeof(k_models[0]); ++i) {
        GF_ASSERT(gf_assets_read_file(&am, k_models[i], &data, &sz));
        GFMeshData mesh;
        GF_ASSERT(gf_load_obj_memory((const char *)data, sz, &mesh));
        GF_ASSERT(mesh.index_count >= 24);
        GF_ASSERT(mesh.vertex_count >= 6);
        gf_mesh_data_free(&mesh);
        free(data);
    }

    static const char *const k_shaders[] = {
        "shaders/cable.vert.glsl",
        "shaders/cable.frag.glsl"
    };
    for (size_t i = 0; i < sizeof(k_shaders) / sizeof(k_shaders[0]); ++i) {
        GF_ASSERT(gf_assets_read_file(&am, k_shaders[i], &data, &sz));
        char *gl_src = NULL;
        GF_ASSERT(gf_shader_preprocess((const char *)data, false, &gl_src));
        GF_ASSERT(strstr(gl_src, "#version 330 core") != NULL);
        free(gl_src);
        GF_ASSERT(gf_shader_preprocess((const char *)data, true, &gl_src));
        GF_ASSERT(strstr(gl_src, "#version 300 es") != NULL);
        free(gl_src);
        free(data);
    }

    GF_ASSERT(gf_assets_read_file(&am, "sprites/fairy_wisp.tga", &data, &sz));
    GFImageData img;
    GF_ASSERT(gf_load_image_memory(data, sz, &img));
    GF_ASSERT(img.width == 64 && img.height == 16);
    gf_image_data_free(&img);
    free(data);

    GF_ASSERT(gf_assets_read_file(&am, "voxels/fairy_spire.gfvox", &data, &sz));
    GFVoxelGrid grid;
    GF_ASSERT(gf_load_voxel_memory((const char *)data, sz, &grid));
    GF_ASSERT(grid.sx == 7 && grid.sy == 9 && grid.sz == 7);
    gf_voxel_grid_free(&grid);
    free(data);

    GF_ASSERT(gf_assets_read_file(&am, "sounds/absinthe_bell.wav", &data, &sz));
    GFAudioSample samp;
    GF_ASSERT(gf_load_wav_memory(data, sz, &samp));
    GF_ASSERT(samp.frame_count > 1000 && samp.sample_rate == 44100);
    gf_audio_sample_free(&samp);
    free(data);

    GF_ASSERT(gf_assets_read_file(&am, "fonts/retro_font.fnt", &data, &sz));
    GFFontMetrics fm;
    GF_ASSERT(gf_load_font_metrics_memory((const char *)data, sz, &fm));
    GF_ASSERT(fm.cell_w == 8 && fm.cols == 16);
    free(data);
}

int main(void) {
    GF_RUN_TEST(test_asset_manager_and_hot_reload);
    GF_RUN_TEST(test_hot_reload_behavior);
    GF_RUN_TEST(test_obj_loader);
    GF_RUN_TEST(test_image_loaders);
    GF_RUN_TEST(test_voxel_loader);
    GF_RUN_TEST(test_wav_loader);
    GF_RUN_TEST(test_shader_and_font_loaders);
    GF_RUN_TEST(test_real_external_assets_pack);
    GF_TEST_SUMMARY();
}
