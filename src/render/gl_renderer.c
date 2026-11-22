#include "render/gl_renderer.h"

#include "assets/loaders.h"
#include "core/log.h"
#include "render/voxel_mesher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GFGLuint compile_shader(const GFGlApi *gl, GFGLenum type, const char *src) {
    GFGLuint sh = gl->CreateShader(type);
    if (!sh) return 0;
    gl->ShaderSource(sh, 1, &src, NULL);
    gl->CompileShader(sh);
    GFGLint ok = 0;
    gl->GetShaderiv(sh, GF_GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        GFGLsizei len = 0;
        memset(log, 0, sizeof(log));
        gl->GetShaderInfoLog(sh, (GFGLsizei)sizeof(log) - 1, &len, log);
        GF_LOGE("render", "Shader compile failed: %s", log);
        gl->DeleteShader(sh);
        return 0;
    }
    return sh;
}

static GFGLuint build_program_from_assets(const GFGlRenderer *r, const char *vert_rel, const char *frag_rel) {
    uint8_t *vraw = NULL, *fraw = NULL;
    size_t vsz = 0, fsz = 0;
    if (!gf_assets_read_file(r->am, vert_rel, &vraw, &vsz)) return 0;
    if (!gf_assets_read_file(r->am, frag_rel, &fraw, &fsz)) {
        free(vraw);
        return 0;
    }

    char *vsrc = NULL, *fsrc = NULL;
    bool ok = gf_shader_preprocess((const char *)vraw, r->is_gles, &vsrc) &&
              gf_shader_preprocess((const char *)fraw, r->is_gles, &fsrc);
    free(vraw);
    free(fraw);
    if (!ok) {
        free(vsrc); free(fsrc);
        return 0;
    }

    GFGLuint vs = compile_shader(&r->gl, GF_GL_VERTEX_SHADER, vsrc);
    GFGLuint fs = vs ? compile_shader(&r->gl, GF_GL_FRAGMENT_SHADER, fsrc) : 0;
    free(vsrc);
    free(fsrc);
    if (!vs || !fs) {
        if (vs) r->gl.DeleteShader(vs);
        if (fs) r->gl.DeleteShader(fs);
        return 0;
    }

    GFGLuint prog = r->gl.CreateProgram();
    r->gl.AttachShader(prog, vs);
    r->gl.AttachShader(prog, fs);
    r->gl.LinkProgram(prog);
    r->gl.DeleteShader(vs);
    r->gl.DeleteShader(fs);

    GFGLint linked = 0;
    r->gl.GetProgramiv(prog, GF_GL_LINK_STATUS, &linked);
    if (!linked) {
        char log[512];
        GFGLsizei len = 0;
        memset(log, 0, sizeof(log));
        r->gl.GetProgramInfoLog(prog, (GFGLsizei)sizeof(log) - 1, &len, log);
        GF_LOGE("render", "Program link failed: %s", log);
        r->gl.DeleteProgram(prog);
        return 0;
    }
    return prog;
}

static void setup_mesh_attribs(const GFGlApi *gl) {
    GFGLsizei stride = (GFGLsizei)sizeof(GFVertex);
    gl->EnableVertexAttribArray(0);
    gl->VertexAttribPointer(0, 3, GF_GL_FLOAT, 0, stride, (const void *)offsetof(GFVertex, px));
    gl->EnableVertexAttribArray(1);
    gl->VertexAttribPointer(1, 3, GF_GL_FLOAT, 0, stride, (const void *)offsetof(GFVertex, nx));
    gl->EnableVertexAttribArray(2);
    gl->VertexAttribPointer(2, 2, GF_GL_FLOAT, 0, stride, (const void *)offsetof(GFVertex, u));
    gl->EnableVertexAttribArray(3);
    gl->VertexAttribPointer(3, 4, GF_GL_FLOAT, 0, stride, (const void *)offsetof(GFVertex, r));
}

