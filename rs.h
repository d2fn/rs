#ifndef RS_H
#define RS_H

#include <stdint.h>
#include <raylib.h>

typedef uint32_t u32;
typedef uint8_t u8;

typedef struct {
    float x, y, z;
    float intensity;
} rs_light;

typedef struct {
    u32 width;
    u32 height;
    u32 size;
    float* data;
} rs_grid;

typedef struct {
    rs_grid* base;
    rs_grid* continentalness;
    rs_grid* erosion;
    rs_grid* map;
} rs_terra;

//
// utility function
//
float linterp(float x, float* fx, float* fy, int len);
float rs_remap(float value, float input_min, float input_max, float output_min, float output_max);

//
// terrain functions
//
rs_terra* rs_build_world(u32 width, u32 height);


//
// rs_grid functions
//
rs_grid* rs_make_grid(u32 width, u32 height);
void rs_grid_norm(rs_grid* g, float lo, float hi);
void rs_grid_fill(rs_grid* g, float value);
void rs_grid_fill(rs_grid* g, float value);
float rs_grid_get(rs_grid* g, u32 x, u32 y);
void rs_grid_set(rs_grid* g, u32 x, u32 y, float value);
void rs_free_grid(rs_grid* g);

//
// rs_light functions
//
void rs_calculate_lighting(rs_grid* lightmap, rs_grid* world, rs_light* light);
rs_light* rs_make_light(float x, float y, float z, float intensity);

//
// rendering and coloring functions
//
Color calc_world_color(rs_grid* world, u32 x_cell, u32 y_cell, u32 frame_num);
Color calc_lit_color(Color c, float intensity);

#endif
