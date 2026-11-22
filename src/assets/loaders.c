#include "assets/loaders.h"

#include "core/log.h"
#include "core/math3d.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- Wavefront OBJ Loader ---------------- */

typedef struct ObjVec3 { float x, y, z; float r, g, b; bool has_col; } ObjVec3;
typedef struct ObjVec2 { float u, v; } ObjVec2;

static bool parse_obj_index(const char *tok, int *vi, int *ti, int *ni) {
    *vi = 0; *ti = 0; *ni = 0;
    const char *s1 = strchr(tok, '/');
    if (!s1) {
        *vi = atoi(tok);
        return *vi != 0;
    }
    *vi = atoi(tok);
    const char *s2 = strchr(s1 + 1, '/');
    if (!s2) {
        if (s1[1] != '\0') *ti = atoi(s1 + 1);
    } else {
        if (s2 > s1 + 1) *ti = atoi(s1 + 1);
        if (s2[1] != '\0') *ni = atoi(s2 + 1);
    }
    return *vi != 0;
}

static int resolve_obj_idx(int idx, int count) {
    if (idx > 0 && idx <= count) return idx - 1;
    if (idx < 0 && (-idx) <= count) return count + idx;
    return -1;
}

bool gf_load_obj_memory(const char *text, size_t len, GFMeshData *out_mesh) {
    if (!text || len == 0 || !out_mesh) return false;
    memset(out_mesh, 0, sizeof(*out_mesh));

    int v_cap = 64, v_cnt = 0;
    int n_cap = 64, n_cnt = 0;
    int t_cap = 64, t_cnt = 0;
    uint32_t out_v_cap = 128, out_v_cnt = 0;
    uint32_t out_i_cap = 128, out_i_cnt = 0;

    ObjVec3 *pos = (ObjVec3 *)malloc((size_t)v_cap * sizeof(ObjVec3));
    GFVec3 *nrm = (GFVec3 *)malloc((size_t)n_cap * sizeof(GFVec3));
    ObjVec2 *uvs = (ObjVec2 *)malloc((size_t)t_cap * sizeof(ObjVec2));
    GFVertex *verts = (GFVertex *)malloc((size_t)out_v_cap * sizeof(GFVertex));
    uint32_t *idxs = (uint32_t *)malloc((size_t)out_i_cap * sizeof(uint32_t));

    if (!pos || !nrm || !uvs || !verts || !idxs) {
        free(pos); free(nrm); free(uvs); free(verts); free(idxs);
        return false;
    }

    const char *p = text;
    const char *end = text + len;
    char line[512];

    while (p < end) {
        size_t l = 0;
        while (p < end && *p != '\n' && *p != '\r' && l + 1 < sizeof(line)) {
            line[l++] = *p++;
        }
        while (p < end && (*p == '\n' || *p == '\r')) p++;
        line[l] = '\0';

        char *s = line;
        while (*s == ' ' || *s == '\t') s++;
        if (*s == '\0' || *s == '#') continue;

        if (s[0] == 'v' && (s[1] == ' ' || s[1] == '\t')) {
            ObjVec3 v = {0, 0, 0, 1, 1, 1, false};
            int nread = sscanf(s + 2, "%f %f %f %f %f %f", &v.x, &v.y, &v.z, &v.r, &v.g, &v.b);
            if (nread >= 3) {
                v.has_col = (nread == 6);
                if (v_cnt == v_cap) {
                    v_cap *= 2;
                    pos = (ObjVec3 *)realloc(pos, (size_t)v_cap * sizeof(ObjVec3));
                }
                pos[v_cnt++] = v;
            }
        } else if (s[0] == 'v' && s[1] == 'n' && (s[2] == ' ' || s[2] == '\t')) {
            GFVec3 n = {0, 1, 0};
            if (sscanf(s + 3, "%f %f %f", &n.x, &n.y, &n.z) == 3) {
                if (n_cnt == n_cap) {
                    n_cap *= 2;
                    nrm = (GFVec3 *)realloc(nrm, (size_t)n_cap * sizeof(GFVec3));
                }
                nrm[n_cnt++] = gf_vec3_normalize(n);
            }
        } else if (s[0] == 'v' && s[1] == 't' && (s[2] == ' ' || s[2] == '\t')) {
            ObjVec2 uv = {0, 0};
            if (sscanf(s + 3, "%f %f", &uv.u, &uv.v) == 2) {
                if (t_cnt == t_cap) {
                    t_cap *= 2;
                    uvs = (ObjVec2 *)realloc(uvs, (size_t)t_cap * sizeof(ObjVec2));
                }
                uvs[t_cnt++] = uv;
            }
        } else if (s[0] == 'f' && (s[1] == ' ' || s[1] == '\t')) {
            GFVertex face_v[16];
            int f_cnt = 0;
            bool missing_nrm = false;
            char *cur = s + 2;
            while (*cur && f_cnt < 16) {
                while (*cur == ' ' || *cur == '\t') cur++;
                if (*cur == '\0') break;
                char tok[64];
                int tl = 0;
                while (*cur && *cur != ' ' && *cur != '\t' && tl + 1 < (int)sizeof(tok)) {
                    tok[tl++] = *cur++;
                }
                tok[tl] = '\0';
                int vi = 0, ti = 0, ni = 0;
                if (!parse_obj_index(tok, &vi, &ti, &ni)) continue;
                int rvi = resolve_obj_idx(vi, v_cnt);
                if (rvi < 0) continue;
                int rti = resolve_obj_idx(ti, t_cnt);
                int rni = resolve_obj_idx(ni, n_cnt);

                GFVertex fv;
                memset(&fv, 0, sizeof(fv));
                fv.px = pos[rvi].x; fv.py = pos[rvi].y; fv.pz = pos[rvi].z;
                fv.r = pos[rvi].has_col ? pos[rvi].r : 1.0f;
                fv.g = pos[rvi].has_col ? pos[rvi].g : 1.0f;
                fv.b = pos[rvi].has_col ? pos[rvi].b : 1.0f;
                fv.a = 1.0f;
                if (rti >= 0) { fv.u = uvs[rti].u; fv.v = uvs[rti].v; }
                if (rni >= 0) {
                    fv.nx = nrm[rni].x; fv.ny = nrm[rni].y; fv.nz = nrm[rni].z;
                } else {
                    missing_nrm = true;
                }
                face_v[f_cnt++] = fv;
            }
            if (f_cnt >= 3) {
                for (int k = 1; k + 1 < f_cnt; ++k) {
                    GFVertex tri[3] = {face_v[0], face_v[k], face_v[k + 1]};
                    if (missing_nrm) {
                        GFVec3 e1 = gf_vec3(tri[1].px - tri[0].px, tri[1].py - tri[0].py, tri[1].pz - tri[0].pz);
                        GFVec3 e2 = gf_vec3(tri[2].px - tri[0].px, tri[2].py - tri[0].py, tri[2].pz - tri[0].pz);
                        GFVec3 fn = gf_vec3_normalize(gf_vec3_cross(e1, e2));
                        for (int j = 0; j < 3; ++j) {
                            tri[j].nx = fn.x; tri[j].ny = fn.y; tri[j].nz = fn.z;
                        }
                    }
                    if (out_v_cnt + 3 > out_v_cap) {
                        out_v_cap *= 2;
                        verts = (GFVertex *)realloc(verts, (size_t)out_v_cap * sizeof(GFVertex));
                    }
                    if (out_i_cnt + 3 > out_i_cap) {
                        out_i_cap *= 2;
                        idxs = (uint32_t *)realloc(idxs, (size_t)out_i_cap * sizeof(uint32_t));
                    }
                    for (int j = 0; j < 3; ++j) {
                        idxs[out_i_cnt++] = out_v_cnt;
                        verts[out_v_cnt++] = tri[j];
                    }
                }
            }
        }
    }

    free(pos); free(nrm); free(uvs);

    if (out_i_cnt == 0) {
        free(verts); free(idxs);
        GF_LOGW("assets", "OBJ contained no valid triangles");
        return false;
    }

    out_mesh->vertices = verts;
    out_mesh->vertex_count = out_v_cnt;
    out_mesh->indices = idxs;
    out_mesh->index_count = out_i_cnt;
    return true;
}

