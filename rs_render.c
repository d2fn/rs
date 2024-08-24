#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "rs_grid.h"
#include "rs_tween.h"
#include "rs_types.h"
#include "rs_render.h"
#include "rs_player.h"
#include "rs_graphics.h"
#include "rs_math.h"

rs_scene* rs_make_scene(rs_screen* s, rs_grid* world, rs_camera* camera, rs_light* light, rs_grid* lightmap, rs_player* player, u8 fps) {
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
    rs_free_grid(scene->world);
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

void map2screen(float* screen_x, float* screen_y, float map_x, float map_y, rs_buffer* buf, rs_camera* camera) {
    float cell_size = (float)buf->width / rs_tween_poll(camera->fov);
    float center_x = (float)buf->width / 2.0;
    float center_y = (float)buf->height / 2.0;
    float viewport_map_x = rs_tween_poll(camera->point_at_x);
    float viewport_map_y = rs_tween_poll(camera->point_at_y);
    *screen_x = center_x + (map_x - viewport_map_x) * cell_size;
    *screen_y = center_y + (map_y - viewport_map_y) * cell_size;
}

void rs_render(rs_scene* scene, u32 millis) {
    if ((millis - scene->last_millis) <= (1000/scene->fps)) {
        return;
    }
    rs_screen* screen = scene->screen;
    scene->last_millis = millis;
    scene->frame_num++;
    if ((scene->frame_num % 100) == 0) {
        printf("frame number: %d\n", scene->frame_num);
    }

    // clear screen buffer
    memset(screen->buf->pixels, 0, screen->buf->num_pixels * sizeof(u32));

    // render map at the given viewport
    rs_grid* world = scene->world;
    rs_camera* camera = scene->camera;
    float viewport_map_x = rs_tween_poll(camera->point_at_x);
    float viewport_map_y = rs_tween_poll(camera->point_at_y);
    float viewport_span_x = rs_tween_poll(camera->fov);
    float viewport_span_y = viewport_span_x * ((float)screen->buf->height / (float)screen->buf->width);

    float cell_x1 = viewport_map_x - viewport_span_x / 2.0;
    float cell_x2 = viewport_map_x + viewport_span_x / 2.0;
    float cell_y1 = viewport_map_y - viewport_span_y / 2.0;
    float cell_y2 = viewport_map_y + viewport_span_y / 2.0;

    float num_x_cells = cell_x2 - cell_x1 + 1;
    float num_y_cells = cell_y2 - cell_y1 + 1;

    float cell_pixels = (float)screen->buf->width / viewport_span_x;
    float pixel_width = (num_x_cells) * cell_pixels;
    float pixel_height = (num_y_cells) * cell_pixels;

    float center_x = screen->buf->width / 2.0;
    float center_y = screen->buf->height / 2.0;
    float screen_map_x1 = center_x - pixel_width / 2.0;
    float screen_map_y1 = center_y - pixel_height / 2.0;
    float screen_map_x2 = center_x + pixel_width / 2.0;
    float screen_map_y2 = center_y + pixel_height / 2.0;

    u8 alpha = 255;

    rs_color deep_water_color = rs_make_color(alpha, 20, 50, 250);
    rs_color shallow_water_color = rs_make_color(alpha, 20, 130, 200);
    rs_color lo_land_color = rs_make_color(alpha, 20, 200, 130);
    rs_color hi_land_color = rs_make_color(alpha, 200, 200, 50);
    rs_color ice_land_color = rs_make_color(alpha, 200, 200, 200);

    for (int x = 0; x < (int)screen->buf->width; x++) {
        int x_cell = round(cell_x1 + num_x_cells * (((float)x - screen_map_x1) / (screen_map_x2 - screen_map_x1)));
        if (x_cell >= 0 && x_cell < (int)world->width) {
            for (int y = 0; y < (int)screen->buf->height; y++) {
                int pixel_num = screen->buf->width * y + x;
                int y_cell = round(cell_y1 + num_y_cells * (((float)y - screen_map_y1) / (screen_map_y2 - screen_map_y1)));
                if (y_cell >= 0 && y_cell < (int)world->height) {
                    int i = x_cell + (y_cell * world->width);
                    if (i >= 0 && i < (int)(world->width * world->height)) {
                        int map_val = world->data[x_cell + y_cell*world->width];
                        rs_color c;
                        if (map_val <= 100) {
                            c = rs_make_color(255, 0, 0, (u8)((pixel_num+scene->frame_num)%128));
                        }
                        else if(map_val <= 110) {
                            u32 redmask = (50<<8) | 255;
                            c = ((pixel_num + scene->frame_num) & redmask) | (255<<24);
                        }
                        else if(map_val <= 135) {
                            c = lo_land_color;
                        }
                        else if(map_val <= 140) {
                            c = hi_land_color;
                        }
                        else {
                            c = ice_land_color;
                        }
                        rs_draw_pixel(screen->buf, x, y, c);
                    }
                }
            }
        }
    }

    for(u32 x = 0; x < screen->buf->width; x += 15) {
        for(u32 y = 0; y < screen->buf->height; y += 15) {
            float weight = rs_remap(x * y, 0, screen->buf->width * screen->buf->height, 1, 4);
            rs_line(screen->buf, (float)x, (float)y, x + 10, y + 10, rs_make_color(200, 100, 200, 100), weight);
        }
    }

    /*rs_draw_contour_lines(screen->buf, world->mapdata, (int)world->width, (int)world->height, rs_make_color(150, 255, 255, 255));*/


    rs_player* player = scene->player;

    float player_x, player_y;
    map2screen(&player_x, &player_y, rs_tween_poll(player->map_x), rs_tween_poll(player->map_y), screen->buf, scene->camera);
    rs_rect(screen->buf, player_x - 10, player_y - 10, player_x + 10, player_y + 10, rs_make_color(255, 255, 255, 255), 1);
}

rs_color calc_lit_color(rs_color c, float intensity) {

    if (intensity > 0.5) {
        // light
        intensity = 2.0 * (intensity - 0.5);
        u8 alpha = (u8)round(intensity * 255);
        return rs_make_color(alpha, rs_red(c), rs_green(c), rs_blue(c));
    }

    return rs_make_color(255, 0, 0, 0);

    /*float alpha = 255 - rs_remap(intensity, -1, 0.5, 0, 255);*/
    /*return rs_make_color((u8)round(alpha), 0, 0, 255);*/
}

void rs_camera_render_to(rs_scene* scene, float x, float y, float width, float height) {

    rs_camera* camera = scene->camera;
    rs_grid* world = scene->world;
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
                        float map_val = world->data[x_cell + y_cell*world->width];
                        //rs_color c = rs_make_color(255, round(255 * map_val), round(255 * map_val), round(255 * map_val));
                        /*rs_draw_pixel(buf, x, y, c);*/
                        rs_draw_pixel(buf, x, y, calc_lit_color(rs_make_color(255, 255, 255, 255), light_intensity));
                        /*
                        rs_color c;
                        if (map_val <= 0.1) {
                            c = rs_make_color(255, 0, 0, (u8)((pixel_num+scene->frame_num)%128));
                        }
                        else if(map_val <= 0.11) {
                            u32 redmask = (50<<8) | 255;
                            c = ((pixel_num + scene->frame_num) & redmask) | (alpha<<24);
                        }
                        else if(map_val <= 0.135) {
                            c = lo_land_color;
                        }
                        else if(map_val <= 0.140) {
                            c = hi_land_color;
                        }
                        else {
                            c = ice_land_color;
                        }
                        rs_draw_pixel(buf, x, y, c);
                        rs_draw_pixel(buf, x, y, calc_light_color(light_intensity));
                        */
                    }
                }
            }
        }
    }

    rs_player* player = scene->player;
    float player_x, player_y;
    map2screen(&player_x, &player_y, rs_tween_poll(player->map_x), rs_tween_poll(player->map_y), buf, camera);
    int half_player_size = 10;

    rs_vline(buf, player_x, player_y - half_player_size, player_y + half_player_size, rs_make_color(255, 255, 0, 0));
    rs_vline(buf, player_x+1, player_y - half_player_size, player_y + half_player_size, rs_make_color(255, 255, 0, 0));
    rs_hline(buf, player_x - half_player_size, player_x + half_player_size, player_y, rs_make_color(255, 255, 0, 0));
    rs_hline(buf, player_x - half_player_size, player_x + half_player_size, player_y+1, rs_make_color(255, 255, 0, 0));

    rs_light* light = scene->light;
    float z_at_light = rs_grid_get(world, round(light->x), round(light->y));

    if (z_at_light <= light->z) {
        float light_x, light_y;
        map2screen(&light_x, &light_y, light->x, light->y, buf, camera);
        //rs_rect(buf, light_x - 10, light_y - 10, light_x + 10, light_y + 10, rs_make_color(255, 255, 255, 255), 1);
    }
}

