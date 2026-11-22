#include "render/cable_renderer.h"
#include "render/camera_rig.h"

#include <string.h>

#define GF_TUBE_SIDES 5

void gf_cable_drag_init(GFCableDrag3D *drag) {
    if (!drag) return;
    memset(drag, 0, sizeof(*drag));
    drag->hovered_port = -1;
}

void gf_cable_batch_init(GFCableBatch3D *cb) {
    if (!cb) return;
    memset(cb, 0, sizeof(*cb));
    cb->active_ring_cable_id = -1;
    cb->hovered_ring_cable_id = -1;
}

static GFVec4 cable_color_for(const GFPortInfo *src_info, float gain, bool is_feedback) {
    if (is_feedback) {
        return (GFVec4){1.00f, 0.36f, 0.24f, 0.98f};
    }
    if (!src_info) {
        return (GFVec4){0.35f, 0.95f, 0.75f, 0.95f};
    }
    if (src_info->kind == GF_SIG_AUDIO) {
        return (gain < 0.0f) ? (GFVec4){0.20f, 0.92f, 0.95f, 0.96f}
                             : (GFVec4){0.22f, 0.98f, 0.64f, 0.96f};
    }
    if (src_info->kind == GF_SIG_TRIGGER) {
        return (GFVec4){1.00f, 0.82f, 0.26f, 0.96f};
    }
    /* CV: Cyan/Amber when positive, Magenta/Rose when inverted by bipolar attenuverter */
    return (gain < 0.0f) ? (GFVec4){0.98f, 0.42f, 0.76f, 0.95f}
                         : (GFVec4){0.30f, 0.80f, 1.00f, 0.95f};
}

bool gf_cable_begin_from_jack(GFCableDrag3D *drag, GFSynth *synth, const GFSpatialUI *ui,
                              int current_room, GFPortId clicked_port, bool force_stack_new) {
    if (!drag || !synth || (int)clicked_port < 0 || clicked_port >= GF_PORT_COUNT) return false;

    /* If clicking an occupied INPUT jack without stack modifier, unplug the top cable so user can move/remove it */
    if (gf_port_is_input(clicked_port) && !force_stack_new) {
        int found_slot = -1;
        for (int i = GF_MAX_CABLES - 1; i >= 0; --i) {
            if (synth->patchbay.cables[i].active && synth->patchbay.cables[i].dst == clicked_port) {
                found_slot = i;
                break;
            }
        }
        if (found_slot >= 0) {
            GFCable c = synth->patchbay.cables[found_slot];
            gf_synth_disconnect_cable(synth, c.id);
            drag->active = true;
            drag->anchor_port = c.src;
            drag->gain = c.gain;
            drag->hovered_port = (int)clicked_port;
            GFVec3 jpos = gf_vec3(0, 1.4f, 0);
            if (ui) gf_spatial_port_world_socket(ui, clicked_port, current_room, &jpos, NULL, NULL);
            drag->loose_pos = jpos;
            return true;
        }
    }

    /* Otherwise start a brand-new stackable cable from clicked_port */
    drag->active = true;
    drag->anchor_port = clicked_port;
    drag->gain = gf_port_is_output(clicked_port) &&
                 gf_port_info(clicked_port) &&
                 gf_port_info(clicked_port)->kind == GF_SIG_AUDIO ? 0.85f : 0.65f;
    drag->hovered_port = -1;
    GFVec3 jpos = gf_vec3(0, 1.4f, 0);
    if (ui) gf_spatial_port_world_socket(ui, clicked_port, current_room, &jpos, NULL, NULL);
    drag->loose_pos = jpos;
    return true;
}

