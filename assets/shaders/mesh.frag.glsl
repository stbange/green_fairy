in vec4 v_color;
in vec2 v_uv;
in float v_light;

uniform vec4 u_tint;
uniform float u_color_steps;
uniform float u_dither;
uniform float u_use_tex;
uniform float u_unlit;
uniform sampler2D u_tex;

out vec4 frag_color;

void main() {
    float lit = mix(v_light, 1.0, clamp(u_unlit, 0.0, 1.0));
    vec4 base = v_color * u_tint;
    if (u_use_tex > 0.5) {
        vec4 texel = texture(u_tex, v_uv);
        if (texel.a < 0.15) {
            discard;
        }
        base *= texel;
    }
    vec3 col = base.rgb * lit;
    if (u_dither > 0.5 && u_unlit < 0.5) {
        vec2 p = mod(floor(gl_FragCoord.xy), 2.0);
        float bayer = (p.x + p.y * 2.0) * 0.25 - 0.375;
        col += bayer / max(u_color_steps, 8.0);
    }
    if (u_color_steps > 1.0 && u_unlit < 0.5) {
        col = floor(clamp(col, 0.0, 1.0) * u_color_steps + 0.5) / u_color_steps;
    }
    frag_color = vec4(clamp(col, 0.0, 1.0), base.a);
}
