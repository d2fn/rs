#ifndef RS_GRAPHICS_H
#define RS_GRAPHICS_H

#include <stdint.h>

typedef uint32_t rs_color;
typedef uint32_t u32;
typedef uint8_t u8;

typedef struct {
    u32 width;
    u32 height;
    u32 num_pixels; // width * height
    rs_color* pixels;
} rs_buffer;

u8 rs_alpha(rs_color c);
u8 rs_red(rs_color c);
u8 rs_green(rs_color c);
u8 rs_blue(rs_color c);
rs_color rs_make_color(u8 a, u8 r, u8 g, u8 b);
rs_color rs_blend_color(rs_color a, rs_color b, float amt);
rs_buffer* rs_new_buffer(int width, int height);
void rs_free_buffer(rs_buffer* buf);
void rs_set_pixel(rs_buffer* buf, float x, float y, rs_color c);
rs_color rs_get_pixel(rs_buffer* buf, float x, float y);
void rs_draw_pixel(rs_buffer* buf, float x, float y, rs_color c);
rs_color rs_adjust_gamma(rs_color color, float gamma);
void rs_hline(rs_buffer* buf, float x1, float x2, float y, rs_color color);
void rs_vline(rs_buffer* buf, float x, float y1, float y2, rs_color color);
void rs_rect(rs_buffer* buf, float x1, float y1, float x2, float y2, rs_color color, u8 fill);
void rs_line(rs_buffer* buf, float x1, float y1, float x2, float y2, rs_color color, float weight);
void rs_draw_contour_lines(rs_buffer* b, int* data, int data_width, int data_height, rs_color stroke_color);

#endif