void gf_cable_update_drag(GFCableDrag3D *drag, const GFSpatialUI *ui,
                          GFRay ray, float default_dist, int hovered_widget_id) {
    if (!drag || !drag->active) return;
    drag->hovered_port = -1;

    if (ui && hovered_widget_id >= 0) {
        const GFWidget3D *hw = gf_spatial_get_widget_const(ui, hovered_widget_id);
        if (hw && (hw->kind == GF_W3D_JACK_IN || hw->kind == GF_W3D_JACK_OUT) && hw->port_id >= 0) {
            GFPortId hp = (GFPortId)hw->port_id;
            bool compat = (gf_port_is_output(drag->anchor_port) && gf_port_is_input(hp)) ||
                          (gf_port_is_input(drag->anchor_port) && gf_port_is_output(hp));
            if (compat) {
                drag->hovered_port = (int)hp;
                GFVec3 snap_pos;
                if (gf_spatial_port_world_socket(ui, hp, hw->room_id, &snap_pos, NULL, NULL)) {
                    drag->loose_pos = snap_pos;
                    return;
                }
            }
        }
    }

    float d = default_dist > 0.35f ? default_dist : 1.8f;
    drag->loose_pos = gf_vec3_add(ray.origin, gf_vec3_scale(ray.dir, d));
}

int gf_cable_finish_drag(GFCableDrag3D *drag, GFSynth *synth) {
    if (!drag || !drag->active) return 0;
    int connected_id = 0;
    if (synth && drag->hovered_port >= 0 && drag->hovered_port < GF_PORT_COUNT) {
        GFPortId hp = (GFPortId)drag->hovered_port;
        if (gf_port_is_output(drag->anchor_port) && gf_port_is_input(hp)) {
            connected_id = gf_synth_connect_cable(synth, drag->anchor_port, hp, drag->gain);
        } else if (gf_port_is_input(drag->anchor_port) && gf_port_is_output(hp)) {
            connected_id = gf_synth_connect_cable(synth, hp, drag->anchor_port, drag->gain);
        }
    }
    drag->active = false;
    drag->hovered_port = -1;
    return connected_id;
}

int gf_cable_pick_ring(const GFCableBatch3D *cb, GFRay ray, const bool *room_vis,
                       bool touch_inflate, float *out_t) {
    if (!cb) return -1;
    float radius = touch_inflate ? 0.072f : 0.046f;
    GFVec3 he = gf_vec3(radius, radius, radius);
    int best_cid = -1;
    float best_t = 1e30f;

    for (int i = 0; i < cb->ring_count; ++i) {
        const GFCableRing3D *rg = &cb->rings[i];
        if (room_vis && rg->room_id >= 1 && rg->room_id <= 7 && !room_vis[rg->room_id]) continue;
        float t = 0.0f;
        if (gf_ray_intersect_obb(ray, rg->pos, gf_vec3(1, 0, 0), gf_vec3(0, 1, 0), gf_vec3(0, 0, 1), he, &t)) {
            if (t >= 0.0f && t < best_t) {
                best_t = t;
                best_cid = rg->cable_id;
            }
        }
    }
    if (best_cid >= 0 && out_t) *out_t = best_t;
    return best_cid;
}

GFMat4 gf_cable_ring_model_matrix(const GFCableRing3D *ring) {
    if (!ring) return gf_mat4_identity();
    GFVec3 f = gf_vec3_normalize(ring->tangent);
    if (gf_vec3_len(f) < 1e-4f) f = gf_vec3(1, 0, 0);
    GFVec3 up_ref = (fabsf(f.y) > 0.92f) ? gf_vec3(1, 0, 0) : gf_vec3(0, 1, 0);
    GFVec3 r = gf_vec3_normalize(gf_vec3_cross(up_ref, f));
    GFVec3 u = gf_vec3_cross(f, r);

    /* Rotate indicator fin around cable tangent according to bipolar gain [-2..+2] */
    float ang = -(ring->gain / 2.0f) * (0.75f * GF_PI);
    float ca = cosf(ang), sa = sinf(ang);
    GFVec3 nr = gf_vec3_add(gf_vec3_scale(r, ca), gf_vec3_scale(u, sa));
    GFVec3 nu = gf_vec3_add(gf_vec3_scale(r, -sa), gf_vec3_scale(u, ca));

    float s = 0.048f;
    GFMat4 m = gf_mat4_identity();
    m.m[0] = nr.x * s; m.m[1] = nr.y * s; m.m[2]  = nr.z * s;
    m.m[4] = nu.x * s; m.m[5] = nu.y * s; m.m[6]  = nu.z * s;
    m.m[8] = f.x * s;  m.m[9] = f.y * s;  m.m[10] = f.z * s;
    m.m[12] = ring->pos.x; m.m[13] = ring->pos.y; m.m[14] = ring->pos.z;
    return m;
}

