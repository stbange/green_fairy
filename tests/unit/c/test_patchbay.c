#include "test_harness.h"

#include "dsp/patchbay.h"

static void test_port_registry_and_lookup(void) {
    GF_ASSERT(gf_port_info((GFPortId)-1) == NULL);
    GF_ASSERT(gf_port_info(GF_PORT_COUNT) == NULL);
    GF_ASSERT(gf_port_from_name(NULL) == -1);
    GF_ASSERT(gf_port_from_name("") == -1);
    GF_ASSERT(gf_port_from_name("nonexistent.port") == -1);

    for (int i = 0; i < GF_PORT_COUNT; ++i) {
        const GFPortInfo *info = gf_port_info((GFPortId)i);
        GF_ASSERT(info != NULL);
        GF_ASSERT((int)info->id == i);
        GF_ASSERT(gf_port_from_name(info->name) == i);
        GF_ASSERT(info->room_id >= 1 && info->room_id <= 7);
        if (i < GF_PORT_IN_START) {
            GF_ASSERT(info->is_output && gf_port_is_output((GFPortId)i));
            GF_ASSERT(!gf_port_is_input((GFPortId)i));
        } else {
            GF_ASSERT(!info->is_output && gf_port_is_input((GFPortId)i));
            GF_ASSERT(!gf_port_is_output((GFPortId)i));
        }
    }
}

static void test_stackable_cables_attenuverters_and_routing(void) {
    GFPatchbay pb;
    gf_patchbay_init(NULL);
    gf_patchbay_init(&pb);

    /* Reject invalid directions (input->input or output->output) */
    GF_ASSERT(gf_patchbay_connect(&pb, GF_PORT_FLT_1_IN, GF_PORT_FLT_2_IN, 1.0f) == -1);
    GF_ASSERT(gf_patchbay_connect(&pb, GF_PORT_OSC_1_OUT, GF_PORT_OSC_2_OUT, 1.0f) == -1);

    /* Stack two modulators into osc1.fm_in */
    int c1 = gf_patchbay_connect(&pb, GF_PORT_ENV_2_OUT, GF_PORT_OSC_1_FM_IN, 0.8f);
    int c2 = gf_patchbay_connect(&pb, GF_PORT_LFO_2_OUT, GF_PORT_OSC_1_FM_IN, -0.4f);
    GF_ASSERT(c1 > 0 && c2 > 0 && c1 != c2);

    /* Duplicate connect updates gain in place */
    int c1_dup = gf_patchbay_connect(&pb, GF_PORT_ENV_2_OUT, GF_PORT_OSC_1_FM_IN, 0.75f);
    GF_ASSERT(c1_dup == c1);
    GF_ASSERT(gf_patchbay_find_cable(&pb, GF_PORT_ENV_2_OUT, GF_PORT_OSC_1_FM_IN) == c1);
    GF_ASSERT(gf_patchbay_find_cable(&pb, GF_PORT_OSC_3_OUT, GF_PORT_OSC_1_FM_IN) == -1);

    GF_ASSERT(gf_patchbay_set_gain(&pb, c2, -0.5f));
    GF_ASSERT(!gf_patchbay_set_gain(&pb, 9999, 1.0f));

    GFPatchPlan plan;
    gf_patchbay_compile(&pb, &plan);
    GF_ASSERT(plan.cable_count == 2);
    GF_ASSERT(!plan.has_cycles);
    GF_ASSERT(gf_patch_plan_port_connected(&plan, GF_PORT_OSC_1_FM_IN));
    GF_ASSERT(!gf_patch_plan_port_connected(&plan, GF_PORT_FLT_1_IN));

    float bus[GF_PORT_COUNT];
    float fb[GF_PORT_COUNT];
    float peaks[GF_MAX_CABLES];
    memset(bus, 0, sizeof(bus));
    memset(fb, 0, sizeof(fb));
    memset(peaks, 0, sizeof(peaks));

    bus[GF_PORT_ENV_2_OUT] = 1.0f;
    bus[GF_PORT_LFO_2_OUT] = 0.5f;
    gf_patch_plan_route_module(&plan, GF_MOD_NODE_OSC1, bus, fb, peaks);
    /* 1.0 * 0.75 + 0.5 * (-0.5) = 0.50 */
    GF_ASSERT_NEAR(bus[GF_PORT_OSC_1_FM_IN], 0.50f, 1e-6);
    GF_ASSERT_NEAR(peaks[0], 0.75f, 1e-6);
    GF_ASSERT_NEAR(peaks[1], 0.25f, 1e-6);

    GF_ASSERT(gf_patchbay_disconnect(&pb, c1));
    GF_ASSERT(!gf_patchbay_disconnect(&pb, c1));
    GF_ASSERT(gf_patchbay_disconnect_port(&pb, GF_PORT_OSC_1_FM_IN) == 1);
    GF_ASSERT(gf_patchbay_disconnect_port(&pb, (GFPortId)999) == 0);
}

static void test_cycle_detection_and_feedback_stability(void) {
    GFPatchbay pb;
    gf_patchbay_init(&pb);
    gf_patchbay_wire_default_absynth(&pb);

    GFPatchPlan plan;
    gf_patchbay_compile(&pb, &plan);
    GF_ASSERT(plan.cable_count >= 20);
    GF_ASSERT(!plan.has_cycles);

    /* Create mutual FM cycle: Osc1 <-> Osc2 AND Master Aether -> Osc3 */
    gf_patchbay_connect(&pb, GF_PORT_OSC_1_OUT, GF_PORT_OSC_2_FM_IN, 1.2f);
    gf_patchbay_connect(&pb, GF_PORT_OSC_2_OUT, GF_PORT_OSC_1_FM_IN, 1.2f);
    gf_patchbay_connect(&pb, GF_PORT_AETHER_OUT, GF_PORT_OSC_3_FM_IN, 0.9f);

    gf_patchbay_compile(&pb, &plan);
    GF_ASSERT(plan.has_cycles);

    float bus[GF_PORT_COUNT];
    float fb[GF_PORT_COUNT];
    memset(fb, 0, sizeof(fb));
    for (int n = 0; n < 256; ++n) {
        memset(bus, 0, sizeof(bus));
        for (int step = 0; step < plan.exec_count; ++step) {
            GFModuleId m = plan.exec_order[step];
            gf_patch_plan_route_module(&plan, m, bus, fb, NULL);
            if (m == GF_MOD_NODE_OSC1) bus[GF_PORT_OSC_1_OUT] = sinf(0.2f * (float)n + bus[GF_PORT_OSC_1_FM_IN]);
            if (m == GF_MOD_NODE_OSC2) bus[GF_PORT_OSC_2_OUT] = cosf(0.3f * (float)n + bus[GF_PORT_OSC_2_FM_IN]);
            if (m == GF_MOD_NODE_AETHER) bus[GF_PORT_AETHER_OUT] = 2.5f; /* Hot signal clamped by tanhf */
        }
        gf_patch_plan_commit_feedback(bus, fb);
        GF_ASSERT(isfinite(fb[GF_PORT_OSC_1_OUT]) && fabsf(fb[GF_PORT_AETHER_OUT]) <= 1.0f);
    }
}

int main(void) {
    GF_RUN_TEST(test_port_registry_and_lookup);
    GF_RUN_TEST(test_stackable_cables_attenuverters_and_routing);
    GF_RUN_TEST(test_cycle_detection_and_feedback_stability);
    GF_TEST_SUMMARY();
}