static GFGpuTex *get_or_load_texture(GFGlRenderer *r, const char *rel_path) {
    for (int i = 0; i < r->tex_count; ++i) {
        if (strcmp(r->textures[i].path, rel_path) == 0) {
            return &r->textures[i];
        }
    }
    if (r->tex_count >= GF_GL_CACHE_MAX) return NULL;

    uint8_t *raw = NULL;
    size_t sz = 0;
    if (!gf_assets_read_file(r->am, rel_path, &raw, &sz)) return NULL;
    GFImageData img;
    bool ok = gf_load_image_memory(raw, sz, &img);
    free(raw);
    if (!ok) return NULL;

    GFGpuTex *gt = &r->textures[r->tex_count++];
    memset(gt, 0, sizeof(*gt));
    snprintf(gt->path, sizeof(gt->path), "%s", rel_path);
    gt->width = img.width;
    gt->height = img.height;

    r->gl.GenTextures(1, &gt->tex);
    r->gl.BindTexture(GF_GL_TEXTURE_2D, gt->tex);
    r->gl.TexParameteri(GF_GL_TEXTURE_2D, GF_GL_TEXTURE_MIN_FILTER, GF_GL_NEAREST);
    r->gl.TexParameteri(GF_GL_TEXTURE_2D, GF_GL_TEXTURE_MAG_FILTER, GF_GL_NEAREST);
    r->gl.TexParameteri(GF_GL_TEXTURE_2D, GF_GL_TEXTURE_WRAP_S, GF_GL_CLAMP_TO_EDGE);
    r->gl.TexParameteri(GF_GL_TEXTURE_2D, GF_GL_TEXTURE_WRAP_T, GF_GL_CLAMP_TO_EDGE);
    r->gl.TexImage2D(GF_GL_TEXTURE_2D, 0, GF_GL_RGBA, img.width, img.height, 0,
                     GF_GL_RGBA, GF_GL_UNSIGNED_BYTE, img.rgba);
    gf_image_data_free(&img);
    return gt;
}

static GFGpuMesh *get_or_load_mesh(GFGlRenderer *r, const char *rel_path, bool is_voxel) {
    if (!rel_path || !rel_path[0]) return NULL;
    for (int i = 0; i < r->mesh_count; ++i) {
        if (r->meshes[i].is_voxel == is_voxel && strcmp(r->meshes[i].path, rel_path) == 0) {
            return &r->meshes[i];
        }
    }
    if (r->mesh_count >= GF_GL_CACHE_MAX) return NULL;

    uint8_t *raw = NULL;
    size_t sz = 0;
    if (!gf_assets_read_file(r->am, rel_path, &raw, &sz)) return NULL;

    GFMeshData md;
    bool ok = false;
    if (is_voxel) {
        GFVoxelGrid grid;
        if (gf_load_voxel_memory((const char *)raw, sz, &grid)) {
            ok = gf_voxel_build_mesh(&grid, 0.22f, &md);
            gf_voxel_grid_free(&grid);
        }
    } else {
        ok = gf_load_obj_memory((const char *)raw, sz, &md);
    }
    free(raw);
    if (!ok) return NULL;

    GFGpuMesh *gm = &r->meshes[r->mesh_count++];
    memset(gm, 0, sizeof(*gm));
    snprintf(gm->path, sizeof(gm->path), "%s", rel_path);
    gm->is_voxel = is_voxel;
    gm->index_count = md.index_count;

    r->gl.GenVertexArrays(1, &gm->vao);
    r->gl.GenBuffers(1, &gm->vbo);
    r->gl.GenBuffers(1, &gm->ebo);

    r->gl.BindVertexArray(gm->vao);
    r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, gm->vbo);
    r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)(md.vertex_count * sizeof(GFVertex)),
                     md.vertices, GF_GL_STATIC_DRAW);
    r->gl.BindBuffer(GF_GL_ELEMENT_ARRAY_BUFFER, gm->ebo);
    r->gl.BufferData(GF_GL_ELEMENT_ARRAY_BUFFER, (GFGLsizeiptr)(md.index_count * sizeof(uint32_t)),
                     md.indices, GF_GL_STATIC_DRAW);
    setup_mesh_attribs(&r->gl);
    r->gl.BindVertexArray(0);

    gf_mesh_data_free(&md);
    return gm;
}

bool gf_gl_renderer_reload_shaders(GFGlRenderer *r) {
    if (!r || !r->am) return false;
    GFGLuint pm = build_program_from_assets(r, "shaders/mesh.vert.glsl", "shaders/mesh.frag.glsl");
    GFGLuint pc = pm ? build_program_from_assets(r, "shaders/cable.vert.glsl", "shaders/cable.frag.glsl") : 0;
    GFGLuint ps = pc ? build_program_from_assets(r, "shaders/sprite.vert.glsl", "shaders/sprite.frag.glsl") : 0;
    GFGLuint ph = ps ? build_program_from_assets(r, "shaders/hud.vert.glsl", "shaders/hud.frag.glsl") : 0;
    if (!pm || !pc || !ps || !ph) {
        if (pm) r->gl.DeleteProgram(pm);
        if (pc) r->gl.DeleteProgram(pc);
        if (ps) r->gl.DeleteProgram(ps);
        if (ph) r->gl.DeleteProgram(ph);
        return false;
    }
    if (r->prog_mesh) r->gl.DeleteProgram(r->prog_mesh);
    if (r->prog_cable) r->gl.DeleteProgram(r->prog_cable);
    if (r->prog_sprite) r->gl.DeleteProgram(r->prog_sprite);
    if (r->prog_hud) r->gl.DeleteProgram(r->prog_hud);
    r->prog_mesh = pm;
    r->prog_cable = pc;
    r->prog_sprite = ps;
    r->prog_hud = ph;
    return true;
}

