#ifndef GF_ASSETS_LOADERS_H
#define GF_ASSETS_LOADERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct GFVertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
    float r, g, b, a;
} GFVertex;

typedef struct GFMeshData {
    GFVertex *vertices;
    uint32_t vertex_count;
    uint32_t *indices;
    uint32_t index_count;
} GFMeshData;

typedef struct GFImageData {
    int width;
    int height;
    uint8_t *rgba; /* width * height * 4 */
} GFImageData;

typedef struct GFVoxelGrid {
    int sx, sy, sz;
    uint8_t *cells;          /* sx * sy * sz, 0 = empty, 1..255 = palette index */
    uint8_t palette[256][4]; /* RGBA8 per index */
} GFVoxelGrid;

typedef struct GFAudioSample {
    float *samples;       /* Interleaved or mono normalized float [-1, 1] */
    uint32_t frame_count;
    uint32_t sample_rate;
    uint16_t channels;
} GFAudioSample;

typedef struct GFFontMetrics {
    int cell_w;
    int cell_h;
    int cols;
    int rows;
    int first_char;
    int char_count;
    char atlas_path[128];
} GFFontMetrics;

bool gf_load_obj_memory(const char *text, size_t len, GFMeshData *out_mesh);
void gf_mesh_data_free(GFMeshData *mesh);

bool gf_load_image_memory(const uint8_t *data, size_t size, GFImageData *out_img);
void gf_image_data_free(GFImageData *img);

bool gf_load_voxel_memory(const char *text, size_t len, GFVoxelGrid *out_grid);
void gf_voxel_grid_free(GFVoxelGrid *grid);

bool gf_load_wav_memory(const uint8_t *data, size_t size, GFAudioSample *out_sample);
void gf_audio_sample_free(GFAudioSample *sample);

bool gf_shader_preprocess(const char *src, bool is_gles, char **out_src);

bool gf_load_font_metrics_memory(const char *text, size_t len, GFFontMetrics *out_font);

#endif
