#ifndef RS_TERRAIN_H
#define RS_TERRAIN_H

#include "rs_types.h"

void diamond_square(float map[], int size, float roughness);
void threshold(float map[], int size, float threshold);
void rs_calculate_lighting(rs_grid* lightmap, rs_grid* world, rs_light* light);

#endif

