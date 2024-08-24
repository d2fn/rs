#include "rs_grid.h"
#include "rs_math.h"
#include "rs_types.h"
#include "rs_terrain.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <float.h>

rs_grid* rs_make_grid(u32 width, u32 height) {
    rs_grid* g = malloc(sizeof(rs_grid));
    g->width = width;
    g->height = height;
    g->size = width * height;
    g->data = malloc(g->size * sizeof(float));
    memset(g->data, 0, g->size * sizeof(int));
    return g;
}

void rs_free_grid(rs_grid* g) {
    free(g->data);
    free(g);
}

void rs_grid_random_fill(rs_grid* g) {
    srand(time(NULL));
    for (u32 i = 0; i < g->width * g->height; i++) {
        g->data[i] = rand() % 5;
    }
}

void rs_grid_seq_fill(rs_grid* g) {
    for (u32 i = 0; i < g->width * g->height; i++) {
        g->data[i] = i;
    }
}

void rs_grid_make_terrain(rs_grid* g) {
    diamond_square(g->data, g->width, 128.0f);
    rs_grid_norm(g);
}

void rs_grid_norm(rs_grid* g) {
    float min = FLT_MAX;
    float max = FLT_MIN;
    for (u32 i = 0; i < g->size; i++) {
        float d = g->data[i];
        if (d > max) max = d;
        if (d < min) min = d;
    }
    for (u32 i = 0; i < g->size; i++) {
        g->data[i] = rs_remap(g->data[i], min, max, 0.0, 1.0);
    }
}

float rs_grid_get(rs_grid* g, u32 x, u32 y) {
    if (x >= g->width) return 0;
    if (y >= g->height) return 0;
    return g->data[x + (y * g->width)];
}

void rs_grid_set(rs_grid* g, u32 x, u32 y, float value) {
    if (x >= g->width) return;
    if (y >= g->height) return;
    g->data[x + y * g->width] = value;
}
