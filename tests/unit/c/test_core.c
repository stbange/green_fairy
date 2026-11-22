#include "test_harness.h"

#include "core/log.h"
#include "core/math3d.h"
#include "core/path.h"
#include "core/prng.h"
#include "core/ringbuf.h"

static void test_math_vectors(void) {
    GF_ASSERT_NEAR(gf_clampf(-1.0f, 0.0f, 1.0f), 0.0f, 1e-6);
    GF_ASSERT_NEAR(gf_clampf(2.0f, 0.0f, 1.0f), 1.0f, 1e-6);
    GF_ASSERT_NEAR(gf_clampf(0.4f, 0.0f, 1.0f), 0.4f, 1e-6);
    GF_ASSERT_NEAR(gf_lerpf(10.0f, 20.0f, 0.25f), 12.5f, 1e-6);

    GFVec3 a = gf_vec3(1.0f, 2.0f, 3.0f);
    GFVec3 b = gf_vec3(4.0f, 5.0f, 6.0f);
    GFVec3 sum = gf_vec3_add(a, b);
    GF_ASSERT_NEAR(sum.x, 5.0f, 1e-6);
    GF_ASSERT_NEAR(sum.y, 7.0f, 1e-6);
    GF_ASSERT_NEAR(sum.z, 9.0f, 1e-6);

    GFVec3 diff = gf_vec3_sub(b, a);
    GF_ASSERT_NEAR(diff.x, 3.0f, 1e-6);
    GF_ASSERT_NEAR(gf_vec3_dot(a, b), 32.0f, 1e-6);

    GFVec3 c = gf_vec3_cross(gf_vec3(1, 0, 0), gf_vec3(0, 1, 0));
    GF_ASSERT_NEAR(c.z, 1.0f, 1e-6);

    GFVec3 n = gf_vec3_normalize(gf_vec3(3, 4, 0));
    GF_ASSERT_NEAR(gf_vec3_len(n), 1.0f, 1e-6);
    GFVec3 z = gf_vec3_normalize(gf_vec3(0, 0, 0));
    GF_ASSERT_NEAR(gf_vec3_len(z), 0.0f, 1e-6);

    GFVec3 l = gf_vec3_lerp(gf_vec3(0, 0, 0), gf_vec3(2, 4, 6), 0.5f);
    GF_ASSERT_NEAR(l.x, 1.0f, 1e-6);
    GF_ASSERT_NEAR(l.y, 2.0f, 1e-6);
    GF_ASSERT_NEAR(l.z, 3.0f, 1e-6);
}

