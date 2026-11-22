#ifndef GF_RENDER_VOXEL_MESHER_H
#define GF_RENDER_VOXEL_MESHER_H

#include "assets/loaders.h"

#include <stdbool.h>

/* Converts an external voxel volume into a neighbor face-culled indexed triangle mesh. */
bool gf_voxel_build_mesh(const GFVoxelGrid *grid, float voxel_size, GFMeshData *out_mesh);

#endif
