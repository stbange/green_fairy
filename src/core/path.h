#ifndef GF_CORE_PATH_H
#define GF_CORE_PATH_H

#include <stdbool.h>
#include <stddef.h>

/* Normalizes a relative asset path to forward slashes and rejects absolute paths or '..' traversal. */
bool gf_path_normalize_rel(const char *rel_path, char *out_buf, size_t out_cap);

/* Joins a base directory and a validated relative path into out_buf. */
bool gf_path_join_safe(const char *root_dir, const char *rel_path, char *out_buf, size_t out_cap);

/* Extracts parent directory of a file path into out_buf. */
bool gf_path_dirname(const char *file_path, char *out_buf, size_t out_cap);

#endif
