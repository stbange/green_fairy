#include "test_harness.h"

#include "core/math3d.h"
#include "dsp/synth.h"
#include "render/cable_renderer.h"
#include "render/camera_rig.h"
#include "render/spatial_ui.h"

static void test_mat4_invert_and_camera_rig(void) {
    GFMat4 m = gf_mat4_trs(gf_vec3(2.0f, -1.5f, 4.0f), gf_vec3(0.2f, -0.5f, 0.1f), gf_vec3(1.5f, 2.0f, 0.8f));
    GFMat4 inv;
    GF_ASSERT(gf_mat4_invert(m, &inv));
    GFMat4 prod = gf_mat4_mul(m, inv);
    GF_ASSERT_NEAR(prod.m[0], 1.0f, 1e-4);
    GF_ASSERT_NEAR(prod.m[5], 1.0f, 1e-4);
    GF_ASSERT_NEAR(prod.m[10], 1.0f, 1e-4);
    GF_ASSERT_NEAR(prod.m[12], 0.0f, 1e-4);

    GFMat4 zero_m;
    memset(&zero_m, 0, sizeof(zero_m));
    GF_ASSERT(!gf_mat4_invert(zero_m, &inv));
    GF_ASSERT(!gf_mat4_invert(m, NULL));

    /* Floorplan metadata */
    GF_ASSERT(gf_sanctum_room_def(0) == NULL);
    GF_ASSERT(gf_sanctum_room_def(8) == NULL);
    for (int r = 1; r <= GF_SANCTUM_ROOM_COUNT; ++r) {
        const GFRoomDef *rd = gf_sanctum_room_def(r);
        GF_ASSERT(rd != NULL && rd->id == r && rd->name != NULL);
    }
    GF_ASSERT(gf_sanctum_corridor_def(-1) == NULL);
    GF_ASSERT(gf_sanctum_corridor_def(GF_SANCTUM_CORRIDOR_COUNT) == NULL);
    GF_ASSERT(gf_sanctum_corridor_def(0) != NULL);

    /* Walk from Room 6 (Bridge) straight north along corridor into Room 1 (Oscillator Conservatory) */
    GFCameraRig rig;
    gf_camera_rig_init(NULL);
    gf_camera_rig_init(&rig);
    GF_ASSERT(rig.current_room == 6);
    GF_ASSERT(!gf_camera_teleport_room(&rig, 99));

    for (int step = 0; step < 45; ++step) {
        gf_camera_walk(&rig, 1.0f, 0.0f, 0.10f);
    }
    GF_ASSERT(rig.current_room == 1);
    GF_ASSERT(rig.pos.z < -13.0f);

    /* Wall collision clamping outside sanctum bounds */
    GFVec3 clamped = gf_camera_clamp_walkable(gf_vec3(100.0f, 1.6f, -100.0f));
    GF_ASSERT(fabsf(clamped.x) < 25.0f && fabsf(clamped.z) < 25.0f);
    /* Corridor interior point stays unclamped */
    GFVec3 corr_pt = gf_vec3(0.2f, 1.62f, -9.0f);
    GFVec3 corr_clamped = gf_camera_clamp_walkable(corr_pt);
    GF_ASSERT_NEAR(corr_clamped.x, corr_pt.x, 1e-4);
    GF_ASSERT_NEAR(corr_clamped.z, corr_pt.z, 1e-4);

    /* Free look wrap-around and pitch clamp */
    gf_camera_look(&rig, 7.0f, 5.0f);
    GF_ASSERT(rig.yaw >= -GF_PI && rig.yaw <= GF_PI);
    GF_ASSERT(rig.pitch <= 1.23f);
    gf_camera_look(&rig, -14.0f, -10.0f);
    GF_ASSERT(rig.pitch >= -1.23f);

    /* Smooth cubic Rack-Focus zoom and Escape exit restoration */
    GFVec3 pre_focus = rig.pos;
    gf_camera_focus_rack(&rig, 3, gf_vec3(0.0f, 1.5f, -22.5f), gf_vec3(0.0f, 0.0f, 1.0f), 2.4f, 1.8f, 16.0f / 9.0f);
    GF_ASSERT(rig.mode == GF_CAM_RACK_FOCUS && rig.focused_rack_id == 3 && rig.animating);
    /* Look is locked during Rack-Focus */
    float locked_yaw = rig.yaw;
    gf_camera_look(&rig, 0.5f, 0.2f);
    GF_ASSERT_NEAR(rig.yaw, locked_yaw, 1e-6);

    for (int i = 0; i < 10; ++i) gf_camera_step(&rig, 0.05f);
    GF_ASSERT(!rig.animating);
    GF_ASSERT_NEAR(rig.fov_deg, 48.0f, 1e-3);
    GF_ASSERT_NEAR(rig.target.z, -22.5f, 1e-3);

    /* Frustum & Rack-Focus visibility culling: Room 1 is visible, opposite Room 4 (z=+18) is culled! */
    GFMat4 view, proj;
    gf_camera_compute_matrices(&rig, 16.0f / 9.0f, 0.08f, 85.0f, &view, &proj);
    GF_ASSERT(gf_camera_is_room_visible(&rig, &view, rig.fov_deg, 16.0f / 9.0f, 85.0f, 1));
    GF_ASSERT(!gf_camera_is_room_visible(&rig, &view, rig.fov_deg, 16.0f / 9.0f, 85.0f, 4));
    GF_ASSERT(gf_camera_is_room_visible(&rig, &view, rig.fov_deg, 16.0f / 9.0f, 85.0f, 0));

    /* Walking automatically exits Rack-Focus back toward free-walk */
    gf_camera_walk(&rig, -0.5f, 0.3f, 0.05f);
    GF_ASSERT(rig.mode == GF_CAM_FREE_WALK);
    for (int i = 0; i < 10; ++i) gf_camera_step(&rig, 0.05f);
    GF_ASSERT_NEAR(rig.fov_deg, 62.0f, 1e-2);
    (void)pre_focus;

    /* WebXR view transform & room teleport */
    GF_ASSERT(gf_camera_teleport_room(&rig, 5));
    GF_ASSERT(rig.current_room == 5);
    GFMat4 xr_v = gf_camera_apply_xr_view(&rig, gf_mat4_identity());
    GF_ASSERT(fabsf(xr_v.m[12]) > 1.0f || fabsf(xr_v.m[14]) > 1.0f);
    GF_ASSERT(gf_vec3_len(gf_camera_right(&rig)) > 0.9f);
}

