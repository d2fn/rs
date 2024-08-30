#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "rs_grid.h"
#include "rs_terra.h"
#include "rs_tween.h"
#include "rs_types.h"
#include "rs_render.h"
#include "rs_player.h"
#include "rs_graphics.h"
#include "rs_math.h"

rs_scene* rs_make_scene(rs_screen* s, rs_terra* world, rs_camera* camera, rs_light* light, rs_grid* lightmap, rs_player* player, u8 fps) {
    rs_scene* scene = malloc(sizeof(rs_scene));
    scene->screen = s;
    scene->world = world;
    scene->camera = camera;
    scene->player = player;
    scene->light = light;
    scene->lightmap = lightmap;
    scene->fps = fps;
    scene->frame_num = 0;
    scene->last_millis = 0;
    return scene;
}

rs_screen* rs_make_screen(u32 output_width, u32 output_height, u8 pixel_size) {
    rs_screen* s = malloc(sizeof(rs_screen));
    s->output_width = output_width;
    s->output_height = output_height;
    s->pixel_size = pixel_size;
    s->buf = rs_new_buffer(output_width/pixel_size, output_height/pixel_size);
    s->output_buffer = malloc(s->output_width * s->output_height * sizeof(u32));
    return s;
}

rs_camera* rs_make_camera(float point_at_x, float point_at_y, float fov) {
    rs_camera* camera = malloc(sizeof(rs_camera));
    camera->point_at_x = rs_make_tween(point_at_x);
    camera->point_at_y = rs_make_tween(point_at_y);
    camera->fov = rs_make_tween(fov);
    return camera;
}

void rs_update_camera(rs_camera* camera) {
    rs_update_tween(camera->point_at_x);
    rs_update_tween(camera->point_at_y);
    rs_update_tween(camera->fov);
}

void rs_free_camera(rs_camera* camera) {
    rs_free_tween(camera->point_at_x);
    rs_free_tween(camera->point_at_y);
    rs_free_tween(camera->fov);
    free(camera);
}

rs_light* rs_make_light(float x, float y, float z, float intensity) {
    rs_light* light = malloc(sizeof(rs_light));
    light->x = x;
    light->y = y;
    light->z = z;
    light->intensity = intensity;
    return light;
}

void rs_free_light(rs_light* light) {
    free(light);
}

void rs_update_player(rs_player* player) {
    rs_update_tween(player->map_x);
    rs_update_tween(player->map_y);
}

void rs_update_scene(rs_scene* scene) {
    rs_update_camera(scene->camera);
    rs_update_player(scene->player);
}

void rs_free_screen(rs_screen* s) {
    rs_free_buffer(s->buf);
    free(s->output_buffer);
    free(s);
}

void rs_free_scene(rs_scene* scene) {
    rs_free_screen(scene->screen);
    rs_free_camera(scene->camera);
    rs_free_light(scene->light);
    rs_free_grid(scene->lightmap);
    rs_free_terra(scene->world);
    rs_free_player(scene->player);
    free(scene);
}

// move to a pure function in rs_graphics
u32* rs_capture_output_buffer(rs_screen* s) {
    for (u32 i = 0; i < s->output_width * s->output_height; i++) {
        u32 output_x = i % s->output_width;
        u32 output_y = i / s->output_width;
        u32 input_x = output_x / s->pixel_size;
        u32 input_y = output_y / s->pixel_size;
        u32 j = input_x + input_y * s->buf->width;
        s->output_buffer[i] = s->buf->pixels[j];
    }
    return s->output_buffer;
}

