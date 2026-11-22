#include "platform/sdl_runtime.h"

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    (void)argc;
    (void)argv;
    /* Web entry is triggered by web/js/bridge.js via gf_web_start() after fetching external /assets/. */
    return 0;
#else
    GFAppConfig cfg;
    if (!gf_app_parse_args(argc, argv, &cfg)) {
        return 2;
    }
    return gf_sdl_run(&cfg);
#endif
}
