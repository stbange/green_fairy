#ifndef GF_RENDER_SPATIAL_UI_H
#define GF_RENDER_SPATIAL_UI_H

#include "assets/loaders.h"
#include "core/math3d.h"
#include "dsp/synth.h"

#include <stdbool.h>
#include <stdint.h>

#define GF_MAX_RACKS_3D      48
#define GF_MAX_WIDGETS_3D    1024
#define GF_MAX_CURVES_3D     24
#define GF_CURVE_MAX_PTS     128

typedef struct GFRay {
    GFVec3 origin;
    GFVec3 dir;
} GFRay;

typedef enum GFWidgetKind3D {
    GF_W3D_RACK_HEADER  = 1,
    GF_W3D_KNOB         = 2,
    GF_W3D_FADER        = 3,
    GF_W3D_SWITCH       = 4,
    GF_W3D_STEP_DIAL    = 5,
    GF_W3D_JACK_OUT     = 6,
    GF_W3D_JACK_IN      = 7,
    GF_W3D_ENV_NODE     = 8,
    GF_W3D_HARMONIC_BAR = 9,
    GF_W3D_XYZ_ORB      = 10,
    GF_W3D_PIANO_KEY    = 11,
    GF_W3D_BUTTON       = 12
} GFWidgetKind3D;

typedef struct GFRack3D {
    int id;
    bool active;
    int room_id;
    char title[48];
    GFVec3 pos;         /* World center of rack front panel */
    float rot_y;        /* Yaw in radians */
    float tilt_x;       /* Pitch tilt in radians (negative tilts panel back like console) */
    float width;
    float height;
    float depth;
    GFVec4 theme_color;
} GFRack3D;

typedef struct GFWidgetDesc3D {
    int room_id;
    int rack_id;            /* >= 0 mounts on rack local XY; < 0 uses local_pos as world_pos */
    GFWidgetKind3D kind;
    GFVec3 local_pos;       /* Rack-local (lx, ly, lz) in meters */
    GFVec2 size;            /* Visual width, height (or diameter) in meters */
    float track_len;        /* Travel length for GF_W3D_FADER / GF_W3D_HARMONIC_BAR */
    int param_id;           /* GFParamId or -1 */
    int port_id;            /* GFPortId or -1 */
    int aux_idx;            /* Env breakpoint idx, harmonic idx, MIDI note, preset idx, etc. */
    int step_count;         /* Discrete step count for GF_W3D_STEP_DIAL */
    float min_val;
    float max_val;
    float default_val;
    GFVec4 color;
    const char *label;
    const char *action_tag; /* Optional Lua action identifier */
} GFWidgetDesc3D;

typedef struct GFWidget3D {
    int id;
    bool active;
    bool visible;
    int room_id;
    int rack_id;
    GFWidgetKind3D kind;

    GFVec3 local_pos;
    GFVec2 size;
    float track_len;

    /* Cached world OBB for ray-picking & rendering */
    GFVec3 world_pos;
    GFVec3 axis_r;
    GFVec3 axis_u;
    GFVec3 axis_f;
    GFVec3 half_extents;

    int param_id;
    int port_id;
    int aux_idx;
    int step_count;
    float min_val;
    float max_val;
    float default_val;
    float norm_val;         /* [0..1] live state */

    GFVec4 color;
    char label[28];
    char action_tag[40];
} GFWidget3D;

typedef struct GFCurveDisplay3D {
    bool active;
    int room_id;
    int rack_id;
    float cx, cy, w, h;     /* Rack-local rectangle */
    bool is_bars;
    GFVec4 color;
    int count;
    float pts_x[GF_CURVE_MAX_PTS]; /* Normalized [0..1] X */
    float pts_y[GF_CURVE_MAX_PTS]; /* Normalized [0..1] Y */
} GFCurveDisplay3D;

