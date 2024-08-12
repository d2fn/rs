#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "rs_render.h"
#include "rs_map.h"
#include "rs_types.h"

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

rs_map_viewport* rs_make_map_viewport(float map_x, float map_y, float span_x) {
    rs_map_viewport* v = malloc(sizeof(rs_map_viewport));
    v->map_x = map_x;
    v->map_y = map_y;
    v->span_x = span_x;
    return v;
}

void rs_free_map_viewport(rs_map_viewport* v) {
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
    red   = (uint8_t)(pow(red / 255.0, gamma) * 255.0);
    green = (uint8_t)(pow(green / 255.0, gamma) * 255.0);
    blue  = (uint8_t)(pow(blue / 255.0, gamma) * 255.0);

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

void rs_set_pixel(rs_screen* s, u32 x, u32 y, u32 color) {
    if (x >= s->width) return;
    if (y >= s->height) return;
    s->pixels[x + y * s->width] = color;
}

void swp(u32* a, u32* b) {
    /*u32 tmp = *a;*/
    /**a = *b;*/
    /**b = tmp;*/
}

void rs_hline(rs_screen* s, u32 x1, u32 x2, u32 y, u32 color) {
    if (x1 > x2) swp(&x1, &x2);
    for (u32 x = x1; x <= x2; x++) {
        rs_set_pixel(s, x, y, color);
    }
}

void rs_vline(rs_screen* s, u32 x, u32 y1, u32 y2, u32 color) {
    if (y1 > y2) swp(&y1, &y2);
    for (u32 y = y1; y <= y2; y++) {
        rs_set_pixel(s, x, y, color);
    }
}

void rs_rect(rs_screen* s, u32 x1, u32 y1, u32 x2, u32 y2, u32 color, u8 fill) {
    if (x1 > x2) swp(&x1, &x2);
    if (y1 > y2) swp(&y1, &y2);
    if (fill) {
        for (u32 x = x1; x <= x2; x++) {
            for (u32 y = y1; y <= y2; y++) {
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

void map2screen(u32* screen_x, u32* screen_y, float map_x, float map_y, rs_screen* screen, rs_map_viewport* v) {
    float cell_size = (float)screen->width / (float)v->span_x;
    u32 center_x = screen->width / 2;
    u32 center_y = screen->height / 2;
    *screen_x = center_x + (map_x - v->map_x) * cell_size;
    *screen_y = center_y + (map_y - v->map_y) * cell_size;
}

void calc_map_cell_bounds_for_viewport(u32* ul_map_cell_x, u32* ul_map_cell_y,
                                       u32* lr_map_cell_x, u32* lr_map_cell_y,
                                       rs_screen* s,
                                       rs_map_viewport* v) {
    
    float half_width = v->span_x / 2.0f;
    float half_height = v->span_x / 2.0f * ((float)s->height / (float)s->width);

    *ul_map_cell_x = floor(v->map_x - half_width);
    *ul_map_cell_y = floor(v->map_y - half_height);

    *lr_map_cell_x = floor(v->map_x + half_width);
    *lr_map_cell_y = floor(v->map_y + half_height);
}

void draw_map_cell(rs_screen* screen, rs_map_viewport* v, rs_map* m, u32 map_x, u32 map_y) {
    u32 screen_x, screen_y;
    map2screen(&screen_x, &screen_y, map_x, map_y, screen, v);
    u32 mapdata = rs_get_mapdata(m, map_x, map_y);
    rs_set_pixel(screen, screen_x, screen_y, mapdata);
    float cell_size = (float)screen->width / (float)v->span_x;
    rs_rect(screen, screen_x, screen_y, screen_x + cell_size, screen_y + cell_size, mapdata, 1);

    u32 lighter = rs_adjust_gamma(mapdata, 0.6);
    u32 darker  = rs_adjust_gamma(mapdata, 1.6);

    rs_hline(screen, screen_x, screen_x + cell_size, screen_y, lighter);
    rs_hline(screen, screen_x, screen_x + cell_size - 1, screen_y + cell_size - 1, darker);
    rs_vline(screen, screen_x, screen_y, screen_y + cell_size, lighter);
    rs_vline(screen, screen_x + cell_size, screen_y, screen_y + cell_size - 1, darker);
    /*u32 i = screen_x + screen_y * screen->width;*/
    /*if (i < 0 || i >= (screen->num_pixels-1)) return;*/
    /*screen->pixels[i] = m->mapdata[map_x + map_y*m->width];*/
    /*screen->pixels[i+1] = rs_color_pixel(255, 255, 88, 14);*/
}


void rs_render(rs_scene* scene, u32 millis) {
    if ((millis - scene->last_millis) <= (1000/scene->fps)) {
        return;
    }
    rs_screen* screen = scene->screen;
    for (u32 i = 0; i < screen->num_pixels; i++) {
        /*if ((get_y(s, i)/5)%2 == 0) {*/
        /*    s->pixels[i] = (255 << 24) + rs_color_pixel(255, 255, 88, 14) + s->frame_num;*/
        /*}*/
        /*else if ((get_x(s, i)/5)%2 == 0) {*/
        /*    s->pixels[i] = (255 << 24) + rs_color_pixel(255, 14, 88, 255) + s->frame_num;*/
        /*}*/
        /*else {*/
            /*screen->pixels[i] = (255 << 24) + (i*2) + scene->frame_num;*/
            screen->pixels[i] = rs_color_pixel(255, 0, 0, 0);
        /*}*/
    }
    scene->last_millis = millis;
    scene->frame_num++;
    if ((scene->frame_num % 100) == 0) {
        printf("frame number: %d\n", scene->frame_num);
    }

    rs_map_viewport* v = scene->viewport;

    u32 ul_map_cell_x, ul_map_cell_y;
    u32 lr_map_cell_x, lr_map_cell_y;
    calc_map_cell_bounds_for_viewport(&ul_map_cell_x, &ul_map_cell_y,
                                      &lr_map_cell_x, &lr_map_cell_y,
                                      screen, v);

    rs_map* map = scene->map;

    for (u32 cell_x = ul_map_cell_x; cell_x <= lr_map_cell_x; cell_x++) {
        for (u32 cell_y = ul_map_cell_y; cell_y <= lr_map_cell_y; cell_y++) {
            draw_map_cell(screen, v, map, cell_x, cell_y);
        }
    }

    /*u32 screen_x;*/
    /*u32 screen_y;*/
    /*map2screen(&screen_x, &screen_y, scene->screen, v);*/


}
