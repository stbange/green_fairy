in vec4 v_color;
in vec2 v_uv;
in float v_light;

uniform vec4 u_tint;
uniform float u_color_steps;
uniform float u_dither;

out vec4 frag_color;

void main() {
    vec3 col = v_color.rgb * u_tint.rgb * v_light;
    if (u_dither > 0.5) {
        vec2 p = mod(floor(gl_FragCoord.xy), 2.0);
        float bayer = (p.x + p.y * 2.0) * 0.25 - 0.375;
        col += bayer / max(u_color_steps, 8.0);
    }
    if (u_color_steps > 1.0) {
        col = floor(clamp(col, 0.0, 1.0) * u_color_steps + 0.5) / u_color_steps;
    }
    frag_color = vec4(clamp(col, 0.0, 1.0), v_color.a * u_tint.a);
}
