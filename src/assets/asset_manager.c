#include "assets/asset_manager.h"

#include "core/log.h"
#include "core/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

static bool is_directory(const char *path) {
    if (!path || path[0] == '\0') return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
#ifdef _WIN32
    return (st.st_mode & _S_IFDIR) != 0;
#else
    return S_ISDIR(st.st_mode);
#endif
}

static void copy_root(GFAssetManager *am, const char *dir) {
    snprintf(am->root_dir, sizeof(am->root_dir), "%s", dir);
    size_t n = strlen(am->root_dir);
    for (size_t i = 0; i < n; ++i) {
        if (am->root_dir[i] == '\\') am->root_dir[i] = '/';
    }
    while (n > 1 && am->root_dir[n - 1] == '/') {
        am->root_dir[--n] = '\0';
    }
    am->initialized = true;
}

bool gf_assets_init(GFAssetManager *am, const char *cli_override, const char *exe_path) {
    if (!am) return false;
    memset(am, 0, sizeof(*am));

    if (cli_override && cli_override[0] != '\0') {
        if (!is_directory(cli_override)) {
            GF_LOGE("assets", "CLI assets directory not found: %s", cli_override);
            return false;
        }
        copy_root(am, cli_override);
        GF_LOGI("assets", "Using CLI asset root: %s", am->root_dir);
        return true;
    }

    const char *env_dir = getenv("GREEN_FAIRY_ASSETS");
    if (env_dir && env_dir[0] != '\0') {
        if (!is_directory(env_dir)) {
            GF_LOGE("assets", "GREEN_FAIRY_ASSETS directory not found: %s", env_dir);
            return false;
        }
        copy_root(am, env_dir);
        GF_LOGI("assets", "Using ENV asset root: %s", am->root_dir);
        return true;
    }

    if (exe_path && exe_path[0] != '\0') {
        char exe_dir[GF_ASSET_PATH_MAX];
        char sibling[GF_ASSET_PATH_MAX];
        if (gf_path_dirname(exe_path, exe_dir, sizeof(exe_dir)) &&
            gf_path_join_safe(exe_dir, "assets", sibling, sizeof(sibling)) &&
            is_directory(sibling)) {
            copy_root(am, sibling);
            GF_LOGI("assets", "Using sibling asset root: %s", am->root_dir);
            return true;
        }
    }

    if (is_directory("assets")) {
        copy_root(am, "assets");
        GF_LOGI("assets", "Using working-dir asset root: %s", am->root_dir);
        return true;
    }

    GF_LOGE("assets", "Failed to locate external assets directory");
    return false;
}

const char *gf_assets_root(const GFAssetManager *am) {
    return (am && am->initialized) ? am->root_dir : "";
}

bool gf_assets_resolve_path(const GFAssetManager *am, const char *rel_path, char *out_buf, size_t out_cap) {
    if (!am || !am->initialized) return false;
    if (!gf_path_join_safe(am->root_dir, rel_path, out_buf, out_cap)) {
        GF_LOGW("assets", "Rejected unsafe asset path: %s", rel_path ? rel_path : "(null)");
        return false;
    }
    return true;
}

bool gf_assets_read_file(const GFAssetManager *am, const char *rel_path, uint8_t **out_data, size_t *out_size) {
    if (!out_data || !out_size) return false;
    *out_data = NULL;
    *out_size = 0;

    char full_path[GF_ASSET_PATH_MAX];
    if (!gf_assets_resolve_path(am, rel_path, full_path, sizeof(full_path))) {
        return false;
    }

    FILE *fp = fopen(full_path, "rb");
    if (!fp) {
        GF_LOGE("assets", "Failed to open asset: %s", full_path);
        return false;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return false;
    }
    long len = ftell(fp);
    if (len < 0) {
        fclose(fp);
        return false;
    }
    rewind(fp);

    uint8_t *buf = (uint8_t *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(fp);
        return false;
    }

    size_t read_bytes = (len > 0) ? fread(buf, 1, (size_t)len, fp) : 0;
    fclose(fp);
    if (read_bytes != (size_t)len) {
        free(buf);
        return false;
    }
    buf[len] = '\0';
    *out_data = buf;
    *out_size = (size_t)len;
    return true;
}

static int cmp_names(const void *a, const void *b) {
    return strcmp((const char *)a, (const char *)b);
}

int gf_assets_list_dir(const GFAssetManager *am, const char *rel_subdir,
                       char out_names[][GF_ASSET_NAME_MAX], int max_names) {
    if (!am || !am->initialized || !out_names || max_names <= 0) return -1;

    char full_dir[GF_ASSET_PATH_MAX];
    if (!rel_subdir || rel_subdir[0] == '\0' || strcmp(rel_subdir, ".") == 0) {
        snprintf(full_dir, sizeof(full_dir), "%s", am->root_dir);
    } else if (!gf_assets_resolve_path(am, rel_subdir, full_dir, sizeof(full_dir))) {
        return -1;
    }

    int count = 0;
#ifdef _WIN32
    char pattern[GF_ASSET_PATH_MAX];
    snprintf(pattern, sizeof(pattern), "%s/*", full_dir);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return -1;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        if (count < max_names) {
            snprintf(out_names[count++], GF_ASSET_NAME_MAX, "%s", fd.cFileName);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR *dir = opendir(full_dir);
    if (!dir) return -1;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        if (count < max_names) {
            snprintf(out_names[count++], GF_ASSET_NAME_MAX, "%.*s", GF_ASSET_NAME_MAX - 1, ent->d_name);
        }
    }
    closedir(dir);
#endif

    if (count > 1) {
        qsort(out_names, (size_t)count, GF_ASSET_NAME_MAX, cmp_names);
    }
    return count;
}

bool gf_assets_file_mtime(const GFAssetManager *am, const char *rel_path, uint64_t *out_mtime) {
    if (!out_mtime) return false;
    *out_mtime = 0;
    char full_path[GF_ASSET_PATH_MAX];
    if (!gf_assets_resolve_path(am, rel_path, full_path, sizeof(full_path))) {
        return false;
    }
    struct stat st;
    if (stat(full_path, &st) != 0) {
        return false;
    }
    *out_mtime = (uint64_t)st.st_mtime;
    return true;
}