void gf_mesh_data_free(GFMeshData *mesh) {
    if (!mesh) return;
    free(mesh->vertices);
    free(mesh->indices);
    memset(mesh, 0, sizeof(*mesh));
}

/* ---------------- Image Loader (TGA 24/32-bit + PPM P6) ---------------- */

static bool load_ppm_p6(const uint8_t *data, size_t size, GFImageData *out_img) {
    size_t i = 2;
    int vals[3] = {0, 0, 0};
    int vi = 0;
    while (i < size && vi < 3) {
        while (i < size && isspace(data[i])) i++;
        if (i < size && data[i] == '#') {
            while (i < size && data[i] != '\n') i++;
            continue;
        }
        if (i >= size || !isdigit(data[i])) return false;
        int v = 0;
        while (i < size && isdigit(data[i])) {
            v = v * 10 + (data[i++] - '0');
        }
        vals[vi++] = v;
    }
    if (vi < 3 || vals[0] <= 0 || vals[1] <= 0 || vals[2] != 255) return false;
    if (i >= size || !isspace(data[i])) return false;
    i++;
    size_t px_count = (size_t)vals[0] * (size_t)vals[1];
    if (size - i < px_count * 3) return false;

    uint8_t *rgba = (uint8_t *)malloc(px_count * 4);
    if (!rgba) return false;
    for (size_t p = 0; p < px_count; ++p) {
        rgba[p * 4 + 0] = data[i + p * 3 + 0];
        rgba[p * 4 + 1] = data[i + p * 3 + 1];
        rgba[p * 4 + 2] = data[i + p * 3 + 2];
        rgba[p * 4 + 3] = 255;
    }
    out_img->width = vals[0];
    out_img->height = vals[1];
    out_img->rgba = rgba;
    return true;
}