typedef struct GFSpatialUI {
    GFRack3D racks[GF_MAX_RACKS_3D];
    int rack_count;

    GFWidget3D widgets[GF_MAX_WIDGETS_3D];
    int widget_count;

    /* Primary jack widget index per GFPortId (-1 if unplaced) */
    int primary_port_widget[GF_PORT_COUNT];

    GFCurveDisplay3D curves[GF_MAX_CURVES_3D];

    int hovered_widget_id;
    int active_widget_id;
    char tooltip_text[64];
    GFVec3 tooltip_pos;
    bool tooltip_visible;
} GFSpatialUI;

void gf_spatial_init(GFSpatialUI *ui);
void gf_spatial_clear(GFSpatialUI *ui);

/* Rack registration & coordinate frame helpers */
int gf_spatial_add_rack(GFSpatialUI *ui, int room_id, const char *title,
                        GFVec3 pos, float rot_y, float tilt_x,
                        float width, float height, GFVec4 theme_color);
const GFRack3D *gf_spatial_get_rack(const GFSpatialUI *ui, int rack_id);
void gf_rack_basis(const GFRack3D *rack, GFVec3 *out_r, GFVec3 *out_u, GFVec3 *out_f);
GFVec3 gf_rack_local_to_world(const GFRack3D *rack, GFVec3 local_pt);
GFMat4 gf_rack_model_matrix(const GFRack3D *rack);

/* 3D Widget registration, geometry & DSP state sync */
int gf_spatial_add_widget(GFSpatialUI *ui, const GFWidgetDesc3D *desc);
GFWidget3D *gf_spatial_get_widget(GFSpatialUI *ui, int widget_id);
const GFWidget3D *gf_spatial_get_widget_const(const GFSpatialUI *ui, int widget_id);
void gf_spatial_update_widget_bounds(GFSpatialUI *ui, GFWidget3D *w);

float gf_widget_value_from_norm(const GFWidget3D *w);
void gf_widget_set_real_value(GFWidget3D *w, float real_val);
void gf_spatial_sync_from_synth(GFSpatialUI *ui, const GFSynth *synth);

/* Computes exact world transform & mesh asset path for rendering a 3D widget */
const char *gf_spatial_widget_mesh_path(GFWidgetKind3D kind);
GFMat4 gf_spatial_widget_model_matrix(const GFSpatialUI *ui, const GFWidget3D *w);

/* Port jack world-space lookup (returns primary jack or room-local trunk jack) */
bool gf_spatial_port_world_socket(const GFSpatialUI *ui, GFPortId port, int preferred_room,
                                  GFVec3 *out_pos, GFVec3 *out_normal, int *out_room);

/* 3D Ray construction & OBB ray-picking */
GFRay gf_ray_from_screen(float screen_x, float screen_y, float vp_w, float vp_h,
                         const GFMat4 *view, const GFMat4 *proj);
bool gf_ray_intersect_obb(GFRay ray, GFVec3 center,
                          GFVec3 axis_r, GFVec3 axis_u, GFVec3 axis_f,
                          GFVec3 half_extents, float *out_t);
int gf_spatial_pick(const GFSpatialUI *ui, GFRay ray, const bool *room_vis,
                    bool touch_inflate, float *out_t);

/* Projects a 3D ray onto a rack's local XY plane (used for dragging 3D env nodes, faders, XYZ) */
bool gf_spatial_ray_to_rack_local(const GFSpatialUI *ui, int rack_id, GFRay ray, GFVec2 *out_local_xy);

/* 3D Holographic Curve/Bar Sculptures & Diegetic World-Space Bitmap Text */
bool gf_spatial_set_curve_xy(GFSpatialUI *ui, int slot, int rack_id,
                             float cx, float cy, float w, float h,
                             const float *xs, const float *ys, int count,
                             bool is_bars, GFVec4 color);
int gf_spatial_build_curve_verts(const GFSpatialUI *ui, const bool *room_vis,
                                 GFVertex *out_verts, int max_verts);
int gf_spatial_build_world_text_verts(const GFSpatialUI *ui, const GFFontMetrics *font,
                                      const bool *room_vis, GFVertex *out_verts, int max_verts);

#endif