bool gf_gl_renderer_init(GFGlRenderer *r, const GFGlApi *api, const GFAssetManager *am, bool is_gles) {
    if (!r || !api || !am || !api->CreateShader) return false;
    memset(r, 0, sizeof(*r));
    r->gl = *api;
    r->am = am;
    r->is_gles = is_gles;

    if (!gf_gl_renderer_reload_shaders(r)) {
        return false;
    }

    r->dyn3d_cpu_buf = (GFVertex *)malloc((size_t)GF_DYN3D_MAX_VERTS * sizeof(GFVertex));
    r->cable_cpu_buf = (GFVertex *)malloc((size_t)GF_CABLE_MAX_VERTS * sizeof(GFVertex));
    if (!r->dyn3d_cpu_buf || !r->cable_cpu_buf) {
        free(r->dyn3d_cpu_buf);
        free(r->cable_cpu_buf);
        return false;
    }

    /* Billboard sprite unit quad */
    static const GFVertex quad[6] = {
        {-0.5f, -0.5f, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1},
        { 0.5f, -0.5f, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
        { 0.5f,  0.5f, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1},
        {-0.5f, -0.5f, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1},
        { 0.5f,  0.5f, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1},
        {-0.5f,  0.5f, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1},
    };
    r->gl.GenVertexArrays(1, &r->sprite_vao);
    r->gl.GenBuffers(1, &r->sprite_vbo);
    r->gl.BindVertexArray(r->sprite_vao);
    r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->sprite_vbo);
    r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)sizeof(quad), quad, GF_GL_STATIC_DRAW);
    setup_mesh_attribs(&r->gl);

    /* Dynamic audio ribbon VBO */
    r->gl.GenVertexArrays(1, &r->ribbon_vao);
    r->gl.GenBuffers(1, &r->ribbon_vbo);
    r->gl.BindVertexArray(r->ribbon_vao);
    r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->ribbon_vbo);
    r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)(512 * sizeof(GFVertex)), NULL, GF_GL_DYNAMIC_DRAW);
    setup_mesh_attribs(&r->gl);

    /* Dynamic 3D holographic curves & world-space text VBO */
    r->gl.GenVertexArrays(1, &r->dyn3d_vao);
    r->gl.GenBuffers(1, &r->dyn3d_vbo);
    r->gl.BindVertexArray(r->dyn3d_vao);
    r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->dyn3d_vbo);
    r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)(GF_DYN3D_MAX_VERTS * sizeof(GFVertex)), NULL, GF_GL_DYNAMIC_DRAW);
    setup_mesh_attribs(&r->gl);

    /* Dynamic batched catenary patch cables VBO */
    r->gl.GenVertexArrays(1, &r->cable_vao);
    r->gl.GenBuffers(1, &r->cable_vbo);
    r->gl.BindVertexArray(r->cable_vao);
    r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->cable_vbo);
    r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)(GF_CABLE_MAX_VERTS * sizeof(GFVertex)), NULL, GF_GL_DYNAMIC_DRAW);
    setup_mesh_attribs(&r->gl);

    /* Dynamic HUD VBO */
    r->gl.GenVertexArrays(1, &r->hud_vao);
    r->gl.GenBuffers(1, &r->hud_vbo);
    r->gl.BindVertexArray(r->hud_vao);
    r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->hud_vbo);
    r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)(GF_HUD_MAX_VERTS * sizeof(GFHudVertex)), NULL, GF_GL_DYNAMIC_DRAW);
    r->gl.EnableVertexAttribArray(0);
    r->gl.VertexAttribPointer(0, 2, GF_GL_FLOAT, 0, (GFGLsizei)sizeof(GFHudVertex), (const void *)offsetof(GFHudVertex, x));
    r->gl.EnableVertexAttribArray(1);
    r->gl.VertexAttribPointer(1, 2, GF_GL_FLOAT, 0, (GFGLsizei)sizeof(GFHudVertex), (const void *)offsetof(GFHudVertex, u));
    r->gl.EnableVertexAttribArray(2);
    r->gl.VertexAttribPointer(2, 4, GF_GL_FLOAT, 0, (GFGLsizei)sizeof(GFHudVertex), (const void *)offsetof(GFHudVertex, r));
    r->gl.BindVertexArray(0);

    GFGpuTex *ft = get_or_load_texture(r, "fonts/retro_font.tga");
    if (ft) r->font_tex = ft->tex;

    r->initialized = true;
    return true;
}

