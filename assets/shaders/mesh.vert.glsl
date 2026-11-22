layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform vec3 u_light_dir;
uniform float u_snap_res;
uniform float u_audio_pulse;

out vec4 v_color;
out vec2 v_uv;
out float v_light;

void main() {
    vec4 world_pos = u_model * vec4(a_pos, 1.0);
    vec4 clip = u_proj * u_view * world_pos;
    if (u_snap_res > 1.0 && clip.w > 0.25) {
        vec2 ndc = clip.xy / clip.w;
        ndc = floor(ndc * u_snap_res + 0.5) / u_snap_res;
        clip.xy = ndc * clip.w;
    }
    gl_Position = clip;
    vec3 world_n = normalize(mat3(u_model) * a_normal);
    float diff = max(dot(world_n, normalize(u_light_dir)), 0.0);
    v_light = 0.28 + 0.72 * diff + 0.25 * u_audio_pulse;
    v_color = a_color;
    v_uv = a_uv;
}
