#ifndef GF_ASSETS_ASSET_MANAGER_H
#define GF_ASSETS_ASSET_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GF_ASSET_PATH_MAX 512
#define GF_ASSET_NAME_MAX 128

typedef struct GFAssetManager {
    char root_dir[GF_ASSET_PATH_MAX];
    bool initialized;
} GFAssetManager;

/* Resolves external asset root: cli_override > GREEN_FAIRY_ASSETS > <exe_dir>/assets > ./assets */
bool gf_assets_init(GFAssetManager *am, const char *cli_override, const char *exe_path);
const char *gf_assets_root(const GFAssetManager *am);

/* Safely resolves relative path inside asset root (rejects '..' or absolute paths). */
bool gf_assets_resolve_path(const GFAssetManager *am, const char *rel_path, char *out_buf, size_t out_cap);

/* Reads an entire file from the asset root into a NUL-terminated malloc'd buffer. */
bool gf_assets_read_file(const GFAssetManager *am, const char *rel_path, uint8_t **out_data, size_t *out_size);

/* Lists non-dot entries inside root/rel_subdir sorted lexicographically. */
int gf_assets_list_dir(const GFAssetManager *am, const char *rel_subdir,
                       char out_names[][GF_ASSET_NAME_MAX], int max_names);

/* Retrieves file mtime in seconds (or 0/false if missing). */
bool gf_assets_file_mtime(const GFAssetManager *am, const char *rel_path, uint64_t *out_mtime);

#endif