static void test_spatial_widgets_raypicking_and_curves(void) {
    GFSynth synth;
    gf_synth_init(&synth, 44100.0f, 1234u);

    GFSpatialUI ui;
    gf_spatial_init(&ui);

    /* Place a rack in Room 1 at (0, 1.5, -21.5) facing +Z */
    int rk = gf_spatial_add_rack(&ui, 1, "OSC 1 CONSERVATORY",
                                 gf_vec3(0.0f, 1.5f, -21.5f), 0.0f, 0.0f,
                                 2.4f, 1.8f, (GFVec4){0.2f, 0.9f, 0.7f, 1.0f});
    GF_ASSERT(rk == 0);
    GF_ASSERT(ui.widget_count == 1); /* Auto-created GF_W3D_RACK_HEADER */

    /* Mount rotary knob, fader, switch, step dial, harmonic bar, env node, piano key, button, XYZ orb, and jacks */
    GFWidgetDesc3D d;
    memset(&d, 0, sizeof(d));
    d.room_id = 1; d.rack_id = rk; d.param_id = -1; d.port_id = -1;
    d.color = (GFVec4){0.4f, 1.0f, 0.8f, 1.0f};

    d.kind = GF_W3D_KNOB;
    d.local_pos = gf_vec3(-0.6f, 0.2f, 0.01f);
    d.size = (GFVec2){0.14f, 0.14f};
    d.param_id = GF_P_MASTER_FILTER_CUTOFF;
    d.min_val = 40.0f; d.max_val = 16000.0f; d.default_val = 2000.0f;
    d.label = "CUTOFF";
    int w_knob = gf_spatial_add_widget(&ui, &d);

    d.kind = GF_W3D_FADER;
    d.local_pos = gf_vec3(0.0f, 0.1f, 0.01f);
    d.size = (GFVec2){0.10f, 0.08f};
    d.track_len = 0.40f;
    d.param_id = GF_P_MASTER_GAIN;
    d.min_val = 0.0f; d.max_val = 1.0f; d.default_val = 0.7f;
    d.label = "GAIN";
    int w_fader = gf_spatial_add_widget(&ui, &d);

    d.kind = GF_W3D_SWITCH;
    d.local_pos = gf_vec3(0.5f, 0.2f, 0.01f);
    d.size = (GFVec2){0.10f, 0.10f};
    d.param_id = GF_P_DRONE_LATCH;
    d.min_val = 0.0f; d.max_val = 1.0f; d.default_val = 0.0f;
    d.label = "DRONE";
    int w_sw = gf_spatial_add_widget(&ui, &d);

    d.kind = GF_W3D_STEP_DIAL;
    d.local_pos = gf_vec3(0.8f, 0.2f, 0.01f);
    d.step_count = 9;
    d.param_id = GF_P_CH_BASE;
    d.min_val = 0.0f; d.max_val = 8.0f; d.default_val = 0.0f;
    d.label = "MODE";
    int w_dial = gf_spatial_add_widget(&ui, &d);

    d.kind = GF_W3D_HARMONIC_BAR;
    d.local_pos = gf_vec3(-0.8f, -0.4f, 0.01f);
    d.size = (GFVec2){0.05f, 0.05f};
    d.track_len = 0.25f;
    d.param_id = -1;
    d.min_val = 0.0f; d.max_val = 1.0f; d.default_val = 0.8f;
    d.label = "";
    int w_hbar = gf_spatial_add_widget(&ui, &d);

    d.kind = GF_W3D_ENV_NODE;
    d.local_pos = gf_vec3(-0.3f, -0.3f, 0.02f);
    d.size = (GFVec2){0.08f, 0.08f};
    d.label = "PT1";
    int w_env = gf_spatial_add_widget(&ui, &d);

    d.kind = GF_W3D_PIANO_KEY;
    d.local_pos = gf_vec3(0.3f, -0.4f, 0.01f);
    d.size = (GFVec2){0.09f, 0.22f};
    d.aux_idx = 60;
    d.label = "C4";
    int w_key = gf_spatial_add_widget(&ui, &d);

    d.kind = GF_W3D_BUTTON;
    d.local_pos = gf_vec3(0.75f, -0.4f, 0.01f);
    d.size = (GFVec2){0.18f, 0.10f};
    d.label = "MUTATE";
    int w_btn = gf_spatial_add_widget(&ui, &d);

    d.rack_id = -1;
    d.kind = GF_W3D_XYZ_ORB;
    d.local_pos = gf_vec3(0.0f, 1.2f, -19.0f);
    d.size = (GFVec2){0.20f, 0.20f};
    d.label = "XYZ";
    int w_orb = gf_spatial_add_widget(&ui, &d);

    GF_ASSERT(w_knob > 0 && w_fader > 0 && w_sw > 0 && w_dial > 0 &&
              w_hbar > 0 && w_env > 0 && w_key > 0 && w_btn > 0 && w_orb > 0);

    /* Frame rack with Camera Rig and cast exact screen rays onto the 3D knob & switch */
    GFCameraRig rig;
    gf_camera_rig_init(&rig);
    gf_camera_focus_rack(&rig, rk, gf_vec3(0.0f, 1.5f, -21.5f), gf_vec3(0, 0, 1), 2.4f, 1.8f, 1280.0f / 720.0f);
    gf_camera_step(&rig, 1.0f);
    GFMat4 view, proj;
    gf_camera_compute_matrices(&rig, 1280.0f / 720.0f, 0.08f, 85.0f, &view, &proj);

    /* Project knob world center to screen coordinates, then unproject back via gf_ray_from_screen! */
    const GFWidget3D *kw = gf_spatial_get_widget_const(&ui, w_knob);
    GFVec4 clip = gf_mat4_mul_vec4(gf_mat4_mul(proj, view), (GFVec4){kw->world_pos.x, kw->world_pos.y, kw->world_pos.z, 1.0f});
    float sx = ((clip.x / clip.w) * 0.5f + 0.5f) * 1280.0f;
    float sy = (0.5f - (clip.y / clip.w) * 0.5f) * 720.0f;

    GFRay ray = gf_ray_from_screen(sx, sy, 1280.0f, 720.0f, &view, &proj);
    float hit_t = 0.0f;
    int picked = gf_spatial_pick(&ui, ray, NULL, false, &hit_t);
    GF_ASSERT(picked == w_knob);
    GF_ASSERT(hit_t > 0.2f);

    GFVec2 rack_xy;
    GF_ASSERT(gf_spatial_ray_to_rack_local(&ui, rk, ray, &rack_xy));
    GF_ASSERT_NEAR(rack_xy.x, -0.6f, 0.03f);
    GF_ASSERT_NEAR(rack_xy.y,  0.2f, 0.03f);

    /* Value conversions & synth state synchronization */
    gf_synth_set_param(&synth, GF_P_MASTER_FILTER_CUTOFF, 8000.0f);
    gf_synth_set_param(&synth, GF_P_DRONE_LATCH, 1.0f);
    gf_synth_set_param(&synth, GF_P_CH_BASE, 5.0f);
    float dummy_lr[128];
    gf_synth_render_block(&synth, dummy_lr, 64);
    gf_spatial_sync_from_synth(&ui, &synth);

    GF_ASSERT_NEAR(gf_widget_value_from_norm(gf_spatial_get_widget_const(&ui, w_knob)), 8000.0f, 1.0f);
    GF_ASSERT_NEAR(gf_widget_value_from_norm(gf_spatial_get_widget_const(&ui, w_sw)), 1.0f, 1e-4f);
    GF_ASSERT_NEAR(gf_widget_value_from_norm(gf_spatial_get_widget_const(&ui, w_dial)), 5.0f, 1e-4f);

    /* Verify model matrices & mesh paths for all widget kinds */
    for (int i = 0; i < ui.widget_count; ++i) {
        GFMat4 wm = gf_spatial_widget_model_matrix(&ui, &ui.widgets[i]);
        GF_ASSERT(wm.m[15] == 1.0f);
    }

    /* 3D Holographic Curves (both line-ribbon and harmonic bars) & World-Space Bitmap Text */
    float xs[4] = {0.0f, 0.2f, 0.6f, 1.0f};
    float ys[4] = {0.0f, 1.0f, 0.5f, 0.0f};
    GF_ASSERT(gf_spatial_set_curve_xy(&ui, 0, rk, 0.0f, -0.2f, 1.2f, 0.4f, xs, ys, 4, false, (GFVec4){0, 1, 0.8f, 1}));
    GF_ASSERT(gf_spatial_set_curve_xy(&ui, 1, rk, 0.0f,  0.2f, 1.2f, 0.4f, NULL, ys, 4, true, (GFVec4){1, 0.8f, 0.2f, 1}));

    GFVertex cverts[1024];
    int cv = gf_spatial_build_curve_verts(&ui, NULL, cverts, 1024);
    GF_ASSERT(cv >= 30);

    ui.hovered_widget_id = w_knob;
    ui.tooltip_visible = true;
    ui.tooltip_pos = kw->world_pos;
    snprintf(ui.tooltip_text, sizeof(ui.tooltip_text), "CUTOFF: 8000 Hz");

    GFFontMetrics fm = {8, 8, 16, 6, 32, 96, "fonts/retro_font.tga"};
    GFVertex tverts[4096];
    int tv = gf_spatial_build_world_text_verts(&ui, &fm, NULL, tverts, 4096);
    GF_ASSERT(tv >= 60);

    gf_synth_destroy(&synth);
}