static bool load_tga(const uint8_t *data, size_t size, GFImageData *out_img) {
    if (size < 18) return false;
    uint8_t id_len = data[0];
    uint8_t img_type = data[2];
    int width = (int)(data[12] | ((uint16_t)data[13] << 8));
    int height = (int)(data[14] | ((uint16_t)data[15] << 8));
    uint8_t bpp = data[16];
    uint8_t desc = data[17];

    if (img_type != 2 || width <= 0 || height <= 0 || (bpp != 24 && bpp != 32)) {
        return false;
    }
    size_t bytes_per_px = (size_t)(bpp / 8);
    size_t offset = 18u + id_len;
    size_t px_count = (size_t)width * (size_t)height;
    if (offset + px_count * bytes_per_px > size) {
        return false;
    }

    uint8_t *rgba = (uint8_t *)malloc(px_count * 4);
    if (!rgba) return false;

    bool top_origin = (desc & 0x20) != 0;
    for (int y = 0; y < height; ++y) {
        int dst_y = top_origin ? y : (height - 1 - y);
        for (int x = 0; x < width; ++x) {
            size_t src_i = offset + ((size_t)y * (size_t)width + (size_t)x) * bytes_per_px;
            size_t dst_i = ((size_t)dst_y * (size_t)width + (size_t)x) * 4;
            rgba[dst_i + 0] = data[src_i + 2]; /* R */
            rgba[dst_i + 1] = data[src_i + 1]; /* G */
            rgba[dst_i + 2] = data[src_i + 0]; /* B */
            rgba[dst_i + 3] = (bpp == 32) ? data[src_i + 3] : 255;
        }
    }
    out_img->width = width;
    out_img->height = height;
    out_img->rgba = rgba;
    return true;
}

bool gf_load_image_memory(const uint8_t *data, size_t size, GFImageData *out_img) {
    if (!data || size < 4 || !out_img) return false;
    memset(out_img, 0, sizeof(*out_img));
    if (data[0] == 'P' && data[1] == '6') {
        return load_ppm_p6(data, size, out_img);
    }
    return load_tga(data, size, out_img);
}

void gf_image_data_free(GFImageData *img) {
    if (!img) return;
    free(img->rgba);
    memset(img, 0, sizeof(*img));
}

/* ---------------- Optional Voxel Loader (.gfvox) ---------------- */

