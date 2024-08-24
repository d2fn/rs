#ifndef RS_TYPES
#define RS_TYPES

#include <stdint.h>
#include "rs_graphics.h"

typedef uint32_t u32;
typedef uint8_t u8;

typedef struct {
    float value;
    float vel;
    float accel;
    float force;
    float mass;
    float damping;
    float attraction;
    u8 targeting;
    float target;
    float prev;
} rs_tween;

typedef struct {

    // output buffer size (window size)
    u32 output_width, output_height;
    // output pixel buffer -- don't write to this directly
    // only written when copying out internal buffer to the
    // output and scaling up
    u32* output_buffer;
    // single pixels are quite small on modern displays
    // so our logical pixels are scaled up by this factor
    u8 pixel_size;

    rs_buffer* buf;
} rs_screen;

typedef struct {
    int* mapdata;
    u32  width, height;
    u32  start_player_x;
    u32  start_player_y;
} rs_map;

#define VIEWPORT_MAX_ZOOM	7
extern const u32 rs_map_viewport_zoom_level[];

typedef struct {
    rs_tween* point_at_x;
    rs_tween* point_at_y;
    rs_tween* fov;
} rs_camera;

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
    rs_tween* map_x;
    rs_tween* map_y;
} rs_player;

typedef struct {
    rs_screen* screen;
    rs_grid* world;
    rs_camera* camera;
    rs_light* light;
    rs_grid* lightmap;
    rs_player* player;

    // frames per second
    u8 fps;
    // how many frames have elapsed, including the current one
    u32 frame_num;
    // the last recorded number of elapsed milliseconds since the program started
    // used to control frame rate
    u32 last_millis;
} rs_scene;

#endif
