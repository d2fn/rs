#ifndef RS_TERRAIN_H
#define RS_TERRAIN_H

#include "rs_types.h"

rs_terra* rs_build_world(u32 width, u32 height);
float linterp(float x, float* fx, float* fy, int len);
int find_index(float x, float* fx, int len);
void rs_free_terra(rs_terra* terra);
void diamond_square(float map[], int size, float roughness);
void perlin_fill(rs_grid* g, float scale, float octaves, float persistence, float lacunarity);
void threshold(float map[], int size, float threshold);
void rs_calculate_lighting(rs_grid* lightmap, rs_grid* world, rs_light* light);

#endif

