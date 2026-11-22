#ifndef GF_ASSETS_HOT_RELOAD_H
#define GF_ASSETS_HOT_RELOAD_H

#include "assets/asset_manager.h"

#include <stdbool.h>
#include <stdint.h>

#define GF_MAX_WATCHED_FILES 64

typedef void (*GFReloadCallback)(const char *rel_path, void *user_data);

typedef struct GFWatchedEntry {
    char rel_path[GF_ASSET_NAME_MAX];
    uint64_t last_mtime;
    GFReloadCallback cb;
    void *user_data;
} GFWatchedEntry;

typedef struct GFHotReloader {
    const GFAssetManager *am;
    GFWatchedEntry entries[GF_MAX_WATCHED_FILES];
    int count;
} GFHotReloader;

void gf_hot_reload_init(GFHotReloader *hr, const GFAssetManager *am);
bool gf_hot_reload_watch(GFHotReloader *hr, const char *rel_path, GFReloadCallback cb, void *user_data);
int gf_hot_reload_poll(GFHotReloader *hr);
int gf_hot_reload_trigger_all(GFHotReloader *hr);

#endif
