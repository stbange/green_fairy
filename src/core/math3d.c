#include "core/math3d.h"

#include <string.h>

float gf_clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float gf_lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

GFVec3 gf_vec3(float x, float y, float z) {
    GFVec3 v = {x, y, z};
    return v;
}

GFVec3 gf_vec3_add(GFVec3 a, GFVec3 b) {
    return gf_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

GFVec3 gf_vec3_sub(GFVec3 a, GFVec3 b) {
    return gf_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

GFVec3 gf_vec3_scale(GFVec3 v, float s) {
    return gf_vec3(v.x * s, v.y * s, v.z * s);
}

float gf_vec3_dot(GFVec3 a, GFVec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

GFVec3 gf_vec3_cross(GFVec3 a, GFVec3 b) {
    return gf_vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

float gf_vec3_len(GFVec3 v) {
    return sqrtf(gf_vec3_dot(v, v));
}

GFVec3 gf_vec3_normalize(GFVec3 v) {
    float len = gf_vec3_len(v);
    if (len <= 1e-8f) {
        return gf_vec3(0.0f, 0.0f, 0.0f);
    }
    return gf_vec3_scale(v, 1.0f / len);
}

GFVec3 gf_vec3_lerp(GFVec3 a, GFVec3 b, float t) {
    return gf_vec3(
        gf_lerpf(a.x, b.x, t),
        gf_lerpf(a.y, b.y, t),
        gf_lerpf(a.z, b.z, t)
    );
}

GFMat4 gf_mat4_identity(void) {
    GFMat4 r;
    memset(&r, 0, sizeof(r));
    r.m[0] = 1.0f;
    r.m[5] = 1.0f;
    r.m[10] = 1.0f;
    r.m[15] = 1.0f;
    return r;
}

GFMat4 gf_mat4_mul(GFMat4 a, GFMat4 b) {
    GFMat4 r;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

GFVec4 gf_mat4_mul_vec4(GFMat4 m, GFVec4 v) {
    GFVec4 r;
    r.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w;
    r.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w;
    r.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w;
    r.w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w;
    return r;
}

GFMat4 gf_mat4_translate(GFVec3 t) {
    GFMat4 r = gf_mat4_identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

GFMat4 gf_mat4_scale(GFVec3 s) {
    GFMat4 r = gf_mat4_identity();
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    return r;
}

GFMat4 gf_mat4_rotate_x(float radians) {
    float c = cosf(radians);
    float s = sinf(radians);
    GFMat4 r = gf_mat4_identity();
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}

GFMat4 gf_mat4_rotate_y(float radians) {
    float c = cosf(radians);
    float s = sinf(radians);
    GFMat4 r = gf_mat4_identity();
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}

GFMat4 gf_mat4_rotate_z(float radians) {
    float c = cosf(radians);
    float s = sinf(radians);
    GFMat4 r = gf_mat4_identity();
    r.m[0] = c;
    r.m[1] = s;
    r.m[4] = -s;
    r.m[5] = c;
    return r;
}

GFMat4 gf_mat4_trs(GFVec3 pos, GFVec3 euler_rad, GFVec3 scale) {
    GFMat4 t = gf_mat4_translate(pos);
    GFMat4 rx = gf_mat4_rotate_x(euler_rad.x);
    GFMat4 ry = gf_mat4_rotate_y(euler_rad.y);
    GFMat4 rz = gf_mat4_rotate_z(euler_rad.z);
    GFMat4 s = gf_mat4_scale(scale);
    GFMat4 rot = gf_mat4_mul(rz, gf_mat4_mul(ry, rx));
    return gf_mat4_mul(t, gf_mat4_mul(rot, s));
}

GFMat4 gf_mat4_perspective(float fov_y_rad, float aspect, float near_z, float far_z) {
    GFMat4 r;
    memset(&r, 0, sizeof(r));
    float f = 1.0f / tanf(fov_y_rad * 0.5f);
    float nf = 1.0f / (near_z - far_z);
    r.m[0] = f / (aspect > 1e-6f ? aspect : 1.0f);
    r.m[5] = f;
    r.m[10] = (far_z + near_z) * nf;
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * far_z * near_z) * nf;
    return r;
}

GFMat4 gf_mat4_ortho(float left, float right, float bottom, float top, float near_z, float far_z) {
    GFMat4 r = gf_mat4_identity();
    float rl = right - left;
    float tb = top - bottom;
    float fn = far_z - near_z;
    r.m[0] = 2.0f / rl;
    r.m[5] = 2.0f / tb;
    r.m[10] = -2.0f / fn;
    r.m[12] = -(right + left) / rl;
    r.m[13] = -(top + bottom) / tb;
    r.m[14] = -(far_z + near_z) / fn;
    return r;
}

GFMat4 gf_mat4_look_at(GFVec3 eye, GFVec3 target, GFVec3 up) {
    GFVec3 f = gf_vec3_normalize(gf_vec3_sub(target, eye));
    GFVec3 s = gf_vec3_normalize(gf_vec3_cross(f, up));
    GFVec3 u = gf_vec3_cross(s, f);
    GFMat4 r = gf_mat4_identity();
    r.m[0] = s.x;  r.m[4] = s.y;  r.m[8] = s.z;   r.m[12] = -gf_vec3_dot(s, eye);
    r.m[1] = u.x;  r.m[5] = u.y;  r.m[9] = u.z;   r.m[13] = -gf_vec3_dot(u, eye);
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z; r.m[14] = gf_vec3_dot(f, eye);
    return r;
}

bool gf_mat4_invert(GFMat4 in, GFMat4 *out) {
    if (!out) return false;
    const float *m = in.m;
    float inv[16];

    inv[0] =   m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15]
             + m[9] * m[7]  * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] =  -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15]
             - m[8] * m[7]  * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] =   m[4] * m[9]  * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15]
             + m[8] * m[7]  * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9]  * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14]
             - m[8] * m[6]  * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

    inv[1] =  -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15]
             - m[9] * m[3]  * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] =   m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15]
             + m[8] * m[3]  * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] =  -m[0] * m[9]  * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15]
             - m[8] * m[3]  * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] =  m[0] * m[9]  * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14]
             + m[8] * m[2]  * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

    inv[2] =   m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15]
             + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] =  -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15]
             - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] =  m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15]
             + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14]
             - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

    inv[3] =  -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11]
             - m[5] * m[3] * m[10] - m[9] * m[2] * m[7]  + m[9] * m[3] * m[6];
    inv[7] =   m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11]
             + m[4] * m[3] * m[10] + m[8] * m[2] * m[7]  - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9]  + m[4] * m[1] * m[11]
             - m[4] * m[3] * m[9]  - m[8] * m[1] * m[7]  + m[8] * m[3] * m[5];
    inv[15] =  m[0] * m[5] * m[10] - m[0] * m[6] * m[9]  - m[4] * m[1] * m[10]
             + m[4] * m[2] * m[9]  + m[8] * m[1] * m[6]  - m[8] * m[2] * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (fabsf(det) < 1e-12f) return false;
    float inv_det = 1.0f / det;
    for (int i = 0; i < 16; ++i) out->m[i] = inv[i] * inv_det;
    return true;
}