bool gf_load_voxel_memory(const char *text, size_t len, GFVoxelGrid *out_grid) {
    if (!text || len == 0 || !out_grid) return false;
    memset(out_grid, 0, sizeof(*out_grid));

    bool header_ok = false;
    const char *p = text;
    const char *end = text + len;
    char line[256];

    while (p < end) {
        size_t l = 0;
        while (p < end && *p != '\n' && *p != '\r' && l + 1 < sizeof(line)) {
            line[l++] = *p++;
        }
        while (p < end && (*p == '\n' || *p == '\r')) p++;
        line[l] = '\0';

        char *s = line;
        while (*s == ' ' || *s == '\t') s++;
        if (*s == '\0' || *s == '#') continue;

        if (!header_ok) {
            if (strncmp(s, "GFVOX", 5) == 0) {
                header_ok = true;
                continue;
            }
            return false;
        }

        int a = 0, b = 0, c = 0, d = 0, e = 0;
        if (sscanf(s, "SIZE %d %d %d", &a, &b, &c) == 3) {
            if (a <= 0 || b <= 0 || c <= 0 || a > 64 || b > 64 || c > 64 || out_grid->cells) {
                gf_voxel_grid_free(out_grid);
                return false;
            }
            out_grid->sx = a; out_grid->sy = b; out_grid->sz = c;
            out_grid->cells = (uint8_t *)calloc((size_t)a * (size_t)b * (size_t)c, 1);
            if (!out_grid->cells) return false;
        } else if (sscanf(s, "PALETTE %d %d %d %d %d", &a, &b, &c, &d, &e) == 5) {
            if (a >= 1 && a <= 255) {
                out_grid->palette[a][0] = (uint8_t)(b & 0xFF);
                out_grid->palette[a][1] = (uint8_t)(c & 0xFF);
                out_grid->palette[a][2] = (uint8_t)(d & 0xFF);
                out_grid->palette[a][3] = (uint8_t)(e & 0xFF);
            }
        } else if (sscanf(s, "VOX %d %d %d %d", &a, &b, &c, &d) == 4) {
            if (out_grid->cells &&
                a >= 0 && a < out_grid->sx &&
                b >= 0 && b < out_grid->sy &&
                c >= 0 && c < out_grid->sz &&
                d >= 1 && d <= 255) {
                size_t idx = (size_t)(c * out_grid->sy + b) * (size_t)out_grid->sx + (size_t)a;
                out_grid->cells[idx] = (uint8_t)d;
            }
        }
    }

    if (!header_ok || !out_grid->cells) {
        gf_voxel_grid_free(out_grid);
        return false;
    }
    return true;
}

void gf_voxel_grid_free(GFVoxelGrid *grid) {
    if (!grid) return;
    free(grid->cells);
    memset(grid, 0, sizeof(*grid));
}

/* ---------------- PCM WAV Audio Sample Loader ---------------- */

