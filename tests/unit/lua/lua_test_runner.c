#include "assets/asset_manager.h"
#include "assets/loaders.h"
#include "dsp/synth.h"
#include "lua/lua_host.h"
#include "render/scene.h"

#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *assets_dir = "assets";
    double min_cov = 95.0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--assets") == 0 && i + 1 < argc) {
            assets_dir = argv[++i];
        } else if (strcmp(argv[i], "--fail-under") == 0 && i + 1 < argc) {
            min_cov = atof(argv[++i]);
        }
    }

    GFAssetManager am;
    if (!gf_assets_init(&am, assets_dir, argv[0])) {
        fprintf(stderr, "Failed to initialize assets dir: %s\n", assets_dir);
        return 1;
    }

    GFSynth synth;
    gf_synth_init(&synth, 44100.0f, 777u);
    GFScene scene;
    gf_scene_init(&scene);

    GFFontMetrics fm = {8, 8, 16, 6, 32, 96, "fonts/retro_font.tga"};
    GFHudBatch hud;
    gf_hud_init(&hud, &fm);

    GFLuaHost host;
    if (!gf_lua_init(&host, &am, &synth, &scene, &hud, true)) {
        fprintf(stderr, "Failed to initialize Lua host\n");
        return 1;
    }

    lua_pushnumber(host.L, min_cov);
    lua_setglobal(host.L, "GF_MIN_COVERAGE");

    /* Load tests/unit/lua/run_lua_tests.lua from repo */
    FILE *fp = fopen("tests/unit/lua/run_lua_tests.lua", "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open tests/unit/lua/run_lua_tests.lua\n");
        gf_lua_destroy(&host);
        gf_synth_destroy(&synth);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    char *buf = (char *)malloc((size_t)sz + 1);
    size_t rd = fread(buf, 1, (size_t)sz, fp);
    fclose(fp);
    buf[rd] = '\0';

    int st = luaL_loadbuffer(host.L, buf, rd, "@tests/unit/lua/run_lua_tests.lua");
    free(buf);
    if (st != LUA_OK || lua_pcall(host.L, 0, 1, 0) != LUA_OK) {
        fprintf(stderr, "Lua test runner error: %s\n", lua_tostring(host.L, -1));
        gf_lua_destroy(&host);
        gf_synth_destroy(&synth);
        return 1;
    }

    bool ok = lua_toboolean(host.L, -1) != 0;
    gf_lua_destroy(&host);
    gf_synth_destroy(&synth);
    return ok ? 0 : 1;
}