void rs_screen2map(float* world_x, float* world_y, int x, int y, rs_buffer* buf, rs_camera* camera) {

    float point_at_at = rs_tween_poll(camera->point_at_x);
    float point_at_y = rs_tween_poll(camera->point_at_y);
    float fov_x = rs_tween_poll(camera->fov);
    float fov_y = fov_x * ((float)buf->height / (float)buf->width);

    float cell_x1 = point_at_at - fov_x / 2.0;
    float cell_x2 = point_at_at + fov_x / 2.0;
    float cell_y1 = point_at_y - fov_y / 2.0;
    float cell_y2 = point_at_y + fov_y / 2.0;

    float num_x_cells = cell_x2 - cell_x1 + 1;
    float num_y_cells = cell_y2 - cell_y1 + 1;

    float cell_pixels = (float)buf->width / fov_x;
    float pixel_width = (num_x_cells) * cell_pixels;
    float pixel_height = (num_y_cells) * cell_pixels;

    float center_x = buf->width / 2.0;
    float center_y = buf->height / 2.0;
    float screen_world_x1 = center_x - pixel_width / 2.0;
    float screen_world_y1 = center_y - pixel_height / 2.0;
    float screen_world_x2 = center_x + pixel_width / 2.0;
    float screen_world_y2 = center_y + pixel_height / 2.0;

    *world_x = round(cell_x1 + num_x_cells * (((float)x - screen_world_x1) / (screen_world_x2 - screen_world_x1)));
    *world_y = round(cell_y1 + num_y_cells * (((float)y - screen_world_y1) / (screen_world_y2 - screen_world_y1)));
}

void rs_map2screen(float* screen_x, float* screen_y, float map_x, float map_y, rs_buffer* buf, rs_camera* camera) {
    float cell_size = (float)buf->width / rs_tween_poll(camera->fov);
    float center_x = (float)buf->width / 2.0;
    float center_y = (float)buf->height / 2.0;
    float viewport_map_x = rs_tween_poll(camera->point_at_x);
    float viewport_map_y = rs_tween_poll(camera->point_at_y);
    *screen_x = center_x + (map_x - viewport_map_x) * cell_size;
    *screen_y = center_y + (map_y - viewport_map_y) * cell_size;
}

rs_color calc_world_color_bw(rs_grid* world, u32 x_cell, u32 y_cell, u32 frame_num) {
    /*return rs_make_color(255, 255, 255, 255);*/
    /*float map_val = 255 * rs_grid_get(world, x_cell, y_cell);*/
    /*u8 brightness = (u8)((int)floor(map_val));*/
    /*return rs_make_color(255, brightness, brightness, brightness);*/
}

rs_color calc_world_color(rs_grid* world, u32 x_cell, u32 y_cell, u32 frame_num) {

    u8 alpha = 255;

    rs_color deep_water_color = rs_make_color(alpha, 20, 50, 250);
    rs_color shallow_water_color = rs_make_color(alpha, 20, 130, 200);
    rs_color lo_land_color = rs_make_color(alpha, 20, 200, 130);
    rs_color hi_land_color = rs_make_color(alpha, 200, 200, 50);
    rs_color ice_land_color = rs_make_color(alpha, 200, 200, 200);

    u32 pixel_num = x_cell + y_cell * world->width;

    float map_val = rs_grid_get(world, x_cell, y_cell);//world->data[x_cell + y_cell*world->width];
    
    if (map_val <= 100) {
        return rs_make_color(255, 0, 0, (u8)((pixel_num+frame_num)%128));
    }
    else if(map_val <= 110) {
        u32 redmask = (50<<8) | 255;
        return ((pixel_num + frame_num) & redmask) | (alpha<<24);
    }
    else if(map_val <= 135) {
        return lo_land_color;
    }
    else if(map_val <= 140) {
        return hi_land_color;
    }
    
    return ice_land_color;
}

rs_color calc_layer_color(rs_grid* g, u32 x, u32 y) {
    float v = rs_grid_get(g, x, y);
    u8 alpha = 0;
    if (v > 0 && v < 255) {
        alpha = rs_remap(v, -1, 1, 0, 255);
    }
    return rs_make_color(255, alpha, alpha, alpha);
}

rs_color calc_lit_color(rs_color c, float intensity) {
    u8 alpha = (u8)rs_remap(intensity, 0.0, 1.0, 25.0, 255.0);
    u8 r = rs_red(c);
    u8 g = rs_green(c);
    u8 b = rs_blue(c);
    return rs_make_color(alpha, r, g, b);
}