static void test_math_matrices_and_quats(void) {
    GFMat4 id = gf_mat4_identity();
    GFVec4 p = {1.0f, 2.0f, 3.0f, 1.0f};
    GFVec4 p2 = gf_mat4_mul_vec4(id, p);
    GF_ASSERT_NEAR(p2.x, 1.0f, 1e-6);
    GF_ASSERT_NEAR(p2.y, 2.0f, 1e-6);
    GF_ASSERT_NEAR(p2.z, 3.0f, 1e-6);

    GFMat4 trs = gf_mat4_trs(gf_vec3(10, 0, 0), gf_vec3(0, 0, GF_PI * 0.5f), gf_vec3(2, 2, 2));
    GFVec4 v = gf_mat4_mul_vec4(trs, (GFVec4){1, 0, 0, 1});
    GF_ASSERT_NEAR(v.x, 10.0f, 1e-5);
    GF_ASSERT_NEAR(v.y, 2.0f, 1e-5);
    GF_ASSERT_NEAR(v.z, 0.0f, 1e-5);

    GFMat4 rx = gf_mat4_rotate_x(GF_PI * 0.5f);
    GFVec4 vy = gf_mat4_mul_vec4(rx, (GFVec4){0, 1, 0, 1});
    GF_ASSERT_NEAR(vy.z, 1.0f, 1e-5);

    GFMat4 ry = gf_mat4_rotate_y(GF_PI * 0.5f);
    GFVec4 vz = gf_mat4_mul_vec4(ry, (GFVec4){0, 0, 1, 1});
    GF_ASSERT_NEAR(vz.x, 1.0f, 1e-5);

    GFMat4 persp = gf_mat4_perspective(GF_PI * 0.5f, 16.0f / 9.0f, 0.1f, 100.0f);
    GF_ASSERT(persp.m[11] == -1.0f);
    GFMat4 persp_zero = gf_mat4_perspective(GF_PI * 0.5f, 0.0f, 0.1f, 100.0f);
    GF_ASSERT_NEAR(persp_zero.m[0], 1.0f, 1e-5);

    GFMat4 ortho = gf_mat4_ortho(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f);
    GFVec4 o = gf_mat4_mul_vec4(ortho, (GFVec4){400.0f, 300.0f, 0.0f, 1.0f});
    GF_ASSERT_NEAR(o.x, 0.0f, 1e-5);
    GF_ASSERT_NEAR(o.y, 0.0f, 1e-5);

    GFMat4 view = gf_mat4_look_at(gf_vec3(0, 0, 5), gf_vec3(0, 0, 0), gf_vec3(0, 1, 0));
    GFVec4 origin = gf_mat4_mul_vec4(view, (GFVec4){0, 0, 0, 1});
    GF_ASSERT_NEAR(origin.z, -5.0f, 1e-5);

    GFQuat q1 = gf_quat_from_euler(gf_vec3(0.0f, GF_PI * 0.5f, 0.0f));
    GFQuat q2 = gf_quat_from_euler(gf_vec3(0.0f, GF_PI * 0.5f, 0.0f));
    GFQuat qm = gf_quat_mul(q1, q2);
    GFMat4 qmat = gf_quat_to_mat4(qm);
    GFVec4 qv = gf_mat4_mul_vec4(qmat, (GFVec4){1, 0, 0, 1});
    GF_ASSERT_NEAR(qv.x, -1.0f, 1e-5);

    GFQuat qzero = gf_quat_normalize((GFQuat){0, 0, 0, 0});
    GF_ASSERT_NEAR(qzero.w, 1.0f, 1e-6);
}

static void test_ringbuf(void) {
    int backing[4];
    GFRingBuf rb;
    GF_ASSERT(!gf_ringbuf_init(NULL, backing, sizeof(int), 4));
    GF_ASSERT(!gf_ringbuf_init(&rb, NULL, sizeof(int), 4));
    GF_ASSERT(!gf_ringbuf_init(&rb, backing, 0, 4));
    GF_ASSERT(!gf_ringbuf_init(&rb, backing, sizeof(int), 3));
    GF_ASSERT(gf_ringbuf_init(&rb, backing, sizeof(int), 4));

    int out = 0;
    GF_ASSERT(!gf_ringbuf_push(NULL, &out));
    GF_ASSERT(!gf_ringbuf_pop(NULL, &out));
    GF_ASSERT(!gf_ringbuf_pop(&rb, &out));
    GF_ASSERT(gf_ringbuf_count(NULL) == 0);
    GF_ASSERT(gf_ringbuf_count(&rb) == 0);

    for (int i = 1; i <= 4; ++i) {
        GF_ASSERT(gf_ringbuf_push(&rb, &i));
    }
    int extra = 5;
    GF_ASSERT(!gf_ringbuf_push(&rb, &extra));
    GF_ASSERT(gf_ringbuf_count(&rb) == 4);

    for (int i = 1; i <= 4; ++i) {
        GF_ASSERT(gf_ringbuf_pop(&rb, &out));
        GF_ASSERT(out == i);
    }
    GF_ASSERT(gf_ringbuf_count(&rb) == 0);

    GF_ASSERT(gf_ringbuf_push(&rb, &extra));
    gf_ringbuf_reset(&rb);
    gf_ringbuf_reset(NULL);
    GF_ASSERT(gf_ringbuf_count(&rb) == 0);
}

static void test_prng(void) {
    GFPrng a, b;
    gf_prng_seed(NULL, 123);
    gf_prng_seed(&a, 0);
    gf_prng_seed(&b, 0);
    for (int i = 0; i < 64; ++i) {
        float fa = gf_prng_f32(&a);
        float fb = gf_prng_f32(&b);
        GF_ASSERT(fa == fb);
        GF_ASSERT(fa >= 0.0f && fa < 1.0f);
        float ba = gf_prng_bipolar(&a);
        float bb = gf_prng_bipolar(&b);
        GF_ASSERT(ba == bb);
        GF_ASSERT(ba >= -1.0f && ba < 1.0f);
    }
    a.state = 0;
    GF_ASSERT(gf_prng_u32(&a) != 0);
}

