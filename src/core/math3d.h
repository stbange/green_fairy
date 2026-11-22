#ifndef GF_CORE_MATH3D_H
#define GF_CORE_MATH3D_H

#include <math.h>
#include <stddef.h>

#define GF_PI 3.14159265358979323846f
#define GF_TAU 6.28318530717958647692f

typedef struct GFVec2 {
    float x, y;
} GFVec2;

typedef struct GFVec3 {
    float x, y, z;
} GFVec3;

typedef struct GFVec4 {
    float x, y, z, w;
} GFVec4;

/* Column-major 4x4 matrix matching OpenGL/WebGL2 conventions. */
typedef struct GFMat4 {
    float m[16];
} GFMat4;

typedef struct GFQuat {
    float x, y, z, w;
} GFQuat;

float gf_clampf(float v, float lo, float hi);
float gf_lerpf(float a, float b, float t);

GFVec3 gf_vec3(float x, float y, float z);
GFVec3 gf_vec3_add(GFVec3 a, GFVec3 b);
GFVec3 gf_vec3_sub(GFVec3 a, GFVec3 b);
GFVec3 gf_vec3_scale(GFVec3 v, float s);
float gf_vec3_dot(GFVec3 a, GFVec3 b);
GFVec3 gf_vec3_cross(GFVec3 a, GFVec3 b);
float gf_vec3_len(GFVec3 v);
GFVec3 gf_vec3_normalize(GFVec3 v);
GFVec3 gf_vec3_lerp(GFVec3 a, GFVec3 b, float t);

GFMat4 gf_mat4_identity(void);
GFMat4 gf_mat4_mul(GFMat4 a, GFMat4 b);
GFVec4 gf_mat4_mul_vec4(GFMat4 m, GFVec4 v);
GFMat4 gf_mat4_translate(GFVec3 t);
GFMat4 gf_mat4_scale(GFVec3 s);
GFMat4 gf_mat4_rotate_x(float radians);
GFMat4 gf_mat4_rotate_y(float radians);
GFMat4 gf_mat4_rotate_z(float radians);
GFMat4 gf_mat4_trs(GFVec3 pos, GFVec3 euler_rad, GFVec3 scale);
GFMat4 gf_mat4_perspective(float fov_y_rad, float aspect, float near_z, float far_z);
#include <stdbool.h>
GFMat4 gf_mat4_ortho(float left, float right, float bottom, float top, float near_z, float far_z);
GFMat4 gf_mat4_look_at(GFVec3 eye, GFVec3 target, GFVec3 up);
bool gf_mat4_invert(GFMat4 m, GFMat4 *out);

GFQuat gf_quat_from_euler(GFVec3 euler_rad);
GFQuat gf_quat_normalize(GFQuat q);
GFQuat gf_quat_mul(GFQuat a, GFQuat b);
GFMat4 gf_quat_to_mat4(GFQuat q);

#endif
