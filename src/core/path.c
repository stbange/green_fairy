#include "core/path.h"

#include <ctype.h>
#include <string.h>

bool gf_path_normalize_rel(const char *rel_path, char *out_buf, size_t out_cap) {
    if (!rel_path || !out_buf || out_cap < 2 || rel_path[0] == '\0') {
        return false;
    }

    /* Reject absolute Unix paths, Windows root paths, or drive prefixes (e.g. C:). */
    if (rel_path[0] == '/' || rel_path[0] == '\\') {
        return false;
    }
    if (isalpha((unsigned char)rel_path[0]) && rel_path[1] == ':') {
        return false;
    }

    size_t out_len = 0;
    const char *p = rel_path;
    while (*p != '\0') {
        while (*p == '/' || *p == '\\') {
            p++;
        }
        if (*p == '\0') break;

        const char *seg_start = p;
        while (*p != '\0' && *p != '/' && *p != '\\') {
            p++;
        }
        size_t seg_len = (size_t)(p - seg_start);

        if (seg_len == 1 && seg_start[0] == '.') {
            continue;
        }
        if (seg_len == 2 && seg_start[0] == '.' && seg_start[1] == '.') {
            return false;
        }

        size_t needed = seg_len + (out_len > 0 ? 1 : 0);
        if (out_len + needed + 1 > out_cap) {
            return false;
        }
        if (out_len > 0) {
            out_buf[out_len++] = '/';
        }
        memcpy(out_buf + out_len, seg_start, seg_len);
        out_len += seg_len;
    }

    if (out_len == 0) {
        return false;
    }
    out_buf[out_len] = '\0';
    return true;
}

bool gf_path_join_safe(const char *root_dir, const char *rel_path, char *out_buf, size_t out_cap) {
    if (!root_dir || root_dir[0] == '\0' || !out_buf || out_cap < 2) {
        return false;
    }

    char norm_rel[1024];
    if (!gf_path_normalize_rel(rel_path, norm_rel, sizeof(norm_rel))) {
        return false;
    }

    size_t root_len = strlen(root_dir);
    while (root_len > 1 && (root_dir[root_len - 1] == '/' || root_dir[root_len - 1] == '\\')) {
        root_len--;
    }

    size_t rel_len = strlen(norm_rel);
    bool root_is_slash = (root_len == 1 && (root_dir[0] == '/' || root_dir[0] == '\\'));
    size_t sep_len = root_is_slash ? 0 : 1;
    if (root_len + sep_len + rel_len + 1 > out_cap) {
        return false;
    }

    for (size_t i = 0; i < root_len; ++i) {
        out_buf[i] = (root_dir[i] == '\\') ? '/' : root_dir[i];
    }
    size_t pos = root_len;
    if (!root_is_slash) {
        out_buf[pos++] = '/';
    }
    memcpy(out_buf + pos, norm_rel, rel_len);
    out_buf[pos + rel_len] = '\0';
    return true;
}

bool gf_path_dirname(const char *file_path, char *out_buf, size_t out_cap) {
    if (!file_path || !out_buf || out_cap < 2 || file_path[0] == '\0') {
        return false;
    }

    size_t len = strlen(file_path);
    while (len > 1 && (file_path[len - 1] == '/' || file_path[len - 1] == '\\')) {
        len--;
    }

    const char *last_sep = NULL;
    for (size_t i = 0; i < len; ++i) {
        if (file_path[i] == '/' || file_path[i] == '\\') {
            last_sep = file_path + i;
        }
    }

    if (!last_sep) {
        out_buf[0] = '.';
        out_buf[1] = '\0';
        return true;
    }

    size_t dir_len = (size_t)(last_sep - file_path);
    if (dir_len == 0) {
        dir_len = 1;
    }
    if (dir_len + 1 > out_cap) {
        return false;
    }
    for (size_t i = 0; i < dir_len; ++i) {
        out_buf[i] = (file_path[i] == '\\') ? '/' : file_path[i];
    }
    out_buf[dir_len] = '\0';
    return true;
}