static void emit_polyline_tube(const GFVec3 *pts, int pt_count, float radius,
                               float sig_level, GFVec4 col,
                               GFVertex *out_verts, int *v_cnt, int max_verts) {
    if (pt_count < 2) return;
    int needed = (pt_count - 1) * GF_TUBE_SIDES * 6;
    if (*v_cnt + needed > max_verts) return;

    static const float k_cos[GF_TUBE_SIDES] = {1.0f, 0.309017f, -0.809017f, -0.809017f, 0.309017f};
    static const float k_sin[GF_TUBE_SIDES] = {0.0f, 0.951057f,  0.587785f, -0.587785f, -0.951057f};

    GFVec3 ring_pos[2][GF_TUBE_SIDES];
    GFVec3 ring_nrm[2][GF_TUBE_SIDES];

    for (int i = 0; i < pt_count; ++i) {
        GFVec3 tan_dir;
        if (i == 0) tan_dir = gf_vec3_sub(pts[1], pts[0]);
        else if (i == pt_count - 1) tan_dir = gf_vec3_sub(pts[i], pts[i - 1]);
        else tan_dir = gf_vec3_sub(pts[i + 1], pts[i - 1]);
        tan_dir = gf_vec3_normalize(tan_dir);
        if (gf_vec3_len(tan_dir) < 1e-5f) tan_dir = gf_vec3(0, 1, 0);

        GFVec3 up_ref = (fabsf(tan_dir.y) > 0.92f) ? gf_vec3(1, 0, 0) : gf_vec3(0, 1, 0);
        GFVec3 b1 = gf_vec3_normalize(gf_vec3_cross(up_ref, tan_dir));
        GFVec3 b2 = gf_vec3_cross(tan_dir, b1);

        int cur = i & 1;
        for (int s = 0; s < GF_TUBE_SIDES; ++s) {
            GFVec3 n = gf_vec3_add(gf_vec3_scale(b1, k_cos[s]), gf_vec3_scale(b2, k_sin[s]));
            ring_nrm[cur][s] = n;
            ring_pos[cur][s] = gf_vec3_add(pts[i], gf_vec3_scale(n, radius));
        }

        if (i > 0) {
            int prev = (i - 1) & 1;
            float u0 = (float)(i - 1) / (float)(pt_count - 1);
            float u1 = (float)i / (float)(pt_count - 1);
            for (int s = 0; s < GF_TUBE_SIDES; ++s) {
                int ns = (s + 1) % GF_TUBE_SIDES;
                GFVertex v00 = {ring_pos[prev][s].x, ring_pos[prev][s].y, ring_pos[prev][s].z,
                                ring_nrm[prev][s].x, ring_nrm[prev][s].y, ring_nrm[prev][s].z,
                                u0, sig_level, col.x, col.y, col.z, col.w};
                GFVertex v01 = {ring_pos[prev][ns].x, ring_pos[prev][ns].y, ring_pos[prev][ns].z,
                                ring_nrm[prev][ns].x, ring_nrm[prev][ns].y, ring_nrm[prev][ns].z,
                                u0, sig_level, col.x, col.y, col.z, col.w};
                GFVertex v11 = {ring_pos[cur][ns].x, ring_pos[cur][ns].y, ring_pos[cur][ns].z,
                                ring_nrm[cur][ns].x, ring_nrm[cur][ns].y, ring_nrm[cur][ns].z,
                                u1, sig_level, col.x, col.y, col.z, col.w};
                GFVertex v10 = {ring_pos[cur][s].x, ring_pos[cur][s].y, ring_pos[cur][s].z,
                                ring_nrm[cur][s].x, ring_nrm[cur][s].y, ring_nrm[cur][s].z,
                                u1, sig_level, col.x, col.y, col.z, col.w};
                out_verts[(*v_cnt)++] = v00;
                out_verts[(*v_cnt)++] = v01;
                out_verts[(*v_cnt)++] = v11;
                out_verts[(*v_cnt)++] = v00;
                out_verts[(*v_cnt)++] = v11;
                out_verts[(*v_cnt)++] = v10;
            }
        }
    }
}

