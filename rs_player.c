#include "rs_player.h"
#include "rs_types.h"
#include <stdlib.h>

rs_player* rs_make_player(u32 map_x, u32 map_y) {
    rs_player* p = malloc(sizeof(rs_player));
    p->map_x = map_x;
    p->map_y = map_y;
    return p;
}

void rs_free_player(rs_player* p) {
    free(p);
}
