#include "assets/hot_reload.h"

#include <stdio.h>
#include <string.h>

void gf_hot_reload_init(GFHotReloader *hr, const GFAssetManager *am) {
    if (!hr) return;
    memset(hr, 0, sizeof(*hr));
    hr->am = am;
}

bool gf_hot_reload_watch(GFHotReloader *hr, const char *rel_path, GFReloadCallback cb, void *user_data) {
    if (!hr || !hr->am || !rel_path || !cb || hr->count >= GF_MAX_WATCHED_FILES) {
        return false;
    }
    uint64_t mtime = 0;
    if (!gf_assets_file_mtime(hr->am, rel_path, &mtime)) {
        return false;
    }
    GFWatchedEntry *e = &hr->entries[hr->count++];
    snprintf(e->rel_path, sizeof(e->rel_path), "%s", rel_path);
    e->last_mtime = mtime;
    e->cb = cb;
    e->user_data = user_data;
    return true;
}

int gf_hot_reload_poll(GFHotReloader *hr) {
    if (!hr || !hr->am) return 0;
    int changed = 0;
    for (int i = 0; i < hr->count; ++i) {
        GFWatchedEntry *e = &hr->entries[i];
        uint64_t mtime = 0;
        if (gf_assets_file_mtime(hr->am, e->rel_path, &mtime) && mtime != e->last_mtime) {
            e->last_mtime = mtime;
            e->cb(e->rel_path, e->user_data);
            changed++;
        }
    }
    return changed;
}

int gf_hot_reload_trigger_all(GFHotReloader *hr) {
    if (!hr || !hr->am) return 0;
    for (int i = 0; i < hr->count; ++i) {
        GFWatchedEntry *e = &hr->entries[i];
        uint64_t mtime = 0;
        if (gf_assets_file_mtime(hr->am, e->rel_path, &mtime)) {
            e->last_mtime = mtime;
        }
        e->cb(e->rel_path, e->user_data);
    }
    return hr->count;
}
