#include "rs_player.h"
#include "rs_types.h"
#include "rs_tween.h"
#include <stdlib.h>

rs_player* rs_make_player(float map_x, float map_y) {
    rs_player* p = malloc(sizeof(rs_player));
    p->map_x = rs_make_tween(map_x);
    p->map_y = rs_make_tween(map_x);
    return p;
}

void rs_free_player(rs_player* p) {
    free(p->map_x);
    free(p->map_y);
    free(p);
}