void gf_gl_renderer_destroy(GFGlRenderer *r) {
    if (!r || !r->initialized) return;
    for (int i = 0; i < r->mesh_count; ++i) {
        r->gl.DeleteVertexArrays(1, &r->meshes[i].vao);
        r->gl.DeleteBuffers(1, &r->meshes[i].vbo);
        r->gl.DeleteBuffers(1, &r->meshes[i].ebo);
    }
    for (int i = 0; i < r->tex_count; ++i) {
        r->gl.DeleteTextures(1, &r->textures[i].tex);
    }
    r->gl.DeleteVertexArrays(1, &r->sprite_vao);
    r->gl.DeleteBuffers(1, &r->sprite_vbo);
    r->gl.DeleteVertexArrays(1, &r->ribbon_vao);
    r->gl.DeleteBuffers(1, &r->ribbon_vbo);
    r->gl.DeleteVertexArrays(1, &r->dyn3d_vao);
    r->gl.DeleteBuffers(1, &r->dyn3d_vbo);
    r->gl.DeleteVertexArrays(1, &r->cable_vao);
    r->gl.DeleteBuffers(1, &r->cable_vbo);
    r->gl.DeleteVertexArrays(1, &r->hud_vao);
    r->gl.DeleteBuffers(1, &r->hud_vbo);
    r->gl.DeleteProgram(r->prog_mesh);
    r->gl.DeleteProgram(r->prog_cable);
    r->gl.DeleteProgram(r->prog_sprite);
    r->gl.DeleteProgram(r->prog_hud);
    free(r->dyn3d_cpu_buf);
    free(r->cable_cpu_buf);
    r->dyn3d_cpu_buf = NULL;
    r->cable_cpu_buf = NULL;
    r->initialized = false;
}