static void test_path_safety(void) {
    char buf[256];
    GF_ASSERT(gf_path_normalize_rel("scripts\\ui/./layout.lua", buf, sizeof(buf)));
    GF_ASSERT_STR_EQ(buf, "scripts/ui/layout.lua");

    GF_ASSERT(!gf_path_normalize_rel("", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_normalize_rel(NULL, buf, sizeof(buf)));
    GF_ASSERT(!gf_path_normalize_rel("/etc/passwd", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_normalize_rel("\\windows\\win.ini", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_normalize_rel("C:\\secret.txt", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_normalize_rel("scripts/../../secret.lua", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_normalize_rel("./.", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_normalize_rel("very/long/path/name", buf, 5));

    GF_ASSERT(gf_path_join_safe("/app/assets/", "patches/emerald.lua", buf, sizeof(buf)));
    GF_ASSERT_STR_EQ(buf, "/app/assets/patches/emerald.lua");
    GF_ASSERT(gf_path_join_safe("/", "assets/main.lua", buf, sizeof(buf)));
    GF_ASSERT_STR_EQ(buf, "/assets/main.lua");
    GF_ASSERT(!gf_path_join_safe("", "a.lua", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_join_safe("/app", "../bad", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_join_safe("/app/assets", "patches/emerald.lua", buf, 8));

    GF_ASSERT(gf_path_dirname("/usr/local/bin/green_fairy", buf, sizeof(buf)));
    GF_ASSERT_STR_EQ(buf, "/usr/local/bin");
    GF_ASSERT(gf_path_dirname("green_fairy", buf, sizeof(buf)));
    GF_ASSERT_STR_EQ(buf, ".");
    GF_ASSERT(gf_path_dirname("/green_fairy/", buf, sizeof(buf)));
    GF_ASSERT_STR_EQ(buf, "/");
    GF_ASSERT(!gf_path_dirname("", buf, sizeof(buf)));
    GF_ASSERT(!gf_path_dirname("/very/long/dir/file", buf, 4));
}

typedef struct LogCapture {
    int count;
    GFLogLevel last_level;
    char last_tag[32];
    char last_msg[128];
} LogCapture;

static void test_log_sink(GFLogLevel level, const char *tag, const char *message, void *user_data) {
    LogCapture *cap = (LogCapture *)user_data;
    cap->count++;
    cap->last_level = level;
    snprintf(cap->last_tag, sizeof(cap->last_tag), "%s", tag);
    snprintf(cap->last_msg, sizeof(cap->last_msg), "%s", message);
}

static void test_logging(void) {
    LogCapture cap;
    memset(&cap, 0, sizeof(cap));

    gf_log_set_level(GF_LOG_WARN);
    GF_ASSERT(gf_log_get_level() == GF_LOG_WARN);
    gf_log_set_sink(test_log_sink, &cap);

    GF_LOGD("core", "ignored debug");
    GF_LOGI("core", "ignored info");
    GF_ASSERT(cap.count == 0);

    GF_LOGW("core", "warn %d", 42);
    GF_ASSERT(cap.count == 1);
    GF_ASSERT(cap.last_level == GF_LOG_WARN);
    GF_ASSERT_STR_EQ(cap.last_tag, "core");
    GF_ASSERT_STR_EQ(cap.last_msg, "warn 42");

    gf_log_write(GF_LOG_ERROR, NULL, "err");
    GF_ASSERT(cap.count == 2);
    GF_ASSERT_STR_EQ(cap.last_tag, "gf");

    gf_log_set_sink(NULL, NULL);
    gf_log_set_level(GF_LOG_DEBUG);
    GF_LOGD("test", "stderr debug");
    GF_LOGI("test", "stderr info");
    GF_LOGW("test", "stderr warn");
    GF_LOGE("test", "stderr error");
    gf_log_write((GFLogLevel)99, "test", "fallback level");
    gf_log_write(GF_LOG_ERROR, "test", NULL);
}

int main(void) {
    GF_RUN_TEST(test_math_vectors);
    GF_RUN_TEST(test_math_matrices_and_quats);
    GF_RUN_TEST(test_ringbuf);
    GF_RUN_TEST(test_prng);
    GF_RUN_TEST(test_path_safety);
    GF_RUN_TEST(test_logging);
    GF_TEST_SUMMARY();
}
