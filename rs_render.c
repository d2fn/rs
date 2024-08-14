#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "rs_tween.h"
#include "rs_types.h"
#include "rs_render.h"
#include "rs_map.h"
#include "rs_graphics.h"

float f_bounds_check(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

u32 u32_bounds_check(u32 x, u32 lo, u32 hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

rs_scene* rs_make_scene(rs_screen* s, rs_map* m, rs_map_viewport* v, rs_player* p, u8 fps) {
    rs_scene* scene = malloc(sizeof(rs_scene));
    scene->screen = s;
    scene->map = m;
    scene->viewport = v;
    scene->player = p;
    scene->fps = fps;
    scene->frame_num = 0;
    scene->last_millis = 0;
    return scene;
}

void rs_map_viewport_update(rs_map_viewport* v) {
    rs_tween_update(v->map_x);
    rs_tween_update(v->map_y);
    rs_tween_target(v->span_x, rs_map_viewport_zoom_level[v->zoom_level]);
    rs_tween_update(v->span_x);
}

void rs_scene_update(rs_scene* scene) {
    rs_map_viewport_update(scene->viewport);
}

void rs_free_scene(rs_scene* scene) {
    free(scene);
}

rs_screen* rs_make_screen(u32 output_width, u32 output_height, u8 pixel_size) {
    rs_screen* s = malloc(sizeof(rs_screen));
    s->output_width = output_width;
    s->output_height = output_height;
    s->pixel_size = pixel_size;
    s->width = output_width/pixel_size;
    s->height = output_height/pixel_size;
    s->num_pixels = s->width * s->height;
    s->pixels = malloc(s->num_pixels * sizeof(u32));
    s->output_buffer = malloc(s->output_width * s->output_height * sizeof(u32));
    return s;
}

void rs_free_screen(rs_screen* s) {
    free(s->pixels);
    free(s->output_buffer);
    free(s);
}

rs_map_viewport* rs_make_map_viewport(float map_x, float map_y, u8 zoom_level) {
    rs_map_viewport* v = malloc(sizeof(rs_map_viewport));
    v->map_x = rs_make_tween(map_x);
    v->map_y = rs_make_tween(map_y);
    v->span_x = rs_make_tween((float)rs_map_viewport_zoom_level[zoom_level]);
    v->zoom_level = zoom_level;
    return v;
}

void rs_free_map_viewport(rs_map_viewport* v) {
    rs_free_tween(v->map_x);
    rs_free_tween(v->map_y);
    rs_free_tween(v->span_x);
    free(v);
}

u32 rs_color_pixel(u8 a, u8 r, u8 g, u8 b) {
    u32 pixel = 0;
    pixel |= a << 24;
    pixel |= r << 16;
    pixel |= g << 8;
    pixel |= b;
    return pixel;
}

u32 rs_adjust_gamma(u32 color, float gamma) {
    // Extract the ARGB components
    u8 alpha = (color >> 24) & 0xFF;
    u8 red   = (color >> 16) & 0xFF;
    u8 green = (color >> 8)  & 0xFF;
    u8 blue  = color & 0xFF;

    // Apply gamma correction
    red   = (u8)(pow(red / 255.0, gamma) * 255.0);
    green = (u8)(pow(green / 255.0, gamma) * 255.0);
    blue  = (u8)(pow(blue / 255.0, gamma) * 255.0);

    // Reassemble the color and return it
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

u32 get_x(rs_screen *s, u32 i) {
    return i % s->width;
}

u32 get_y(rs_screen *s, u32 i) {
    return i / s->width;
}

u32* rs_capture_output_buffer(rs_screen* s) {
    for (u32 i = 0; i < s->output_width * s->output_height; i++) {
        u32 output_x = i % s->output_width;
        u32 output_y = i / s->output_width;
        u32 input_x = output_x / s->pixel_size;
        u32 input_y = output_y / s->pixel_size;
        u32 j = input_x + input_y * s->width;
        s->output_buffer[i] = s->pixels[j];
    }
    return s->output_buffer;
}

void rs_set_pixel(rs_screen* s, int x, int y, u32 color) {
    if (x < 0 || x >= (int)s->width) return;
    if (y < 0 || y >= (int)s->height) return;
    int i = x + y * (int)s->width;
    /*if (i < 0 || i >= s->pixel_size) return;*/
    s->pixels[i] = color;
}

void swp(float* a, float* b) {
    /*u32 tmp = *a;*/
    /**a = *b;*/
    /**b = tmp;*/
}

void rs_hline(rs_screen* s, float x1, float x2, float y, u32 color) {
    if (x1 > x2) swp(&x1, &x2);
    for (float x = x1; x <= x2; x++) {
        rs_set_pixel(s, roundf(x), roundf(y), color);
    }
}

void rs_vline(rs_screen* s, float x, float y1, float y2, u32 color) {
    if (y1 > y2) swp(&y1, &y2);
    for (float y = y1; y <= y2; y++) {
        rs_set_pixel(s, roundf(x), roundf(y), color);
    }
}

void rs_rect(rs_screen* s, float x1, float y1, float x2, float y2, u32 color, u8 fill) {
    if (x1 > x2) swp(&x1, &x2);
    if (y1 > y2) swp(&y1, &y2);
    if (fill) {
        for (int x = roundf(x1); x <= x2; x++) {
            for (int y = roundf(y1); y <= y2; y++) {
                rs_set_pixel(s, x, y, color);
            }
        }
    }
    else {
        rs_vline(s, x1, y1, y2, color);
        rs_vline(s, x2, y1, y2, color);
        rs_hline(s, x1, x2, y1, color);
        rs_hline(s, x1, x2, y2, color);
    }
}

void map2screen(float* screen_x, float* screen_y, float map_x, float map_y, rs_screen* screen, rs_map_viewport* v) {
    float cell_size = (float)screen->width / rs_tween_poll(v->span_x);
    float center_x = (float)screen->width / 2.0;
    float center_y = (float)screen->height / 2.0;
    float viewport_map_x = rs_tween_poll(v->map_x);
    float viewport_map_y = rs_tween_poll(v->map_y);
    *screen_x = center_x + (map_x - viewport_map_x) * cell_size;
    *screen_y = center_y + (map_y - viewport_map_y) * cell_size;
}

void calc_map_cell_bounds_for_viewport(u32* ul_map_cell_x, u32* ul_map_cell_y,
                                       u32* lr_map_cell_x, u32* lr_map_cell_y,
                                       rs_screen* s, rs_map* m, rs_map_viewport* v) {

    float viewport_width = rs_tween_poll(v->span_x);
    
    float half_width = viewport_width / 2.0f;
    float half_height = viewport_width / 2.0f * ((float)s->height / (float)s->width);

    float viewport_map_x = rs_tween_poll(v->map_x);
    float viewport_map_y = rs_tween_poll(v->map_y);

    *ul_map_cell_x = floor(viewport_map_x - half_width);
    *ul_map_cell_y = floor(viewport_map_y - half_height);

    *lr_map_cell_x = floor(viewport_map_x + half_width);
    *lr_map_cell_y = floor(viewport_map_y + half_height);
}

void draw_map_cell(rs_screen* screen, rs_map_viewport* v, rs_map* m, u32 map_x, u32 map_y) {

    if (map_x < 0 || map_x >= m->width) return;
    if (map_y < 0 || map_y >= m->height) return;

    float viewport_width = rs_tween_poll(v->span_x);
    float cell_size = (float)screen->width / viewport_width;

    float screen_x, screen_y;
    map2screen(&screen_x, &screen_y, map_x, map_y, screen, v);

    u32 mapdata = rs_get_mapdata(m, map_x, map_y);

    if (cell_size <= 1) {
        rs_set_pixel(screen, screen_x, screen_y, mapdata);
    }
    else {

        rs_rect(screen, screen_x, screen_y, screen_x + cell_size, screen_y + cell_size, mapdata, 1);

        if (cell_size > 5) {
            u32 lighter = rs_adjust_gamma(mapdata, 0.6);
            u32 darker  = rs_adjust_gamma(mapdata, 1.6);

            rs_hline(screen, screen_x, screen_x + cell_size, screen_y, lighter);
            rs_hline(screen, screen_x, screen_x + cell_size, screen_y + cell_size, darker);
            rs_vline(screen, screen_x, screen_y, screen_y + cell_size - 1, lighter);
            rs_vline(screen, screen_x + cell_size - 1, screen_y, screen_y + cell_size - 1, darker);
        }
    }
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
    memset(screen->pixels, 0, screen->num_pixels * sizeof(u32));


    // render map at the given viewport
    rs_map* map = scene->map;
    rs_map_viewport* viewport = scene->viewport;
    float viewport_map_x = rs_tween_poll(viewport->map_x);
    float viewport_map_y = rs_tween_poll(viewport->map_y);
    float viewport_span_x = rs_tween_poll(viewport->span_x);
    float viewport_span_y = viewport_span_x * ((float)screen->height / (float)screen->width);

    float cell_x1 = viewport_map_x - viewport_span_x / 2.0;
    float cell_x2 = viewport_map_x + viewport_span_x / 2.0;
    float cell_y1 = viewport_map_y - viewport_span_y / 2.0;
    float cell_y2 = viewport_map_y + viewport_span_y / 2.0;

    float num_x_cells = cell_x2 - cell_x1 + 1;
    float num_y_cells = cell_y2 - cell_y1 + 1;

    float cell_pixels = (float)screen->width / viewport_span_x;
    float pixel_width = (num_x_cells) * cell_pixels;
    float pixel_height = (num_y_cells) * cell_pixels;

    float center_x = screen->width / 2.0;
    float center_y = screen->height / 2.0;
    float screen_map_x1 = center_x - pixel_width / 2.0;
    float screen_map_y1 = center_y - pixel_height / 2.0;
    float screen_map_x2 = center_x + pixel_width / 2.0;
    float screen_map_y2 = center_y + pixel_height / 2.0;

    for (int x = 0; x < (int)screen->width; x++) {
        int x_cell = round(cell_x1 + num_x_cells * (((float)x - screen_map_x1) / (screen_map_x2 - screen_map_x1)));
        if (x_cell >= 0 && x_cell < (int)map->width) {
            for (int y = 0; y < (int)screen->height; y++) {
                int y_cell = round(cell_y1 + num_y_cells * (((float)y - screen_map_y1) / (screen_map_y2 - screen_map_y1)));
                if (y_cell >= 0 && y_cell < (int)map->height) {
                    int i = x_cell + y_cell*map->width;
                    if (i >= 0 && i < (int)(map->width * map->height)) {
                        rs_set_pixel(screen, x, y, map->mapdata[x_cell + y_cell*map->width]);
                    }
                }
            }
        }
    }


    const rs_player* player = scene->player;

    float player_x, player_y;
    map2screen(&player_x, &player_y, player->map_x, player->map_y, screen, viewport);
    rs_rect(screen, player_x - 10, player_y - 10, player_x + 10, player_y + 10, rs_color_pixel(255, 255, 255, 255), 1);
}

void rs_map_viewport_pan_to(rs_map_viewport* v, rs_map* m, rs_screen* s, float map_x, float map_y) {
    rs_tween_target(v->map_x, map_x);
    rs_tween_target(v->map_y, map_y);
}