void gf_gl_renderer_render_view(GFGlRenderer *r, GFScene *sc, const GFAudioTelemetry *tel,
                                const GFMat4 *view, const GFMat4 *proj,
                                int vp_x, int vp_y, int vp_w, int vp_h, bool clear_bg) {
    if (!r || !r->initialized || !sc || !view || !proj || vp_w <= 0 || vp_h <= 0) return;

    r->last_draw_calls = 0;
    r->gl.Viewport(vp_x, vp_y, vp_w, vp_h);
    if (clear_bg) {
        const GFRoomDef *cur_rm = gf_sanctum_room_def(sc->camera.current_room);
        float glow = tel ? gf_clampf((tel->rms_l + tel->rms_r) * 0.16f, 0.0f, 0.12f) : 0.0f;
        float br = cur_rm ? cur_rm->theme_rgb.x * 0.055f : 0.03f;
        float bg = cur_rm ? cur_rm->theme_rgb.y * 0.065f : 0.06f;
        float bb = cur_rm ? cur_rm->theme_rgb.z * 0.085f : 0.08f;
        r->gl.ClearColor(br + glow * 0.3f, bg + glow, bb + glow * 0.6f, 1.0f);
        r->gl.Clear(GF_GL_COLOR_BUFFER_BIT | GF_GL_DEPTH_BUFFER_BIT);
    }

    /* Evaluate portal & frustum room visibility across all 7 rooms */
    float aspect = (float)vp_w / (float)(vp_h > 0 ? vp_h : 1);
    bool room_vis[8] = {true, true, true, true, true, true, true, true};
    int vis_cnt = 0;
    for (int rm = 1; rm <= GF_SANCTUM_ROOM_COUNT; ++rm) {
        room_vis[rm] = sc->use_camera_rig
            ? gf_camera_is_room_visible(&sc->camera, view, sc->cam_fov_deg, aspect, sc->cam_far, rm)
            : true;
        if (room_vis[rm]) vis_cnt++;
    }
    r->last_visible_rooms = vis_cnt;

    r->gl.Enable(GF_GL_DEPTH_TEST);
    r->gl.Enable(GF_GL_BLEND);
    r->gl.BlendFunc(GF_GL_SRC_ALPHA, GF_GL_ONE_MINUS_SRC_ALPHA);

    /* 1. Retro 3D architectural meshes, voxels, 3D synth racks & 3D hardware controls */
    r->gl.UseProgram(r->prog_mesh);
    r->gl.UniformMatrix4fv(r->gl.GetUniformLocation(r->prog_mesh, "u_view"), 1, 0, view->m);
    r->gl.UniformMatrix4fv(r->gl.GetUniformLocation(r->prog_mesh, "u_proj"), 1, 0, proj->m);
    r->gl.Uniform3f(r->gl.GetUniformLocation(r->prog_mesh, "u_light_dir"), sc->light_dir.x, sc->light_dir.y, sc->light_dir.z);
    r->gl.Uniform1f(r->gl.GetUniformLocation(r->prog_mesh, "u_snap_res"), sc->snap_res);
    r->gl.Uniform1f(r->gl.GetUniformLocation(r->prog_mesh, "u_color_steps"), sc->color_steps);
    r->gl.Uniform1f(r->gl.GetUniformLocation(r->prog_mesh, "u_dither"), sc->dither_enabled ? 1.0f : 0.0f);
    r->gl.Uniform1f(r->gl.GetUniformLocation(r->prog_mesh, "u_audio_pulse"), tel ? (tel->rms_l + tel->rms_r) * 0.5f : 0.0f);

    GFGLint loc_model   = r->gl.GetUniformLocation(r->prog_mesh, "u_model");
    GFGLint loc_tint    = r->gl.GetUniformLocation(r->prog_mesh, "u_tint");
    GFGLint loc_use_tex = r->gl.GetUniformLocation(r->prog_mesh, "u_use_tex");
    GFGLint loc_unlit   = r->gl.GetUniformLocation(r->prog_mesh, "u_unlit");
    GFGLint loc_tex     = r->gl.GetUniformLocation(r->prog_mesh, "u_tex");
    r->gl.Uniform1f(loc_use_tex, 0.0f);
    r->gl.Uniform1f(loc_unlit, 0.0f);
    r->gl.Uniform1i(loc_tex, 0);

    /* 1a. Scene entities (room shells, archway conduits, pedestals, voxels) */
    for (int i = 0; i < GF_MAX_SCENE_ENTITIES; ++i) {
        const GFSceneEntity *e = &sc->entities[i];
        if (!e->active || !e->visible) continue;
        if (e->room_id >= 1 && e->room_id <= 7 && !room_vis[e->room_id]) continue;
        if (e->kind == GF_ENT_VOXEL && !sc->voxels_enabled) continue;
        if (e->kind != GF_ENT_MESH && e->kind != GF_ENT_VOXEL) continue;

        GFGpuMesh *gm = get_or_load_mesh(r, e->asset_path, e->kind == GF_ENT_VOXEL);
        if (!gm) continue;
        GFMat4 model = gf_mat4_trs(e->pos, e->rot, e->scale);
        r->gl.UniformMatrix4fv(loc_model, 1, 0, model.m);
        r->gl.Uniform4f(loc_tint, e->tint.x, e->tint.y, e->tint.z, e->tint.w);
        r->gl.BindVertexArray(gm->vao);
        r->gl.DrawElements(GF_GL_TRIANGLES, (GFGLsizei)gm->index_count, GF_GL_UNSIGNED_INT, NULL);
        r->last_draw_calls++;
    }

    /* 1b. 3D Synth Rack Cabinets */
    if (sc->spatial.rack_count > 0) {
        GFGpuMesh *rack_gm = get_or_load_mesh(r, "models/rack_cabinet.obj", false);
        if (rack_gm) {
            r->gl.BindVertexArray(rack_gm->vao);
            for (int i = 0; i < sc->spatial.rack_count; ++i) {
                const GFRack3D *rk = &sc->spatial.racks[i];
                if (!rk->active) continue;
                if (rk->room_id >= 1 && rk->room_id <= 7 && !room_vis[rk->room_id]) continue;
                GFMat4 m = gf_rack_model_matrix(rk);
                r->gl.UniformMatrix4fv(loc_model, 1, 0, m.m);
                r->gl.Uniform4f(loc_tint, rk->theme_color.x, rk->theme_color.y, rk->theme_color.z, rk->theme_color.w);
                r->gl.DrawElements(GF_GL_TRIANGLES, (GFGLsizei)rack_gm->index_count, GF_GL_UNSIGNED_INT, NULL);
                r->last_draw_calls++;
            }
        }
    }

    /* 1c. 3D Hardware Controls (Knobs, Faders, Switches, Step Dials, Jacks, Keys, Env Nodes, Harmonic Bars, XYZ Orb) */
    for (int i = 0; i < sc->spatial.widget_count; ++i) {
        const GFWidget3D *w = &sc->spatial.widgets[i];
        if (!w->active || !w->visible) continue;
        if (w->room_id >= 1 && w->room_id <= 7 && !room_vis[w->room_id]) continue;
        const char *mpath = gf_spatial_widget_mesh_path(w->kind);
        if (!mpath) continue;
        GFGpuMesh *gm = get_or_load_mesh(r, mpath, false);
        if (!gm) continue;

        GFMat4 m = gf_spatial_widget_model_matrix(&sc->spatial, w);
        GFVec4 c = w->color;
        if (w->id == sc->spatial.active_widget_id || w->id == sc->spatial.hovered_widget_id ||
            (sc->cable_drag.active && w->port_id >= 0 && w->port_id == sc->cable_drag.hovered_port)) {
            c.x = gf_clampf(c.x * 1.35f + 0.25f, 0.0f, 1.0f);
            c.y = gf_clampf(c.y * 1.35f + 0.25f, 0.0f, 1.0f);
            c.z = gf_clampf(c.z * 1.35f + 0.20f, 0.0f, 1.0f);
        }
        r->gl.UniformMatrix4fv(loc_model, 1, 0, m.m);
        r->gl.Uniform4f(loc_tint, c.x, c.y, c.z, c.w);
        r->gl.BindVertexArray(gm->vao);
        r->gl.DrawElements(GF_GL_TRIANGLES, (GFGLsizei)gm->index_count, GF_GL_UNSIGNED_INT, NULL);
        r->last_draw_calls++;
    }

    /* 2. Audio-reactive 3D oscilloscope ribbon anchored in active room crown */
    if (sc->ribbon_enabled && tel) {
        GFVertex rverts[512];
        int rv_cnt = gf_scene_build_ribbon(tel, rverts, 512);
        if (rv_cnt > 0) {
            const GFRoomDef *cur_rm = sc->use_camera_rig ? gf_sanctum_room_def(sc->camera.current_room) : NULL;
            GFMat4 rm_mat = cur_rm ? gf_mat4_translate(cur_rm->center) : gf_mat4_identity();
            r->gl.UniformMatrix4fv(loc_model, 1, 0, rm_mat.m);
            r->gl.Uniform4f(loc_tint, 0.45f, 1.0f, 0.72f, 0.9f);
            r->gl.BindVertexArray(r->ribbon_vao);
            r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->ribbon_vbo);
            r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)((size_t)rv_cnt * sizeof(GFVertex)),
                             rverts, GF_GL_DYNAMIC_DRAW);
            r->gl.DrawArrays(GF_GL_TRIANGLES, 0, (GFGLsizei)rv_cnt);
            r->last_draw_calls++;
        }
    }

    /* 3. 3D Holographic Curves & Bar Sculptures + WebXR Laser Ray */
    GFMat4 id_mat = gf_mat4_identity();
    int curve_verts = gf_spatial_build_curve_verts(&sc->spatial, room_vis, r->dyn3d_cpu_buf, GF_DYN3D_MAX_VERTS - 12);
    if (sc->xr_laser_active && curve_verts + 6 <= GF_DYN3D_MAX_VERTS) {
        GFVec3 d = gf_vec3_sub(sc->xr_laser_end, sc->xr_laser_origin);
        GFVec3 side = gf_vec3_normalize(gf_vec3_cross(d, gf_vec3(0, 1, 0)));
        if (gf_vec3_len(side) < 1e-4f) side = gf_vec3(0.01f, 0, 0);
        side = gf_vec3_scale(side, 0.008f);
        GFVec3 p0 = gf_vec3_sub(sc->xr_laser_origin, side);
        GFVec3 p1 = gf_vec3_add(sc->xr_laser_origin, side);
        GFVec3 p2 = gf_vec3_add(sc->xr_laser_end, side);
        GFVec3 p3 = gf_vec3_sub(sc->xr_laser_end, side);
        GFVertex *lv = &r->dyn3d_cpu_buf[curve_verts];
        lv[0] = (GFVertex){p0.x, p0.y, p0.z, 0, 1, 0, 0, 0, 0.35f, 1.0f, 0.85f, 0.95f};
        lv[1] = (GFVertex){p1.x, p1.y, p1.z, 0, 1, 0, 1, 0, 0.35f, 1.0f, 0.85f, 0.95f};
        lv[2] = (GFVertex){p2.x, p2.y, p2.z, 0, 1, 0, 1, 1, 0.95f, 1.0f, 0.55f, 0.95f};
        lv[3] = (GFVertex){p0.x, p0.y, p0.z, 0, 1, 0, 0, 0, 0.35f, 1.0f, 0.85f, 0.95f};
        lv[4] = (GFVertex){p2.x, p2.y, p2.z, 0, 1, 0, 1, 1, 0.95f, 1.0f, 0.55f, 0.95f};
        lv[5] = (GFVertex){p3.x, p3.y, p3.z, 0, 1, 0, 0, 1, 0.95f, 1.0f, 0.55f, 0.95f};
        curve_verts += 6;
    }
    if (curve_verts > 0) {
        r->gl.UniformMatrix4fv(loc_model, 1, 0, id_mat.m);
        r->gl.Uniform4f(loc_tint, 1.0f, 1.0f, 1.0f, 1.0f);
        r->gl.Uniform1f(loc_unlit, 1.0f);
        r->gl.Uniform1f(loc_use_tex, 0.0f);
        r->gl.BindVertexArray(r->dyn3d_vao);
        r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->dyn3d_vbo);
        r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)((size_t)curve_verts * sizeof(GFVertex)),
                         r->dyn3d_cpu_buf, GF_GL_DYNAMIC_DRAW);
        r->gl.DrawArrays(GF_GL_TRIANGLES, 0, (GFGLsizei)curve_verts);
        r->last_draw_calls++;
    }

    /* 4. 3D Diegetic World-Space Text Plaques & Tooltips */
    if (sc->has_world_font && r->font_tex != 0) {
        int txt_verts = gf_spatial_build_world_text_verts(&sc->spatial, &sc->world_font, room_vis,
                                                          r->dyn3d_cpu_buf, GF_DYN3D_MAX_VERTS);
        if (txt_verts > 0) {
            r->gl.ActiveTexture(GF_GL_TEXTURE0);
            r->gl.BindTexture(GF_GL_TEXTURE_2D, r->font_tex);
            r->gl.UniformMatrix4fv(loc_model, 1, 0, id_mat.m);
            r->gl.Uniform4f(loc_tint, 1.0f, 1.0f, 1.0f, 1.0f);
            r->gl.Uniform1f(loc_unlit, 1.0f);
            r->gl.Uniform1f(loc_use_tex, 1.0f);
            r->gl.BindVertexArray(r->dyn3d_vao);
            r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->dyn3d_vbo);
            r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)((size_t)txt_verts * sizeof(GFVertex)),
                             r->dyn3d_cpu_buf, GF_GL_DYNAMIC_DRAW);
            r->gl.DrawArrays(GF_GL_TRIANGLES, 0, (GFGLsizei)txt_verts);
            r->last_draw_calls++;
        }
    }
    r->gl.Uniform1f(loc_use_tex, 0.0f);
    r->gl.Uniform1f(loc_unlit, 0.0f);

    /* 5. Physical 3D Catenary Patch Cables, Overhead Conduits & Mid-Cable Attenuverter Rings */
    int cable_verts = gf_cable_build_verts(&sc->cable_batch, sc->active_plan, tel, &sc->spatial,
                                           &sc->cable_drag, sc->camera.current_room, room_vis,
                                           r->cable_cpu_buf, GF_CABLE_MAX_VERTS);
    if (cable_verts > 0) {
        r->gl.UseProgram(r->prog_cable);
        r->gl.UniformMatrix4fv(r->gl.GetUniformLocation(r->prog_cable, "u_view"), 1, 0, view->m);
        r->gl.UniformMatrix4fv(r->gl.GetUniformLocation(r->prog_cable, "u_proj"), 1, 0, proj->m);
        r->gl.Uniform3f(r->gl.GetUniformLocation(r->prog_cable, "u_light_dir"), sc->light_dir.x, sc->light_dir.y, sc->light_dir.z);
        r->gl.Uniform1f(r->gl.GetUniformLocation(r->prog_cable, "u_snap_res"), sc->snap_res);
        r->gl.Uniform1f(r->gl.GetUniformLocation(r->prog_cable, "u_color_steps"), sc->color_steps);
        r->gl.Uniform1f(r->gl.GetUniformLocation(r->prog_cable, "u_time"), sc->anim_time);

        r->gl.BindVertexArray(r->cable_vao);
        r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->cable_vbo);
        r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)((size_t)cable_verts * sizeof(GFVertex)),
                         r->cable_cpu_buf, GF_GL_DYNAMIC_DRAW);
        r->gl.DrawArrays(GF_GL_TRIANGLES, 0, (GFGLsizei)cable_verts);
        r->last_draw_calls++;

        if (sc->cable_batch.ring_count > 0) {
            GFGpuMesh *ring_gm = get_or_load_mesh(r, "models/attenuverter_ring.obj", false);
            if (ring_gm) {
                r->gl.UseProgram(r->prog_mesh);
                r->gl.BindVertexArray(ring_gm->vao);
                for (int i = 0; i < sc->cable_batch.ring_count; ++i) {
                    const GFCableRing3D *rg = &sc->cable_batch.rings[i];
                    GFMat4 rm = gf_cable_ring_model_matrix(rg);
                    r->gl.UniformMatrix4fv(loc_model, 1, 0, rm.m);
                    r->gl.Uniform4f(loc_tint, rg->color.x, rg->color.y, rg->color.z, 1.0f);
                    r->gl.DrawElements(GF_GL_TRIANGLES, (GFGLsizei)ring_gm->index_count, GF_GL_UNSIGNED_INT, NULL);
                    r->last_draw_calls++;
                }
            }
        }
    }

    /* 6. Camera-facing billboarded 3D pixel sprites */
    r->gl.UseProgram(r->prog_sprite);
    r->gl.UniformMatrix4fv(r->gl.GetUniformLocation(r->prog_sprite, "u_view"), 1, 0, view->m);
    r->gl.UniformMatrix4fv(r->gl.GetUniformLocation(r->prog_sprite, "u_proj"), 1, 0, proj->m);
    r->gl.Uniform1i(r->gl.GetUniformLocation(r->prog_sprite, "u_tex"), 0);
    r->gl.ActiveTexture(GF_GL_TEXTURE0);
    r->gl.BindVertexArray(r->sprite_vao);

    GFGLint loc_wpos = r->gl.GetUniformLocation(r->prog_sprite, "u_world_pos");
    GFGLint loc_size = r->gl.GetUniformLocation(r->prog_sprite, "u_size");
    GFGLint loc_uvr  = r->gl.GetUniformLocation(r->prog_sprite, "u_uv_rect");
    GFGLint loc_st   = r->gl.GetUniformLocation(r->prog_sprite, "u_tint");

    for (int i = 0; i < GF_MAX_SCENE_ENTITIES; ++i) {
        const GFSceneEntity *e = &sc->entities[i];
        if (!e->active || !e->visible || e->kind != GF_ENT_SPRITE) continue;
        if (e->room_id >= 1 && e->room_id <= 7 && !room_vis[e->room_id]) continue;
        GFGpuTex *gt = get_or_load_texture(r, e->asset_path);
        if (!gt) continue;
        int frames = e->sprite_frames > 0 ? e->sprite_frames : 1;
        float du = 1.0f / (float)frames;
        float u0 = (float)(e->sprite_frame % frames) * du;
        r->gl.BindTexture(GF_GL_TEXTURE_2D, gt->tex);
        r->gl.Uniform3f(loc_wpos, e->pos.x, e->pos.y, e->pos.z);
        r->gl.Uniform2f(loc_size, e->sprite_size.x * e->scale.x, e->sprite_size.y * e->scale.y);
        r->gl.Uniform4f(loc_uvr, u0, 0.0f, du, 1.0f);
        r->gl.Uniform4f(loc_st, e->tint.x, e->tint.y, e->tint.z, e->tint.w);
        r->gl.DrawArrays(GF_GL_TRIANGLES, 0, 6);
        r->last_draw_calls++;
    }
    r->gl.BindVertexArray(0);
}

