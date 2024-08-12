#include "rs_map.h"
#include "rs_types.h"
#include <stdlib.h>
#include <time.h>

rs_map* rs_make_map(u32 width, u32 height, u32 start_player_x, u32 start_player_y) {
    rs_map* m = malloc(sizeof(rs_map));
    m->mapdata = malloc(width * height * sizeof(u32));
    m->width = width;
    m->height = height;
    m->start_player_x = start_player_x;
    m->start_player_y = start_player_y;
    return m;
}

void rs_map_random_fill(rs_map* m) {
    srand(time(NULL));
    for (u32 i = 0; i < m->width * m->height; i++) {
        m->mapdata[i] = rand() % 5;
    }
}

void rs_map_seq_fill(rs_map* m) {
    for (u32 i = 0; i < m->width * m->height; i++) {
        m->mapdata[i] = i;
    }
}

u32 rs_get_mapdata(rs_map* map, u32 x, u32 y) {
    if (x >= map->width) return 0;
    if (y >= map->height) return 0;
    return map->mapdata[x + y * map->width];
}

void rs_set_mapdata(rs_map* map, u32 x, u32 y, u32 value) {
    if (x >= map->width) return;
    if (y >= map->height) return;
    map->mapdata[x + y * map->width] = value;
}

void rs_free_map(rs_map* m) {
    free(m->mapdata);
    free(m);
}