static uint16_t rd_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t rd_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool gf_load_wav_memory(const uint8_t *data, size_t size, GFAudioSample *out_sample) {
    if (!data || size < 44 || !out_sample) return false;
    memset(out_sample, 0, sizeof(*out_sample));

    if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "WAVE", 4) != 0) {
        return false;
    }

    uint16_t audio_fmt = 0, channels = 0, bits_per_sample = 0;
    uint32_t sample_rate = 0;
    const uint8_t *pcm = NULL;
    uint32_t pcm_bytes = 0;

    size_t pos = 12;
    while (pos + 8 <= size) {
        uint32_t chunk_sz = rd_u32(data + pos + 4);
        if (pos + 8 + chunk_sz > size) break;

        if (memcmp(data + pos, "fmt ", 4) == 0 && chunk_sz >= 16) {
            audio_fmt = rd_u16(data + pos + 8);
            channels = rd_u16(data + pos + 10);
            sample_rate = rd_u32(data + pos + 12);
            bits_per_sample = rd_u16(data + pos + 22);
        } else if (memcmp(data + pos, "data", 4) == 0) {
            pcm = data + pos + 8;
            pcm_bytes = chunk_sz;
        }
        pos += 8 + chunk_sz + (chunk_sz & 1u);
    }

    if (!pcm || pcm_bytes == 0 || sample_rate == 0 || (channels != 1 && channels != 2)) {
        return false;
    }

    if (audio_fmt == 1 && bits_per_sample == 16) {
        uint32_t total_samples = pcm_bytes / 2u;
        uint32_t frames = total_samples / channels;
        if (frames == 0) return false;
        float *buf = (float *)malloc((size_t)total_samples * sizeof(float));
        if (!buf) return false;
        for (uint32_t i = 0; i < total_samples; ++i) {
            int16_t s = (int16_t)rd_u16(pcm + (size_t)i * 2u);
            buf[i] = (float)s / 32768.0f;
        }
        out_sample->samples = buf;
        out_sample->frame_count = frames;
        out_sample->sample_rate = sample_rate;
        out_sample->channels = channels;
        return true;
    }

    if (audio_fmt == 3 && bits_per_sample == 32) {
        uint32_t total_samples = pcm_bytes / 4u;
        uint32_t frames = total_samples / channels;
        if (frames == 0) return false;
        float *buf = (float *)malloc((size_t)total_samples * sizeof(float));
        if (!buf) return false;
        memcpy(buf, pcm, (size_t)total_samples * sizeof(float));
        out_sample->samples = buf;
        out_sample->frame_count = frames;
        out_sample->sample_rate = sample_rate;
        out_sample->channels = channels;
        return true;
    }

    return false;
}

void gf_audio_sample_free(GFAudioSample *sample) {
    if (!sample) return;
    free(sample->samples);
    memset(sample, 0, sizeof(*sample));
}

/* ---------------- GLSL Cross-API Preprocessor ---------------- */

bool gf_shader_preprocess(const char *src, bool is_gles, char **out_src) {
    if (!src || !out_src) return false;
    *out_src = NULL;

    const char *body = src;
    while (*body == ' ' || *body == '\t' || *body == '\r' || *body == '\n') body++;
    if (strncmp(body, "#version", 8) == 0) {
        const char *nl = strchr(body, '\n');
        body = nl ? (nl + 1) : "";
    }

    const char *header = is_gles
        ? "#version 300 es\nprecision highp float;\n"
        : "#version 330 core\n";
    size_t hlen = strlen(header);
    size_t blen = strlen(body);
    char *res = (char *)malloc(hlen + blen + 1);
    if (!res) return false;
    memcpy(res, header, hlen);
    memcpy(res + hlen, body, blen);
    res[hlen + blen] = '\0';
    *out_src = res;
    return true;
}

/* ---------------- Bitmap Font Metrics Loader ---------------- */

bool gf_load_font_metrics_memory(const char *text, size_t len, GFFontMetrics *out_font) {
    if (!text || len == 0 || !out_font) return false;
    memset(out_font, 0, sizeof(*out_font));

    const char *p = text;
    const char *end = text + len;
    char line[256];
    while (p < end) {
        size_t l = 0;
        while (p < end && *p != '\n' && *p != '\r' && l + 1 < sizeof(line)) {
            line[l++] = *p++;
        }
        while (p < end && (*p == '\n' || *p == '\r')) p++;
        line[l] = '\0';

        char key[64], val[128];
        if (sscanf(line, "%63s %127s", key, val) == 2) {
            if (strcmp(key, "atlas") == 0) snprintf(out_font->atlas_path, sizeof(out_font->atlas_path), "%s", val);
            else if (strcmp(key, "cell_w") == 0) out_font->cell_w = atoi(val);
            else if (strcmp(key, "cell_h") == 0) out_font->cell_h = atoi(val);
            else if (strcmp(key, "cols") == 0) out_font->cols = atoi(val);
            else if (strcmp(key, "rows") == 0) out_font->rows = atoi(val);
            else if (strcmp(key, "first_char") == 0) out_font->first_char = atoi(val);
            else if (strcmp(key, "char_count") == 0) out_font->char_count = atoi(val);
        }
    }

    return out_font->cell_w > 0 && out_font->cell_h > 0 &&
           out_font->cols > 0 && out_font->rows > 0 &&
           out_font->char_count > 0 && out_font->atlas_path[0] != '\0';
}
