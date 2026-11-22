in vec4 v_color;
in vec2 v_uv;
in float v_light;
in float v_rim;

uniform float u_time;
uniform float u_color_steps;

out vec4 frag_color;

void main() {
    float arc_t = v_uv.x;
    float sig_level = clamp(v_uv.y, 0.0, 2.0);

    /* Directional voltage pulses flowing from source jack (0.0) to dest jack (1.0) */
    float speed = 1.6 + sig_level * 4.5;
    float wave1 = 0.5 + 0.5 * sin((arc_t * 18.0 - u_time * speed));
    float wave2 = pow(0.5 + 0.5 * sin((arc_t * 6.0 - u_time * speed * 0.65)), 4.0);
    float pulse = (0.12 + sig_level * 0.95) * (0.35 * wave1 + 0.85 * wave2);

    vec3 jacket = v_color.rgb * v_light;
    vec3 core_glow = mix(v_color.rgb, vec3(0.95, 1.0, 0.96), 0.55) * pulse;
    vec3 rim_glow = v_color.rgb * v_rim * (0.25 + 0.55 * sig_level);
    vec3 col = jacket + core_glow + rim_glow;

    if (u_color_steps > 1.0) {
        col = floor(clamp(col, 0.0, 1.0) * u_color_steps + 0.5) / u_color_steps;
    }
    frag_color = vec4(clamp(col, 0.0, 1.0), v_color.a);
}