void gf_gl_renderer_render_hud(GFGlRenderer *r, const GFHudBatch *hb, int vp_w, int vp_h) {
    if (!r || !r->initialized || !hb || hb->vert_count == 0 || vp_w <= 0 || vp_h <= 0) return;

    r->gl.Disable(GF_GL_DEPTH_TEST);
    r->gl.Enable(GF_GL_BLEND);
    r->gl.BlendFunc(GF_GL_SRC_ALPHA, GF_GL_ONE_MINUS_SRC_ALPHA);

    GFMat4 ortho = gf_mat4_ortho(0.0f, (float)vp_w, (float)vp_h, 0.0f, -1.0f, 1.0f);
    r->gl.UseProgram(r->prog_hud);
    r->gl.UniformMatrix4fv(r->gl.GetUniformLocation(r->prog_hud, "u_ortho"), 1, 0, ortho.m);
    r->gl.Uniform1i(r->gl.GetUniformLocation(r->prog_hud, "u_tex"), 0);
    GFGLint loc_use_tex = r->gl.GetUniformLocation(r->prog_hud, "u_use_tex");

    r->gl.BindVertexArray(r->hud_vao);
    r->gl.BindBuffer(GF_GL_ARRAY_BUFFER, r->hud_vbo);
    r->gl.BufferData(GF_GL_ARRAY_BUFFER, (GFGLsizeiptr)(hb->vert_count * sizeof(GFHudVertex)),
                     hb->verts, GF_GL_DYNAMIC_DRAW);

    r->gl.ActiveTexture(GF_GL_TEXTURE0);
    r->gl.BindTexture(GF_GL_TEXTURE_2D, r->font_tex);

    for (uint32_t i = 0; i < hb->cmd_count; ++i) {
        const GFHudCmd *c = &hb->cmds[i];
        r->gl.Uniform1f(loc_use_tex, (c->textured && r->font_tex != 0) ? 1.0f : 0.0f);
        r->gl.DrawArrays(GF_GL_TRIANGLES, (GFGLint)c->first_vert, (GFGLsizei)c->vert_count);
        r->last_draw_calls++;
    }
    r->gl.BindVertexArray(0);
}

