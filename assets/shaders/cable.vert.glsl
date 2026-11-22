layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform vec3 u_light_dir;
uniform float u_snap_res;

out vec4 v_color;
out vec2 v_uv;
out float v_light;
out float v_rim;

void main() {
    vec4 view_pos = u_view * vec4(a_pos, 1.0);
    vec4 clip = u_proj * view_pos;
    if (u_snap_res > 1.0) {
        vec2 ndc = clip.xy / max(abs(clip.w), 0.0001);
        ndc = floor(ndc * u_snap_res + 0.5) / u_snap_res;
        clip.xy = ndc * clip.w;
    }
    gl_Position = clip;

    vec3 n = normalize(a_normal);
    float diff = max(dot(n, normalize(u_light_dir)), 0.0);
    vec3 view_n = normalize(mat3(u_view) * n);
    vec3 view_dir = normalize(-view_pos.xyz);
    v_rim = pow(1.0 - max(dot(view_n, view_dir), 0.0), 2.0);
    v_light = 0.35 + 0.65 * diff;
    v_color = a_color;
    v_uv = a_uv;
}
