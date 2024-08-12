#ifndef RS_PLAYER_H
#define RS_PLAYER_H

#include "rs_types.h"

rs_player* rs_make_player(u32 map_x, u32 map_y);

void rs_free_player(rs_player* p);
 

#endif