void rs_camera_render_to(rs_scene* scene, rs_grid* world, float x, float y, float width, float height) {

    rs_camera* camera = scene->camera;
    /*rs_grid* world = scene->world->map;*/
    rs_buffer* buf = scene->screen->buf;

    // render map at the given viewport
    float point_at_at = rs_tween_poll(camera->point_at_x);
    float point_at_y = rs_tween_poll(camera->point_at_y);
    float fov_x = rs_tween_poll(camera->fov);
    float fov_y = fov_x * ((float)buf->height / (float)buf->width);

    float cell_x1 = point_at_at - fov_x / 2.0;
    float cell_x2 = point_at_at + fov_x / 2.0;
    float cell_y1 = point_at_y - fov_y / 2.0;
    float cell_y2 = point_at_y + fov_y / 2.0;

    float num_x_cells = cell_x2 - cell_x1 + 1;
    float num_y_cells = cell_y2 - cell_y1 + 1;

    float cell_pixels = (float)buf->width / fov_x;
    float pixel_width = (num_x_cells) * cell_pixels;
    float pixel_height = (num_y_cells) * cell_pixels;

    float center_x = buf->width / 2.0;
    float center_y = buf->height / 2.0;
    float screen_world_x1 = center_x - pixel_width / 2.0;
    float screen_world_y1 = center_y - pixel_height / 2.0;
    float screen_world_x2 = center_x + pixel_width / 2.0;
    float screen_world_y2 = center_y + pixel_height / 2.0;

    u8 alpha = 255;

    rs_color deep_water_color = rs_make_color(alpha, 20, 50, 250);
    rs_color shallow_water_color = rs_make_color(alpha, 20, 130, 200);
    rs_color lo_land_color = rs_make_color(alpha, 20, 200, 130);
    rs_color hi_land_color = rs_make_color(alpha, 200, 200, 50);
    rs_color ice_land_color = rs_make_color(alpha, 200, 200, 200);

    rs_grid* lightmap = scene->lightmap;

    for (int x = 0; x < (int)buf->width; x++) {
        int x_cell = round(cell_x1 + num_x_cells * (((float)x - screen_world_x1) / (screen_world_x2 - screen_world_x1)));
        if (x_cell >= 0 && x_cell < (int)world->width) {
            for (int y = 0; y < (int)buf->height; y++) {
                int pixel_num = buf->width * y + x;
                int y_cell = round(cell_y1 + num_y_cells * (((float)y - screen_world_y1) / (screen_world_y2 - screen_world_y1)));
                float light_intensity = rs_grid_get(lightmap, x_cell, y_cell);
                if (y_cell >= 0 && y_cell < (int)world->height) {
                    int i = x_cell + (y_cell * world->width);
                    if (i >= 0 && i < (int)(world->width * world->height)) {
                        if (world == scene->world->map) {
                            rs_color c = calc_world_color(world, x_cell, y_cell, scene->frame_num);
                            c = calc_lit_color(c, light_intensity);
                            rs_draw_pixel(buf, x, y, c);
                        }
                        else {
                            rs_color c = calc_layer_color(world, x_cell, y_cell);
                            rs_draw_pixel(buf, x, y, c);
                        }
                    }
                }
            }
        }
    }

    rs_player* player = scene->player;
    float player_x, player_y;
    rs_map2screen(&player_x, &player_y, rs_tween_poll(player->map_x), rs_tween_poll(player->map_y), buf, camera);

    int half_player_size = 10;
    rs_vline(buf, player_x, player_y - half_player_size, player_y + half_player_size, rs_make_color(255, 255, 0, 0));
    rs_vline(buf, player_x+1, player_y - half_player_size, player_y + half_player_size, rs_make_color(255, 255, 0, 0));
    rs_hline(buf, player_x - half_player_size, player_x + half_player_size, player_y, rs_make_color(255, 255, 0, 0));
    rs_hline(buf, player_x - half_player_size, player_x + half_player_size, player_y+1, rs_make_color(255, 255, 0, 0));

    rs_light* light = scene->light;
    float z_at_light = rs_grid_get(world, round(light->x), round(light->y));

    if (z_at_light <= light->z) {
        float light_x, light_y;
        rs_map2screen(&light_x, &light_y, light->x, light->y, buf, camera);
        rs_rect(buf, light_x - 10, light_y - 10, light_x + 10, light_y + 10, rs_make_color(255, 220, 220, 0), 1);
    }
}

