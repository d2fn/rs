#ifndef RS_TYPES
#define RS_TYPES

/*#include <cinttypes>*/
#include <stdint.h>
typedef uint32_t u32;
typedef uint8_t u8;

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

    // describe the buffer our renderer will write to directly
    u32 width, height;
    // width * height
    u32 num_pixels;
    // pixel buffer we'll write to directly in the render
    u32* pixels;
    
} rs_screen;

typedef struct {
    u32* mapdata;
    u32  width, height;
    u32  start_player_x;
    u32  start_player_y;
} rs_map;

typedef struct {
    float map_x, map_y;
    float span_x;
} rs_map_viewport;

typedef struct {
    u32 map_x, map_y;
} rs_player;

typedef struct {

    rs_screen* screen;
    rs_map* map;
    rs_map_viewport* viewport;
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