static void build_catenary_pts(GFVec3 p0, GFVec3 n0, GFVec3 p1, GFVec3 n1,
                               float sag_scale, GFVec3 *out_pts, int segs) {
    float span = gf_vec3_len(gf_vec3_sub(p1, p0));
    float sag = gf_clampf(0.08f + span * 0.24f, 0.06f, 0.75f) * sag_scale;
    float bow = gf_clampf(0.05f + span * 0.08f, 0.04f, 0.18f);

    for (int i = 0; i <= segs; ++i) {
        float t = (float)i / (float)segs;
        GFVec3 base = gf_vec3_lerp(p0, p1, t);
        float bell = 4.0f * t * (1.0f - t);
        GFVec3 n_blend = gf_vec3_lerp(n0, n1, t);
        base = gf_vec3_add(base, gf_vec3_scale(n_blend, bow * sinf(t * GF_PI)));
        base.y -= sag * bell;
        out_pts[i] = base;
    }
}

int gf_cable_build_verts(GFCableBatch3D *cb, const GFPatchPlan *plan,
                         const GFAudioTelemetry *tel, const GFSpatialUI *ui,
                         const GFCableDrag3D *drag, int current_room,
                         const bool *room_vis, GFVertex *out_verts, int max_verts) {
    if (!cb || !ui || !out_verts || max_verts < 60) return 0;
    cb->ring_count = 0;
    int v_cnt = 0;

    if (plan) {
        for (int i = 0; i < plan->cable_count; ++i) {
            const GFCable *c = &plan->cables[i];
            if (!c->active) continue;

            /* Prefer Room 4 trunk jacks when viewing Room 4 if both endpoints exist there, otherwise home jacks */
            GFVec3 p0, n0, p1, n1;
            int r0 = 0, r1 = 0;
            int pref_room = (current_room == 4) ? 4 : 0;
            if (!gf_spatial_port_world_socket(ui, c->src, pref_room, &p0, &n0, &r0)) continue;
            if (!gf_spatial_port_world_socket(ui, c->dst, pref_room, &p1, &n1, &r1)) continue;

            bool vis0 = !room_vis || r0 <= 0 || r0 > 7 || room_vis[r0];
            bool vis1 = !room_vis || r1 <= 0 || r1 > 7 || room_vis[r1];
            if (!vis0 && !vis1) continue;

            float sig = (tel && i < tel->cable_count) ? tel->cable_levels[i] : 0.15f;
            GFVec4 col = cable_color_for(gf_port_info(c->src), c->gain, c->is_feedback);
            if (c->id == cb->active_ring_cable_id || c->id == cb->hovered_ring_cable_id) {
                col = (GFVec4){1.0f, 1.0f, 0.86f, 1.0f};
            }

            if (r0 == r1) {
                /* Intra-room sagging catenary patch cable */
                GFVec3 pts[11];
                build_catenary_pts(p0, n0, p1, n1, 1.0f, pts, 10);
                emit_polyline_tube(pts, 11, 0.013f, sig, col, out_verts, &v_cnt, max_verts);

                if (cb->ring_count < GF_CABLE_MAX_RINGS) {
                    GFCableRing3D *rg = &cb->rings[cb->ring_count++];
                    rg->cable_id = c->id;
                    rg->room_id = r0;
                    rg->pos = pts[5];
                    rg->tangent = gf_vec3_sub(pts[6], pts[4]);
                    rg->gain = c->gain;
                    rg->color = col;
                }
            } else {
                /* Cross-room cable routed cleanly through overhead glowing ceiling conduits (Y = 3.32m) */
                const GFRoomDef *rm0 = gf_sanctum_room_def(r0);
                const GFRoomDef *rm1 = gf_sanctum_room_def(r1);
                GFVec3 hub0 = rm0 ? gf_vec3(rm0->center.x * 0.85f + p0.x * 0.15f, 3.32f, rm0->center.z * 0.85f + p0.z * 0.15f)
                                  : gf_vec3(p0.x, 3.32f, p0.z);
                GFVec3 hub1 = rm1 ? gf_vec3(rm1->center.x * 0.85f + p1.x * 0.15f, 3.32f, rm1->center.z * 0.85f + p1.z * 0.15f)
                                  : gf_vec3(p1.x, 3.32f, p1.z);

                GFVec3 pts[13];
                /* Ascend from source jack into room ceiling conduit */
                pts[0] = p0;
                pts[1] = gf_vec3_add(p0, gf_vec3_add(gf_vec3_scale(n0, 0.14f), gf_vec3(0.0f, 0.12f, 0.0f)));
                pts[2] = gf_vec3_lerp(pts[1], hub0, 0.45f);
                pts[3] = gf_vec3_lerp(pts[1], hub0, 0.82f);
                pts[4] = hub0;
                /* Overhead conduit traverse across rooms (offset slightly by cable index so bundles fan out!) */
                float lane = ((float)(i % 7) - 3.0f) * 0.055f;
                pts[5] = gf_vec3_add(gf_vec3_lerp(hub0, hub1, 0.25f), gf_vec3(lane, 0.04f, lane));
                pts[6] = gf_vec3_add(gf_vec3_lerp(hub0, hub1, 0.50f), gf_vec3(lane, 0.06f, lane));
                pts[7] = gf_vec3_add(gf_vec3_lerp(hub0, hub1, 0.75f), gf_vec3(lane, 0.04f, lane));
                pts[8] = hub1;
                /* Descend from destination room ceiling conduit into destination jack */
                GFVec3 pre1 = gf_vec3_add(p1, gf_vec3_add(gf_vec3_scale(n1, 0.14f), gf_vec3(0.0f, 0.12f, 0.0f)));
                pts[9]  = gf_vec3_lerp(hub1, pre1, 0.25f);
                pts[10] = gf_vec3_lerp(hub1, pre1, 0.65f);
                pts[11] = pre1;
                pts[12] = p1;

                emit_polyline_tube(pts, 13, 0.012f, sig, col, out_verts, &v_cnt, max_verts);

                /* Place reachable 3D Attenuverter Rings near BOTH source and destination racks! */
                if (vis0 && cb->ring_count < GF_CABLE_MAX_RINGS) {
                    GFCableRing3D *rg = &cb->rings[cb->ring_count++];
                    rg->cable_id = c->id;
                    rg->room_id = r0;
                    rg->pos = pts[2];
                    rg->tangent = gf_vec3_sub(pts[3], pts[1]);
                    rg->gain = c->gain;
                    rg->color = col;
                }
                if (vis1 && cb->ring_count < GF_CABLE_MAX_RINGS) {
                    GFCableRing3D *rg = &cb->rings[cb->ring_count++];
                    rg->cable_id = c->id;
                    rg->room_id = r1;
                    rg->pos = pts[10];
                    rg->tangent = gf_vec3_sub(pts[11], pts[9]);
                    rg->gain = c->gain;
                    rg->color = col;
                }
            }
        }
    }

    /* Live interactive dragged cable preview */
    if (drag && drag->active) {
        GFVec3 p0 = drag->loose_pos, n0 = gf_vec3(0, 0, 1);
        int r0 = current_room;
        if (gf_spatial_port_world_socket(ui, drag->anchor_port, current_room, &p0, &n0, &r0)) {
            GFVec3 pts[11];
            GFVec3 p_start = gf_port_is_output(drag->anchor_port) ? p0 : drag->loose_pos;
            GFVec3 p_end   = gf_port_is_output(drag->anchor_port) ? drag->loose_pos : p0;
            build_catenary_pts(p_start, n0, p_end, gf_vec3_scale(n0, -1.0f), 0.85f, pts, 10);
            GFVec4 drag_col = (drag->hovered_port >= 0)
                ? (GFVec4){0.55f, 1.00f, 0.80f, 1.00f}
                : (GFVec4){1.00f, 0.92f, 0.45f, 0.95f};
            emit_polyline_tube(pts, 11, 0.015f, 0.9f, drag_col, out_verts, &v_cnt, max_verts);
        }
    }

    return v_cnt;
}
