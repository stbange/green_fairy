#include "test_harness.h"

#include "assets/asset_manager.h"
#include "dsp/synth.h"
#include "lua/lua_host.h"
#include "render/scene.h"

#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir((p), 0755)
#endif

static void write_text(const char *path, const char *txt) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return;
    fwrite(txt, 1, strlen(txt), fp);
    fclose(fp);
}

static void test_lua_real_scripts_and_bindings(void) {
    GFAssetManager am;
    GF_ASSERT(gf_assets_init(&am, "assets", NULL));

    GFSynth synth;
    gf_synth_init(&synth, 44100.0f, 42u);
    GFScene scene;
    gf_scene_init(&scene);
    GFFontMetrics fm = {8, 8, 16, 6, 32, 96, "fonts/retro_font.tga"};
    GFHudBatch hud;
    gf_hud_init(&hud, &fm);

    GFLuaHost host;
    GF_ASSERT(!gf_lua_init(NULL, &am, &synth, &scene, &hud, false));
    GF_ASSERT(gf_lua_init(&host, &am, &synth, &scene, &hud, false));

    GF_ASSERT(gf_lua_load_entry(&host, "scripts/main.lua"));
    GF_ASSERT(host.last_error[0] == '\0');

    GFAudioTelemetry tel;
    memset(&tel, 0, sizeof(tel));
    tel.rms_l = 0.25f; tel.rms_r = 0.22f; tel.active_voices = 2; tel.seq_step = 3;
    GF_ASSERT(gf_lua_update(&host, 0.016f, &tel));
    GF_ASSERT(gf_lua_render_ui(&host, 1280, 720));
    GF_ASSERT(hud.vert_count > 0);

    GF_ASSERT(gf_lua_set_input_mode(&host, GF_INPUT_TOUCH));
    GF_ASSERT_STR_EQ(gf_input_mode_name(GF_INPUT_TOUCH), "touch");
    GF_ASSERT(gf_lua_render_ui(&host, 390, 844));
    GF_ASSERT(gf_lua_set_input_mode(&host, GF_INPUT_XR));
    GF_ASSERT_STR_EQ(gf_input_mode_name(GF_INPUT_XR), "xr");
    GF_ASSERT(gf_lua_render_ui(&host, 960, 720));
    GF_ASSERT(gf_lua_set_input_mode(&host, GF_INPUT_DESKTOP));

    GF_ASSERT(gf_lua_on_input(&host, "note_on", 60, 0, 0, 0.9f));
    GF_ASSERT(gf_lua_on_input(&host, "note_off", 60, 0, 0, 0.0f));
    GF_ASSERT(gf_lua_on_input(&host, "xr_trigger", 1, 0, 0, 1.0f));
    GF_ASSERT(gf_lua_on_input(&host, "xr_trigger", 1, 0, 0, 0.0f));
    GF_ASSERT(gf_lua_on_input(&host, "pointer_down", 1, 200.0f, 20.0f, 0.0f));
    GF_ASSERT(gf_lua_on_input(&host, "pointer_up", 1, 200.0f, 20.0f, 0.0f));
    GF_ASSERT(gf_lua_on_xr_ray(&host, "xr_ray_move", 0, gf_vec3(0, 1.4f, 0), gf_vec3(0, 0, -1), 0.5f));

    GF_ASSERT(gf_lua_apply_melody(&host, "melodies/organ_cathedrals/01_cornfield_cathedral_organ.lua"));
    GF_ASSERT_STR_EQ(host.looper_state, "playing");
    GF_ASSERT(host.looper_event_count >= 10);
    GF_ASSERT(!gf_lua_apply_melody(&host, "melodies/nonexistent_melody.lua"));
    GF_ASSERT(gf_lua_looper_action(&host, "stop"));
    GF_ASSERT_STR_EQ(host.looper_state, "stopped");

    GF_ASSERT(gf_lua_reload(&host));
    GF_ASSERT(host.reload_count == 1);

    gf_lua_destroy(&host);
    gf_lua_destroy(NULL);
    gf_synth_destroy(&synth);
}

