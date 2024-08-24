#ifndef RS_RENDER_H
#define RS_RENDER_H

#include "rs_types.h"

// create a new scene wraper object
rs_scene* rs_make_scene(rs_screen* s, rs_grid* world, rs_camera* camera, rs_light* light, rs_grid* lightmap, rs_player* player, u8 fps);
void rs_update_scene(rs_scene* scene);
void rs_free_scene(rs_scene* scene);

// construct a new screen object with the given output size, resolution/downscaling and target frames per second
rs_screen* rs_make_screen(u32 output_width, u32 output_height, u8 pixel_size);

// construct a new camera perspective
rs_camera* rs_make_camera(float point_at_x, float point_at_y, float fov);
void rs_camera_render_to(rs_scene* scene, float x, float y, float width, float height);
void rs_free_camera(rs_camera* camera);

rs_light* rs_make_light(float x, float y, float z, float intensity);
void rs_free_light(rs_light* light);

// render the screen at the given tick millis
void rs_render(rs_scene* scene, u32 millis);
// copy the internal pixel buffer to the output, scaling up by screen->ixel_size
u32* rs_capture_output_buffer(rs_screen* s);


#endif