void gf_gl_renderer_render_xr_eye(GFGlRenderer *r, GFScene *sc, const GFHudBatch *hb,
                                  const GFAudioTelemetry *tel, const GFMat4 *view, const GFMat4 *proj,
                                  int vp_x, int vp_y, int vp_w, int vp_h) {
    if (!r || !r->initialized || !sc || !view || !proj) return;
    GFMat4 world_xr_view = sc->use_camera_rig ? gf_camera_apply_xr_view(&sc->camera, *view) : *view;
    gf_gl_renderer_render_view(r, sc, tel, &world_xr_view, proj, vp_x, vp_y, vp_w, vp_h, false);
    if (hb && hb->vert_count > 0) {
        r->gl.Viewport(vp_x, vp_y, vp_w, vp_h);
        gf_gl_renderer_render_hud(r, hb, vp_w, vp_h);
    }
}

bool gf_gl_renderer_read_rgba(const GFGlRenderer *r, int w, int h, uint8_t *out_rgba) {
    if (!r || !r->initialized || !out_rgba || w <= 0 || h <= 0 || !r->gl.ReadPixels) return false;
    r->gl.ReadPixels(0, 0, w, h, GF_GL_RGBA, GF_GL_UNSIGNED_BYTE, out_rgba);
    return true;
}
