#ifndef GF_RENDER_CABLE_RENDERER_H
#define GF_RENDER_CABLE_RENDERER_H

#include "assets/loaders.h"
#include "core/math3d.h"
#include "dsp/patchbay.h"
#include "dsp/synth.h"
#include "render/spatial_ui.h"

#include <stdbool.h>

#define GF_CABLE_MAX_RINGS 256
#define GF_CABLE_MAX_VERTS 49152

typedef struct GFCableRing3D {
    int cable_id;
    int room_id;
    GFVec3 pos;
    GFVec3 tangent;
    float gain;
    GFVec4 color;
} GFCableRing3D;

typedef struct GFCableDrag3D {
    bool active;
    GFPortId anchor_port;
    float gain;
    GFVec3 loose_pos;
    int hovered_port; /* GFPortId or -1 */
} GFCableDrag3D;

typedef struct GFCableBatch3D {
    GFCableRing3D rings[GF_CABLE_MAX_RINGS];
    int ring_count;
    int active_ring_cable_id;
    int hovered_ring_cable_id;
} GFCableBatch3D;

void gf_cable_drag_init(GFCableDrag3D *drag);
void gf_cable_batch_init(GFCableBatch3D *cb);

/* Starts dragging a new cable from an output/empty jack, or unplugs & grabs an existing cable from an occupied input jack. */
bool gf_cable_begin_from_jack(GFCableDrag3D *drag, GFSynth *synth, const GFSpatialUI *ui,
                              int current_room, GFPortId clicked_port, bool force_stack_new);

/* Updates loose 3D plug tip along pointer/XR ray and snaps to compatible hovered jacks. */
void gf_cable_update_drag(GFCableDrag3D *drag, const GFSpatialUI *ui,
                          GFRay ray, float default_dist, int hovered_widget_id);

/* Completes or cancels a 3D cable drag (returns newly connected cable_id, or 0 if unplugged/canceled). */
int gf_cable_finish_drag(GFCableDrag3D *drag, GFSynth *synth);

/* Ray-picks draggable mid-cable 3D attenuverter rings; returns cable_id or -1. */
int gf_cable_pick_ring(const GFCableBatch3D *cb, GFRay ray, const bool *room_vis,
                       bool touch_inflate, float *out_t);

/* Computes world transform for rendering `models/attenuverter_ring.obj` on a cable. */
GFMat4 gf_cable_ring_model_matrix(const GFCableRing3D *ring);

/* Builds batched 3D catenary & overhead cross-room conduit tube triangles + updates ring positions. */
int gf_cable_build_verts(GFCableBatch3D *cb, const GFPatchPlan *plan,
                         const GFAudioTelemetry *tel, const GFSpatialUI *ui,
                         const GFCableDrag3D *drag, int current_room,
                         const bool *room_vis, GFVertex *out_verts, int max_verts);

#endif
