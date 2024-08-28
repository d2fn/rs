#ifndef RS_TERRAIN_H
#define RS_TERRAIN_H

#include "rs_types.h"

rs_terra* rs_build_world(u32 width, u32 height);
void rs_free_terrain(rs_terra* terra);
void diamond_square(float map[], int size, float roughness);
void perlin_fill(rs_grid* g);
void threshold(float map[], int size, float threshold);
void rs_calculate_lighting(rs_grid* lightmap, rs_grid* world, rs_light* light);

#endif