static void test_lua_patchbay_spatial_mutator_and_presets(void) {
    const char *root = "/tmp/gf_lua_v2_assets";
    const char *sdir = "/tmp/gf_lua_v2_assets/scripts";
    const char *pdir = "/tmp/gf_lua_v2_assets/patches";
    MKDIR(root);
    MKDIR(sdir);
    MKDIR(pdir);

    write_text("/tmp/gf_lua_v2_assets/patches/nebula.lua",
        "gf.synth.set_param('master.gain', 0.82)\n"
        "gf.patchbay.clear()\n"
        "gf.patchbay.connect('osc1.out', 'flt1.in', 0.9)\n"
        "return { name = 'Nebula Bloom', realm = 'deep_space' }\n"
    );

    write_text("/tmp/gf_lua_v2_assets/scripts/sanctum_test.lua",
        "-- 1. Synth extended bindings\n"
        "gf.synth.reset_default()\n"
        "local plist = gf.synth.param_list()\n"
        "assert(#plist >= 150)\n"
        "assert(gf.synth.set_param(0, 0.77))\n"
        "assert(gf.synth.get_param(0) ~= nil)\n"
        "assert(not gf.synth.set_param(9999, 1.0))\n"
        "assert(gf.synth.get_param(9999) == nil)\n"
        "assert(gf.synth.set_xyz(0.2, 0.8, 0.6))\n"
        "assert(gf.synth.set_harmonics(1, false, {1.0, 0.5, 0.25, 0.1}))\n"
        "assert(not gf.synth.set_harmonics(1, false, {}))\n"
        "local wt_prev = gf.synth.get_wavetable_preview(1, false, 32)\n"
        "assert(#wt_prev == 32)\n"
        "assert(gf.synth.set_shaper_curve(1, {-1.0, -0.4, 0.0, 0.4, 1.0}))\n"
        "assert(gf.synth.set_shaper_curve(4, {-1.0, 0.0, 1.0}))\n"
        "assert(not gf.synth.set_shaper_curve(1, {0.0}))\n"
        "assert(gf.synth.set_envelope(5, {{0,0,0}, {0.2,1,0.3}, {0.8,0.5,0}, {1.5,0,-0.3}}, 3, 2, 3))\n"
        "assert(gf.synth.set_envelope('env6', {{0,0,0}, {0.5,1,0}}))\n"
        "local ed = gf.synth.get_envelope(5)\n"
        "assert(ed and #ed.points == 4 and ed.sustain == 3)\n"
        "assert(gf.synth.get_envelope(99) == nil)\n"
        "assert(gf.synth.set_seq_step(1, 7.0, 0.8, true))\n"
        "local st1 = gf.synth.get_seq_step(1)\n"
        "assert(st1 and st1.gate == true and math.abs(st1.pitch - 7.0) < 0.01)\n"
        "assert(gf.synth.get_seq_step(99) == nil)\n"
        "local ports = gf.patchbay.ports()\n"
        "assert(#ports >= 96)\n"
        "gf.patchbay.clear()\n"
        "local cid = gf.patchbay.connect('osc1.out', 'flt1.in', 0.75)\n"
        "local cid2 = gf.patchbay.connect(ports[1].id, 'osc1.pitch_cv', 1.0)\n"
        "assert(cid > 0 and cid2 > 0)\n"
        "assert(gf.patchbay.set_gain(cid, -0.5))\n"
        "local cables = gf.patchbay.list_cables()\n"
        "assert(#cables == 2)\n"
        "assert(gf.patchbay.disconnect(cid2))\n"
        "gf.patchbay.wire_default()\n"
        "assert(#gf.patchbay.list_cables() >= 8)\n"
        "gf.assets.dofile('scripts/sanctum_part2.lua')\n"
    );

    write_text("/tmp/gf_lua_v2_assets/scripts/sanctum_part2.lua",
        "gf.spatial.clear()\n"
        "local rooms = gf.spatial.rooms()\n"
        "assert(#rooms == 7)\n"
        "local rk = gf.spatial.add_rack({ room=1, title='OSC RACK', pos={0,1.5,-21.5}, rot_y=0, tilt_x=0, width=2.4, height=1.8, color={0.3,0.9,0.7,1} })\n"
        "assert(rk == 0)\n"
        "local kinds = {'knob','fader','switch','step_dial','jack_out','jack_in','env_node','harmonic_bar','xyz_orb','piano_key','button','rack_header'}\n"
        "local wids = {}\n"
        "for i, k in ipairs(kinds) do\n"
        "  wids[k] = gf.spatial.add_widget({ room=1, rack=rk, kind=k, pos={-0.8 + i*0.12, 0, 0.015}, size={0.1,0.1}, param='master.gain', port=(k=='jack_out' and 'osc1.out' or (k=='jack_in' and 'flt1.in' or nil)), steps=5, min=0, max=1, default=0.5, label=k, action='act:'..k })\n"
        "  assert(wids[k] >= 0)\n"
        "  local pk = gf.spatial.pick_ray(-0.8 + i*0.12, (k=='harmonic_bar' and 1.65 or 1.5), -19.0, 0, 0, -1)\n"
        "  assert(pk.hit == 'widget' and pk.kind == k)\n"
        "end\n"
        "assert(gf.spatial.set_widget_state(wids.knob, 0.8, 'GAIN2', true))\n"
        "assert(not gf.spatial.set_widget_state(9999, 0.5))\n"
        "assert(gf.spatial.set_widget_local_pos(wids.env_node, -0.2, 0.3, 0.02))\n"
        "assert(not gf.spatial.set_widget_local_pos(9999, 0, 0))\n"
        "gf.spatial.sync_widgets()\n"
        "gf.spatial.set_hover(wids.knob, wids.knob)\n"
        "gf.spatial.set_tooltip(true, 'GAIN: 0.80', wids.knob)\n"
        "gf.spatial.set_tooltip(false)\n"
        "assert(gf.spatial.set_curve(1, rk, 0, 0, 1.0, 0.4, {0,0.5,1}, {0.1,0.9,0.2}, false, 0.2, 1.0, 0.8, 1.0))\n"
        "assert(gf.spatial.set_curve(2, rk, 0, 0, 1.0, 0.4, nil, {0.2,0.8,0.4}, true))\n"
        "assert(gf.spatial.teleport_room(1))\n"
        "gf.spatial.walk(0.5, 0.1, 0.05)\n"
        "gf.spatial.look(0.05, -0.02)\n"
        "assert(gf.spatial.focus_rack(rk))\n"
        "assert(not gf.spatial.focus_rack(999))\n"
        "local cs = gf.spatial.camera_state()\n"
        "assert(cs.mode == 'focus' and cs.room == 1)\n"
        "local hit_scr = gf.spatial.pick_screen(640, 360)\n"
        "assert(hit_scr and hit_scr.hit ~= nil)\n"
        "local hit_ray = gf.spatial.pick_ray(0, 1.5, -19.0, 0, 0, -1)\n"
        "assert(hit_ray and hit_ray.hit ~= nil)\n"
        "local lx, ly = gf.spatial.ray_to_rack(rk, 640, 360)\n"
        "local rx, ry = gf.spatial.ray_to_rack(rk, 0, 1.5, -19.0, 0, 0, -1)\n"
        "assert(rx ~= nil and ry ~= nil)\n"
        "assert(gf.spatial.cable_begin('osc1.out', false))\n"
        "assert(gf.spatial.cable_active())\n"
        "gf.spatial.cable_update_screen(640, 360, wids.jack_in, 2.0)\n"
        "gf.spatial.cable_update_ray(0, 1.5, -19.0, 0, 0, -1, wids.jack_in, 2.0)\n"
        "local new_cid = gf.spatial.cable_finish()\n"
        "assert(new_cid > 0 and not gf.spatial.cable_active())\n"
        "gf.spatial.set_ring_hover(new_cid, -1)\n"
        "gf.spatial.exit_focus()\n"
        "local mid = gf.scene.add_mesh('models/room_shell.obj', 1)\n"
        "local sid = gf.scene.add_sprite('sprites/fairy_wisp.tga', 1)\n"
        "local vid = gf.scene.add_voxel('voxels/fairy_spire.gfvox', 1)\n"
        "assert(gf.scene.set_entity_room(mid, 2))\n"
        "assert(gf.scene.set_visible(mid, false) and gf.scene.set_visible(mid, true))\n"
        "assert(gf.scene.set_emissive(mid, true))\n"
        "assert(gf.scene.remove(vid))\n"
        "gf.scene.set_atmosphere(0.01, 0.02, 0.06, 0.08, 0.05, 0.16, 0.03, 0.03, 0.09, 0.008)\n"
        "gf.scene.set_music_state(0.75, 0.60, 0.90)\n"
        "gf.scene.set_world_theme('starfall_cosmos')\n"
        "gf.scene.set_xr_laser(true, 0, 1.2, 0, 0, 1.5, -3)\n"
        "gf.scene.set_xr_laser(false)\n"
        "local snap = gf.mutator.snapshot()\n"
        "gf.synth.set_param('master.gain', 0.12)\n"
        "gf.patchbay.clear()\n"
        "assert(gf.mutator.restore(snap))\n"
        "assert(#gf.patchbay.list_cables() >= 1)\n"
        "gf.presets.set_active_meta('Custom', 'fantasy_realms')\n"
        "local pmeta = gf.presets.apply_file('patches/nebula.lua', true)\n"
        "assert(pmeta and pmeta.name == 'Nebula Bloom')\n"
        "local badp, perr = gf.presets.apply_file('patches/missing.lua', false)\n"
        "assert(badp == nil and perr ~= nil)\n"
        "mutated_count = 0\n"
        "function gf_on_mutator(act, amt, rnd) mutated_count = mutated_count + 1 end\n"
        "function gf_on_xr_ray(ev) local pk = gf.spatial.pick_ray(ev.ox, ev.oy, ev.oz, ev.dx, ev.dy, ev.dz) end\n"
        "function gf_on_preset_path(p) local m = gf.presets.apply_file(p, true); return m ~= nil end\n"
    );

    GFAssetManager am;
    GF_ASSERT(gf_assets_init(&am, root, NULL));
    GFSynth synth;
    gf_synth_init(&synth, 44100.0f, 7u);
    GFScene scene;
    gf_scene_init(&scene);
    GFHudBatch hud;
    gf_hud_init(&hud, NULL);

    GFLuaHost host;
    GF_ASSERT(gf_lua_init(&host, &am, &synth, &scene, &hud, true));
    GF_ASSERT(gf_lua_load_entry(&host, "scripts/sanctum_test.lua"));
    GF_ASSERT_STR_EQ(host.active_preset_name, "Nebula Bloom");
    GF_ASSERT_STR_EQ(host.active_preset_realm, "deep_space");
    GF_ASSERT_STR_EQ(host.active_world_theme, "starfall_cosmos");

    /* Pick a cable ring via C-to-Lua pick_ray when ring is populated */
    GFVertex cverts[4096];
    gf_cable_build_verts(&scene.cable_batch, gf_synth_active_plan(&synth), NULL,
                         &scene.spatial, &scene.cable_drag, 1, NULL, cverts, 4096);
    if (scene.cable_batch.ring_count > 0) {
        GFVec3 rp = scene.cable_batch.rings[0].pos;
        GF_ASSERT(gf_lua_on_xr_ray(&host, "xr_trigger", 0, gf_vec3(rp.x, rp.y, rp.z + 1.0f), gf_vec3(0, 0, -1), 1.0f));
    }

    GF_ASSERT(gf_lua_call_mutator(&host, "mutate", 0.35f, 0.5f));
    GF_ASSERT(gf_lua_apply_preset(&host, "patches/nebula.lua"));
    GF_ASSERT(!gf_lua_apply_preset(&host, "patches/nonexistent.lua"));

    gf_lua_destroy(&host);
    gf_synth_destroy(&synth);
}

