#ifndef RS_RENDER_H
#define RS_RENDER_H

#include "rs_types.h"

// create a new scene wraper object
rs_scene* rs_make_scene(rs_screen* s, rs_map* m, rs_map_viewport* v, rs_player* p, u8 fps);
void rs_scene_update(rs_scene* scene);
// free the scene but none of its members
void rs_free_scene(rs_scene* scene);
// construct a new screen object with the given output size, resolution/downscaling and target frames per second
rs_screen* rs_make_screen(u32 output_width, u32 output_height, u8 pixel_size);
// free memory associated with a previously constructed screen
void rs_free_screen(rs_screen* s);

rs_map_viewport* rs_make_map_viewport(float map_x, float map_y, u8 zoom_level);

void rs_map_viewport_pan_to(rs_map_viewport* v, rs_map* m, rs_screen* s, float map_x, float map_y);
// free memory associated with viewport
void rs_free_map_viewport(rs_map_viewport* v);
// render the screen at the given tick millis
void rs_render(rs_scene* scene, u32 millis);
// create a new pixel with a given color as AARRGGBB
u32 rs_color_pixel(u8 a, u8 r, u8 g, u8 b);
// copy the internal pixel buffer to the output, scaling up by screen->ixel_size
u32* rs_capture_output_buffer(rs_screen* s);

#endif