GFQuat gf_quat_from_euler(GFVec3 euler_rad) {
    float cx = cosf(euler_rad.x * 0.5f), sx = sinf(euler_rad.x * 0.5f);
    float cy = cosf(euler_rad.y * 0.5f), sy = sinf(euler_rad.y * 0.5f);
    float cz = cosf(euler_rad.z * 0.5f), sz = sinf(euler_rad.z * 0.5f);
    GFQuat q;
    q.w = cx * cy * cz + sx * sy * sz;
    q.x = sx * cy * cz - cx * sy * sz;
    q.y = cx * sy * cz + sx * cy * sz;
    q.z = cx * cy * sz - sx * sy * cz;
    return q;
}

GFQuat gf_quat_normalize(GFQuat q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len <= 1e-8f) {
        GFQuat id = {0.0f, 0.0f, 0.0f, 1.0f};
        return id;
    }
    float inv = 1.0f / len;
    GFQuat r = {q.x * inv, q.y * inv, q.z * inv, q.w * inv};
    return r;
}

GFQuat gf_quat_mul(GFQuat a, GFQuat b) {
    GFQuat r;
    r.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    r.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    r.y = a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
    r.z = a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
    return r;
}

GFMat4 gf_quat_to_mat4(GFQuat q) {
    q = gf_quat_normalize(q);
    float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    GFMat4 r = gf_mat4_identity();
    r.m[0] = 1.0f - 2.0f * (yy + zz);
    r.m[1] = 2.0f * (xy + wz);
    r.m[2] = 2.0f * (xz - wy);
    r.m[4] = 2.0f * (xy - wz);
    r.m[5] = 1.0f - 2.0f * (xx + zz);
    r.m[6] = 2.0f * (yz + wx);
    r.m[8] = 2.0f * (xz + wy);
    r.m[9] = 2.0f * (yz - wx);
    r.m[10] = 1.0f - 2.0f * (xx + yy);
    return r;
}
