#ifndef RS_GRID
#define RS_GRID

#include "rs_types.h"

rs_grid* rs_make_grid(u32 width, u32 height);
void rs_grid_random_fill(rs_grid* g);
void rs_grid_seq_fill(rs_grid* g);
void rs_grid_fill(rs_grid* g, float value);
void rs_grid_norm(rs_grid* g, float lo, float hi);
float rs_grid_get(rs_grid* g, u32 x, u32 y);
void rs_grid_set(rs_grid* g, u32 x, u32 y, float value);
void rs_free_grid(rs_grid* g);

#endif
