#include "render/voxel_mesher.h"

#include <stdlib.h>
#include <string.h>

static uint8_t cell_at(const GFVoxelGrid *g, int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= g->sx || y >= g->sy || z >= g->sz) {
        return 0;
    }
    return g->cells[(size_t)(z * g->sy + y) * (size_t)g->sx + (size_t)x];
}

typedef struct FaceDef {
    int dx, dy, dz;
    float nx, ny, nz;
    float corners[4][3];
} FaceDef;

static const FaceDef g_faces[6] = {
    { 1, 0, 0,  1, 0, 0, {{1,0,0},{1,1,0},{1,1,1},{1,0,1}}},
    {-1, 0, 0, -1, 0, 0, {{0,0,1},{0,1,1},{0,1,0},{0,0,0}}},
    { 0, 1, 0,  0, 1, 0, {{0,1,1},{1,1,1},{1,1,0},{0,1,0}}},
    { 0,-1, 0,  0,-1, 0, {{0,0,0},{1,0,0},{1,0,1},{0,0,1}}},
    { 0, 0, 1,  0, 0, 1, {{0,0,1},{1,0,1},{1,1,1},{0,1,1}}},
    { 0, 0,-1,  0, 0,-1, {{1,0,0},{0,0,0},{0,1,0},{1,1,0}}},
};

bool gf_voxel_build_mesh(const GFVoxelGrid *grid, float voxel_size, GFMeshData *out_mesh) {
    if (!grid || !grid->cells || !out_mesh || voxel_size <= 0.0f) return false;
    memset(out_mesh, 0, sizeof(*out_mesh));

    uint32_t v_cap = 256, v_cnt = 0;
    uint32_t i_cap = 384, i_cnt = 0;
    GFVertex *verts = (GFVertex *)malloc(v_cap * sizeof(GFVertex));
    uint32_t *idxs = (uint32_t *)malloc(i_cap * sizeof(uint32_t));
    if (!verts || !idxs) {
        free(verts); free(idxs);
        return false;
    }

    float ox = -0.5f * (float)grid->sx * voxel_size;
    float oy = -0.5f * (float)grid->sy * voxel_size;
    float oz = -0.5f * (float)grid->sz * voxel_size;

    for (int z = 0; z < grid->sz; ++z) {
        for (int y = 0; y < grid->sy; ++y) {
            for (int x = 0; x < grid->sx; ++x) {
                uint8_t pal = cell_at(grid, x, y, z);
                if (pal == 0) continue;

                float r = (float)grid->palette[pal][0] / 255.0f;
                float g = (float)grid->palette[pal][1] / 255.0f;
                float b = (float)grid->palette[pal][2] / 255.0f;
                float a = (float)grid->palette[pal][3] / 255.0f;
                if (a <= 0.0f) a = 1.0f;

                for (int f = 0; f < 6; ++f) {
                    const FaceDef *fd = &g_faces[f];
                    if (cell_at(grid, x + fd->dx, y + fd->dy, z + fd->dz) != 0) {
                        continue; /* Cull internal hidden face */
                    }

                    if (v_cnt + 4 > v_cap) {
                        v_cap *= 2;
                        verts = (GFVertex *)realloc(verts, v_cap * sizeof(GFVertex));
                    }
                    if (i_cnt + 6 > i_cap) {
                        i_cap *= 2;
                        idxs = (uint32_t *)realloc(idxs, i_cap * sizeof(uint32_t));
                    }

                    uint32_t base = v_cnt;
                    static const float uvs[4][2] = {{0,0},{1,0},{1,1},{0,1}};
                    for (int c = 0; c < 4; ++c) {
                        GFVertex v;
                        v.px = ox + ((float)x + fd->corners[c][0]) * voxel_size;
                        v.py = oy + ((float)y + fd->corners[c][1]) * voxel_size;
                        v.pz = oz + ((float)z + fd->corners[c][2]) * voxel_size;
                        v.nx = fd->nx; v.ny = fd->ny; v.nz = fd->nz;
                        v.u = uvs[c][0]; v.v = uvs[c][1];
                        v.r = r; v.g = g; v.b = b; v.a = a;
                        verts[v_cnt++] = v;
                    }
                    idxs[i_cnt++] = base + 0;
                    idxs[i_cnt++] = base + 1;
                    idxs[i_cnt++] = base + 2;
                    idxs[i_cnt++] = base + 0;
                    idxs[i_cnt++] = base + 2;
                    idxs[i_cnt++] = base + 3;
                }
            }
        }
    }

    if (i_cnt == 0) {
        free(verts); free(idxs);
        return false;
    }

    out_mesh->vertices = verts;
    out_mesh->vertex_count = v_cnt;
    out_mesh->indices = idxs;
    out_mesh->index_count = i_cnt;
    return true;
}