static void test_catenary_cables_conduits_and_repatching(void) {
    GFSynth synth;
    gf_synth_init(&synth, 44100.0f, 99u);
    gf_synth_clear_cables(&synth);

    GFSpatialUI ui;
    gf_spatial_init(&ui);
    int rk1 = gf_spatial_add_rack(&ui, 1, "OSC RACK", gf_vec3(0, 1.5f, -21.0f), 0, 0, 2.0f, 1.6f, (GFVec4){0.2f, 0.9f, 0.7f, 1});
    int rk2 = gf_spatial_add_rack(&ui, 2, "FLT RACK", gf_vec3(19.0f, 1.5f, -10.0f), -1.57f, 0, 2.0f, 1.6f, (GFVec4){0.9f, 0.6f, 0.2f, 1});

    GFWidgetDesc3D jd;
    memset(&jd, 0, sizeof(jd));
    jd.size = (GFVec2){0.09f, 0.09f};
    jd.max_val = 1.0f;
    jd.color = (GFVec4){0.3f, 1.0f, 0.8f, 1.0f};

    jd.room_id = 1; jd.rack_id = rk1; jd.kind = GF_W3D_JACK_OUT;
    jd.port_id = GF_PORT_OSC_1_OUT; jd.local_pos = gf_vec3(-0.4f, -0.5f, 0.01f); jd.label = "OSC1 OUT";
    int j_osc1_out = gf_spatial_add_widget(&ui, &jd);

    jd.room_id = 1; jd.rack_id = rk1; jd.kind = GF_W3D_JACK_IN;
    jd.port_id = GF_PORT_OSC_2_FM_IN; jd.local_pos = gf_vec3(0.4f, -0.5f, 0.01f); jd.label = "OSC2 FM";
    int j_osc2_fm = gf_spatial_add_widget(&ui, &jd);

    jd.room_id = 2; jd.rack_id = rk2; jd.kind = GF_W3D_JACK_IN;
    jd.port_id = GF_PORT_FLT_1_IN; jd.local_pos = gf_vec3(-0.3f, -0.5f, 0.01f); jd.label = "FLT1 IN";
    int j_flt1_in = gf_spatial_add_widget(&ui, &jd);
    (void)j_osc1_out;

    /* 1. Drag a brand-new intra-room cable from OSC_1_OUT -> OSC_2_FM_IN */
    GFCableDrag3D drag;
    gf_cable_drag_init(&drag);
    GF_ASSERT(gf_cable_begin_from_jack(&drag, &synth, &ui, 1, GF_PORT_OSC_1_OUT, false));
    GF_ASSERT(drag.active && drag.anchor_port == GF_PORT_OSC_1_OUT);

    GFRay dummy_ray = {gf_vec3(0, 1.5f, -18.0f), gf_vec3(0, 0, -1)};
    gf_cable_update_drag(&drag, &ui, dummy_ray, 2.0f, j_osc2_fm);
    GF_ASSERT(drag.hovered_port == (int)GF_PORT_OSC_2_FM_IN);

    GFCableBatch3D batch;
    gf_cable_batch_init(&batch);
    GFVertex verts[8192];
    /* Preview cable emits geometry while dragging */
    int preview_v = gf_cable_build_verts(&batch, gf_synth_active_plan(&synth), NULL, &ui, &drag, 1, NULL, verts, 8192);
    GF_ASSERT(preview_v >= 300);

    int cid1 = gf_cable_finish_drag(&drag, &synth);
    GF_ASSERT(cid1 > 0 && !drag.active);

    /* 2. Unplug that cable by clicking occupied input jack OSC_2_FM_IN and re-patch cross-room to FLT_1_IN! */
    GF_ASSERT(gf_cable_begin_from_jack(&drag, &synth, &ui, 1, GF_PORT_OSC_2_FM_IN, false));
    GF_ASSERT(drag.active && drag.anchor_port == GF_PORT_OSC_1_OUT);
    GF_ASSERT(gf_synth_active_plan(&synth)->cable_count == 0); /* Unplugged live! */

    gf_cable_update_drag(&drag, &ui, dummy_ray, 2.0f, j_flt1_in);
    GF_ASSERT(drag.hovered_port == (int)GF_PORT_FLT_1_IN);
    int cid2 = gf_cable_finish_drag(&drag, &synth);
    GF_ASSERT(cid2 > 0);
    GF_ASSERT(gf_synth_active_plan(&synth)->cable_count == 1);
    GF_ASSERT(gf_synth_active_plan(&synth)->cables[0].dst == GF_PORT_FLT_1_IN);

    /* Add an intra-room cable too so both intra-room catenary AND cross-room overhead conduit build together */
    int cid3 = gf_synth_connect_cable(&synth, GF_PORT_OSC_1_OUT, GF_PORT_OSC_2_FM_IN, -0.5f);
    GF_ASSERT(cid3 > 0);

    int total_v = gf_cable_build_verts(&batch, gf_synth_active_plan(&synth), NULL, &ui, &drag, 1, NULL, verts, 8192);
    GF_ASSERT(total_v >= 600);
    GF_ASSERT(batch.ring_count == 3); /* 1 intra-room ring + 2 cross-room rings (src & dst rooms) */

    /* Ray-pick the 3D attenuverter ring right at its world position */
    GFVec3 rpos = batch.rings[0].pos;
    GFRay ring_ray = {gf_vec3(rpos.x, rpos.y, rpos.z + 1.0f), gf_vec3(0, 0, -1)};
    float rt = 0.0f;
    int picked_cid = gf_cable_pick_ring(&batch, ring_ray, NULL, true, &rt);
    GF_ASSERT(picked_cid == batch.rings[0].cable_id);
    GFMat4 rm = gf_cable_ring_model_matrix(&batch.rings[0]);
    GF_ASSERT(rm.m[15] == 1.0f);

    gf_synth_destroy(&synth);
}

int main(void) {
    GF_RUN_TEST(test_mat4_invert_and_camera_rig);
    GF_RUN_TEST(test_spatial_widgets_raypicking_and_curves);
    GF_RUN_TEST(test_catenary_cables_conduits_and_repatching);
    GF_TEST_SUMMARY();
}
