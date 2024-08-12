#ifndef RS_MAP
#define RS_MAP

#include "rs_types.h"

rs_map* rs_make_map(u32 width, u32 height, u32 start_player_x, u32 start_player_y);
void rs_map_random_fill(rs_map* m);
void rs_map_seq_fill(rs_map* m);
u32 rs_get_mapdata(rs_map* map, u32 x, u32 y);
void rs_set_mapdata(rs_map* map, u32 x, u32 y, u32 value);
void rs_free_map(rs_map* m);

#endif
