in vec2 v_uv;

uniform sampler2D u_tex;
uniform vec4 u_tint;

out vec4 frag_color;

void main() {
    vec4 tex = texture(u_tex, v_uv);
    if (tex.a < 0.1) {
        discard;
    }
    frag_color = tex * u_tint;
}
