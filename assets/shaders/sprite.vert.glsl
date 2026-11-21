layout(location = 0) in vec3 a_pos;
layout(location = 2) in vec2 a_uv;

uniform mat4 u_view;
uniform mat4 u_proj;
uniform vec3 u_world_pos;
uniform vec2 u_size;
uniform vec4 u_uv_rect; /* u0, v0, du, dv */

out vec2 v_uv;

void main() {
    vec3 cam_right = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
    vec3 cam_up    = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);
    vec3 pos = u_world_pos
             + cam_right * (a_pos.x * u_size.x)
             + cam_up    * (a_pos.y * u_size.y);
    gl_Position = u_proj * u_view * vec4(pos, 1.0);
    v_uv = u_uv_rect.xy + a_uv * u_uv_rect.zw;
}