static void test_lua_sandbox_errors_and_fault_isolated_reload(void) {
    const char *root = "/tmp/gf_lua_test_assets";
    const char *sdir = "/tmp/gf_lua_test_assets/scripts";
    MKDIR(root);
    MKDIR(sdir);

    write_text("/tmp/gf_lua_test_assets/scripts/entry.lua",
        "assert(os == nil and io == nil and dofile == nil and loadfile == nil)\n"
        "local txt, err = gf.assets.read_text('scripts/entry.lua')\n"
        "assert(txt ~= nil)\n"
        "local mtxt, merr = gf.assets.read_text('missing.txt')\n"
        "assert(mtxt == nil and merr ~= nil)\n"
        "assert(not gf.synth.set_param('invalid.param', 1.0))\n"
        "assert(gf.synth.get_param('invalid.param') == nil)\n"
        "assert(gf.synth.set_macro(1, 0.4))\n"
        "assert(not gf.synth.set_wavetable(1, false, {0.1}))\n"
        "assert(not gf.synth.set_envelope('bad', {{0,0,0},{1,1,0}}))\n"
        "assert(not gf.synth.load_sample(1, 'missing.wav'))\n"
        "assert(not gf.synth.load_sample(1, 'scripts/entry.lua'))\n"
        "function gf_init() gf.log('v1 ok') end\n"
        "function gf_update(dt, tel) end\n"
    );

    GFAssetManager am;
    GF_ASSERT(gf_assets_init(&am, root, NULL));
    GFSynth synth;
    gf_synth_init(&synth, 44100.0f, 1u);
    GFScene scene;
    gf_scene_init(&scene);
    GFHudBatch hud;
    gf_hud_init(&hud, NULL);

    GFLuaHost host;
    GF_ASSERT(gf_lua_init(&host, &am, &synth, &scene, &hud, true));
    GF_ASSERT(!gf_lua_load_entry(&host, "scripts/does_not_exist.lua"));
    GF_ASSERT(gf_lua_load_entry(&host, "scripts/entry.lua"));

    GF_ASSERT(gf_lua_render_ui(&host, 640, 480));
    GF_ASSERT(gf_lua_on_input(&host, "note_on", 60, 0, 0, 1.0f));
    GF_ASSERT(!gf_lua_call_mutator(&host, "mutate", 0.2f, 0.2f));
    GF_ASSERT(gf_lua_on_xr_ray(&host, "xr_move", 0, gf_vec3(0, 1, 0), gf_vec3(0, 0, -1), 0.0f));
    GF_ASSERT(gf_lua_set_input_mode(&host, GF_INPUT_DESKTOP));

    write_text("/tmp/gf_lua_test_assets/scripts/entry.lua", "function broken( !!!\n");
    GF_ASSERT(!gf_lua_reload(&host));
    GF_ASSERT(host.last_error[0] != '\0');
    GF_ASSERT(gf_lua_update(&host, 0.016f, NULL));

    write_text("/tmp/gf_lua_test_assets/scripts/entry.lua",
        "function gf_init() error('boom in init') end\n"
    );
    GF_ASSERT(!gf_lua_reload(&host));
    GF_ASSERT(strstr(host.last_error, "boom in init") != NULL);

    write_text("/tmp/gf_lua_test_assets/scripts/entry.lua",
        "local x = require('nonexistent_mod')\n"
    );
    GF_ASSERT(!gf_lua_reload(&host));

    write_text("/tmp/gf_lua_test_assets/scripts/entry.lua",
        "function gf_init() gf.log('restored') end\n"
    );
    GF_ASSERT(gf_lua_reload(&host));
    GF_ASSERT(host.reload_count == 1);

    gf_lua_destroy(&host);
    gf_synth_destroy(&synth);
}

int main(void) {
    GF_RUN_TEST(test_lua_real_scripts_and_bindings);
    GF_RUN_TEST(test_lua_patchbay_spatial_mutator_and_presets);
    GF_RUN_TEST(test_lua_sandbox_errors_and_fault_isolated_reload);
    GF_TEST_SUMMARY();
}
