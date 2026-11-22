#include "test_harness.h"

#include "assets/asset_manager.h"
#include "dsp/synth.h"
#include "lua/lua_host.h"
#include "render/scene.h"

#include <math.h>

static const char *const k_realms[8] = {
    "fantasy_realms",
    "deep_space",
    "bioluminescent_abyss",
    "crystalline_sanctum",
    "ancient_mythology",
    "cybernetic_ruins",
    "astral_choirs",
    "rhythmic_odysseys"
};

static void test_all_320_presets_and_realm_diversity(void) {
    GFAssetManager am;
    GF_ASSERT(gf_assets_init(&am, "assets", NULL));

    /* Verify catalog.json exists and lists >= 320 presets */
    uint8_t *cat_raw = NULL;
    size_t cat_sz = 0;
    GF_ASSERT(gf_assets_read_file(&am, "patches/catalog.json", &cat_raw, &cat_sz));
    GF_ASSERT(cat_sz > 5000);
    GF_ASSERT(strstr((const char *)cat_raw, "\"count\": 320") != NULL);
    free(cat_raw);

    GFSynth synth;
    gf_synth_init(&synth, 44100.0f, 0x99887766u);
    GFScene scene;
    gf_scene_init(&scene);
    GFHudBatch hud;
    gf_hud_init(&hud, NULL);

    GFLuaHost host;
    GF_ASSERT(gf_lua_init(&host, &am, &synth, &scene, &hud, false));
    GF_ASSERT(gf_lua_load_entry(&host, "scripts/main.lua"));

    int total_loaded = 0;
    double realm_zcr_sum[8] = {0};
    double realm_rms_sum[8] = {0};
    float audio_buf[1024 * 2];

    for (int r = 0; r < 8; ++r) {
        char sub[96];
        snprintf(sub, sizeof(sub), "patches/%s", k_realms[r]);
        char files[64][GF_ASSET_NAME_MAX];
        int n = gf_assets_list_dir(&am, sub, files, 64);
        GF_ASSERT(n >= 40);

        for (int i = 0; i < n; ++i) {
            if (!strstr(files[i], ".lua")) continue;
            char rel[160];
            snprintf(rel, sizeof(rel), "patches/%s/%s", k_realms[r], files[i]);
            GF_ASSERT(gf_lua_apply_preset(&host, rel));

            /* Verify every preset wires physical 3D modular patch cables */
            const GFPatchPlan *plan = gf_synth_active_plan(&synth);
            GF_ASSERT(plan != NULL && plan->cable_count >= 12);

            /* Render 1024 stereo frames on representative presets (every 4th = 80 full audio renders + cable/param audit on all 320!) */
            if ((i % 4) == 0) {
                gf_synth_all_notes_off(&synth);
                gf_synth_note_on(&synth, 52, 0.85f);
                gf_synth_note_on(&synth, 59, 0.80f);
                gf_synth_render_block(&synth, audio_buf, 512);  /* attack settle */
                gf_synth_render_block(&synth, audio_buf, 1024); /* sustain analysis */

                double sum_sq = 0.0;
                float peak = 0.0f;
                int zero_crossings = 0;
                for (int k = 0; k < 1024 * 2; ++k) {
                    float s = audio_buf[k];
                    GF_ASSERT(isfinite(s));
                    float a = fabsf(s);
                    if (a > peak) peak = a;
                    sum_sq += (double)s * (double)s;
                    if (k >= 2 && ((audio_buf[k - 2] >= 0.0f) != (s >= 0.0f))) {
                        zero_crossings++;
                    }
                }
                double rms = sqrt(sum_sq / 2048.0);
                GF_ASSERT(rms > 0.002);  /* Non-silent */
                GF_ASSERT(peak <= 1.05f);/* Safe headroom, no blowups */
                realm_rms_sum[r] += rms;
                realm_zcr_sum[r] += (double)zero_crossings / 1024.0;
            }
            total_loaded++;
        }
    }

    GF_ASSERT(total_loaded >= 320);

    /* Verify measurable timbral/spectral diversity across realms (e.g., Crystalline Sanctum brighter than Bioluminescent Abyss) */
    double cryst_zcr = realm_zcr_sum[3] / 10.0;
    double abyss_zcr = realm_zcr_sum[2] / 10.0;
    GF_ASSERT(cryst_zcr > abyss_zcr * 1.15);

    gf_lua_destroy(&host);
    gf_synth_destroy(&synth);
}

int main(void) {
    GF_RUN_TEST(test_all_320_presets_and_realm_diversity);
    GF_TEST_SUMMARY();
}
