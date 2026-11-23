#ifndef GF_RENDER_GL_RENDERER_H
#define GF_RENDER_GL_RENDERER_H

#include "assets/asset_manager.h"
#include "render/gl_api.h"
#include "render/scene.h"

#include <stdbool.h>
#include <stdint.h>

#define GF_GL_CACHE_MAX   128
#define GF_DYN3D_MAX_VERTS 32768

typedef struct GFGpuMesh {
    char path[128];
    bool is_voxel;
    GFGLuint vao;
    GFGLuint vbo;
    GFGLuint ebo;
    uint32_t index_count;
} GFGpuMesh;

typedef struct GFGpuTex {
    char path[128];
    GFGLuint tex;
    int width;
    int height;
} GFGpuTex;

typedef struct GFGlRenderer {
    GFGlApi gl;
    const GFAssetManager *am;
    bool is_gles;
    bool initialized;

    GFGLuint prog_mesh;
    GFGLuint prog_cable;
    GFGLuint prog_sprite;
    GFGLuint prog_hud;

    GFGLuint sprite_vao, sprite_vbo;
    GFGLuint ribbon_vao, ribbon_vbo;
    GFGLuint dyn3d_vao, dyn3d_vbo;
    GFGLuint cable_vao, cable_vbo;
    GFGLuint hud_vao, hud_vbo;

    GFVertex *dyn3d_cpu_buf;
    GFVertex *cable_cpu_buf;

    GFGpuMesh meshes[GF_GL_CACHE_MAX];
    int mesh_count;
    GFGpuTex textures[GF_GL_CACHE_MAX];
    int tex_count;
    GFGLuint font_tex;

    uint32_t last_draw_calls;
    int last_visible_rooms;
} GFGlRenderer;

bool gf_gl_renderer_init(GFGlRenderer *r, const GFGlApi *api, const GFAssetManager *am, bool is_gles);
bool gf_gl_renderer_reload_shaders(GFGlRenderer *r);
void gf_gl_renderer_destroy(GFGlRenderer *r);

void gf_gl_renderer_render_view(GFGlRenderer *r, GFScene *sc, const GFAudioTelemetry *tel,
                                const GFMat4 *view, const GFMat4 *proj,
                                int vp_x, int vp_y, int vp_w, int vp_h, bool clear_bg);
void gf_gl_renderer_render_hud(GFGlRenderer *r, const GFHudBatch *hb, int vp_w, int vp_h);
void gf_gl_renderer_render_xr_eye(GFGlRenderer *r, GFScene *sc, const GFHudBatch *hb,
                                  const GFAudioTelemetry *tel, const GFMat4 *view, const GFMat4 *proj,
                                  int vp_x, int vp_y, int vp_w, int vp_h);
bool gf_gl_renderer_read_rgba(const GFGlRenderer *r, int w, int h, uint8_t *out_rgba);

#endif
