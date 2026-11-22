in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_tex;
uniform float u_use_tex;

out vec4 frag_color;

void main() {
    if (u_use_tex > 0.5) {
        vec4 tex = texture(u_tex, v_uv);
        frag_color = tex * v_color;
    } else {
        frag_color = v_color;
    }
}
