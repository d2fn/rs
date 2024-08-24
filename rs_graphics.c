#include "rs_graphics.h"
#include "rs_render.h"
#include <stdlib.h>
#include <math.h>

void swp(float* a, float* b) {
    /*float tmp = *a;*/
    /**a = *b;*/
    /**b = tmp;*/
}

u8 rs_alpha(rs_color c) {
    return c >> 24;
}

u8 rs_red(rs_color c) {
    return (c & 0x00ff0000) >> 16;
}

u8 rs_green(rs_color c) {
    return (c & 0x0000ff00) >> 8;
}

u8 rs_blue(rs_color c) {
    return (c & 0x000000ff);
}

rs_color rs_make_color(u8 a, u8 r, u8 g, u8 b) {
    rs_color pixel = 0;
    pixel |= a << 24;
    pixel |= r << 16;
    pixel |= g << 8;
    pixel |= b;
    return pixel;
}

rs_color rs_blend_color(rs_color m, rs_color n, float bias) {
    if (bias < 0.0) bias = 0.0;
    if (bias > 1.0) bias = 1.0;
    float a = (1.0 - bias) * rs_alpha(m) + (bias * rs_alpha(n));
    float r = (1.0 - bias) * rs_red(m) + (bias * rs_red(n));
    float g = (1.0 - bias) * rs_green(m) + (bias * rs_green(n));
    float b = (1.0 - bias) * rs_blue(m) + (bias * rs_blue(n));
    return rs_make_color(a, r, g, b);
}

rs_color rs_adjust_gamma(rs_color color, float gamma) {
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

rs_buffer* rs_new_buffer(int width, int height) {
    rs_buffer* buf = malloc(sizeof(rs_buffer));
    buf->width = width;
    buf->height = height;
    buf->num_pixels = width * height;
    buf->pixels = malloc(buf->num_pixels * sizeof(rs_color));
    return buf;
}

void rs_free_buffer(rs_buffer* b) {
    free(b->pixels);
    free(b);
}

u32 xy2i(rs_buffer* b, float x, float y) {
    return (u32)(round(x) + (round(y) * b->width));
}

void rs_set_pixel(rs_buffer* b, float x, float y, rs_color c) {
    if (x < 0 || x >= b->width) return;
    if (y < 0 || y >= b->height) return;
    b->pixels[xy2i(b, x, y)] = c;
}

rs_color rs_get_pixel(rs_buffer* b, float x, float y) {
    if (x < 0 || x >= b->width) return 0;
    if (y < 0 || y >= b->height) return 0;
    return b->pixels[xy2i(b, x, y)];
}

void rs_draw_pixel(rs_buffer* b, float x, float y, rs_color c) {
    if (x < 0 || x >= b->width || y < 0 || y >= b->height) return;
    rs_color to_draw = rs_blend_color(rs_get_pixel(b, x, y), c, (float)rs_alpha(c)/255.0);
    rs_set_pixel(b, x, y, to_draw);
}

void rs_hline(rs_buffer* b, float x1, float x2, float y, rs_color color) {
    if (x1 > x2) swp(&x1, &x2);
    for (float x = x1; x <= x2; x++) {
        rs_draw_pixel(b, roundf(x), roundf(y), color);
    }
}

void rs_vline(rs_buffer* b, float x, float y1, float y2, rs_color color) {
    if (y1 > y2) swp(&y1, &y2);
    for (float y = y1; y <= y2; y++) {
        rs_draw_pixel(b, roundf(x), roundf(y), color);
    }
}

void rs_rect(rs_buffer* b, float x1, float y1, float x2, float y2, rs_color color, u8 fill) {
    if (x1 > x2) swp(&x1, &x2);
    if (y1 > y2) swp(&y1, &y2);
    if (x2 < 0 || x1 >= b->width) return;
    if (y2 < 0 || y1 >= b->height) return;
    if (fill) {
        for (int x = roundf(x1); x <= x2; x++) {
            for (int y = roundf(y1); y <= y2; y++) {
                rs_draw_pixel(b, x, y, color);
            }
        }
    }
    else {
        rs_vline(b, x1, y1, y2, color);
        rs_vline(b, x2, y1, y2, color);
        rs_hline(b, x1, x2, y1, color);
        rs_hline(b, x1, x2, y2, color);
    }
}

void rs_line(rs_buffer* b, float x1, float y1, float x2, float y2, rs_color color, float weight) {

    float dx = fabs(x2 - x1);
    float dy = fabs(y2 - y1);
    float sx = (x1 < x2) ? 1 : -1;
    float sy = (y1 < y2) ? 1 : -1;
    float err = dx - dy;
    float e2, x, y;

    while (1) {
        for (x = -weight / 2.0; x <= weight / 2.0; x++) {
            for (y = -weight / 2; y <= weight / 2.0; y++) {
                rs_draw_pixel(b, x1 + x, y1 + y, color);
            }
        }

        if (round(x1) == round(x2) && round(y1) == round(y2)) break;

        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

float interp(float a, float b, float threshold) {
    return (threshold - a) / (b - a);
}

void rs_draw_contour_lines(rs_buffer* b, int* data, int data_width, int data_height, rs_color stroke_color) {
    int interval = 100;
    for(int level = 150; level <= 200; level += interval) {
        for(int y = 0; y < data_height-1; y++) {
            for(int x = 0; x < data_width-1; x++) {
                int top_left  = data[x + y * data_width];
                int top_right = data[(x+1) + y * data_width];
                int bottom_left = data[x + (y+1) * data_width];
                int bottom_right = data[(x+1) + (y+1) * data_width];

                int case_index = 0;
                if (top_left > level) case_index |= 1;
                if (top_right > level) case_index |= 2;
                if (bottom_right > level) case_index |= 4;
                if (bottom_left > level) case_index |= 8;

                switch (case_index) {
                    case 1: case 14:
                        rs_line(b, x, y + interp(top_left, bottom_left, level), x + interp(top_left, top_right, level), y, stroke_color, 1.0);
                        break;
                }
            }
        }
    }
}

/*// Function to generate contour lines using the Marching Squares algorithm*/
/*void generateContours(int* map, int width, int height, int interval) {*/
/*    // Iterate over each contour level (e.g., 0, 5, 10, ...)*/
/*    for (int level = 0; level <= 100; level += interval) {*/
/*        for (int y = 0; y < height - 1; y++) {*/
/*            for (int x = 0; x < width - 1; x++) {*/
/*                // Values of the four corners of the current grid cell*/
/*                int topLeft = map[y * width + x];*/
/*                int topRight = map[y * width + (x + 1)];*/
/*                int bottomLeft = map[(y + 1) * width + x];*/
/*                int bottomRight = map[(y + 1) * width + (x + 1)];*/
/**/
/*                // Determine the case index*/
/*                int caseIndex = 0;*/
/*                if (topLeft > level) caseIndex |= 1;*/
/*                if (topRight > level) caseIndex |= 2;*/
/*                if (bottomRight > level) caseIndex |= 4;*/
/*                if (bottomLeft > level) caseIndex |= 8;*/
/**/
/*                // Determine the contour line segments based on the case index*/
/*                switch (caseIndex) {*/
/*                    case 1: case 14:*/
/*                        // Left edge to bottom edge*/
/*                        printf("Line from (%d, %f) to (%f, %d)\n", x, y + interpolate(topLeft, bottomLeft, level), x + interpolate(topLeft, topRight, level), y);*/
/*                        break;*/
/*                    case 2: case 13:*/
/*                        // Top edge to right edge*/
/*                        printf("Line from (%f, %d) to (%d, %f)\n", x + interpolate(topLeft, topRight, level), y, x + 1, y + interpolate(topRight, bottomRight, level));*/
/*                        break;*/
/*                    case 3: case 12:*/
/*                        // Left edge to right edge*/
/*                        printf("Line from (%d, %f) to (%d, %f)\n", x, y + interpolate(topLeft, bottomLeft, level), x + 1, y + interpolate(topRight, bottomRight, level));*/
/*                        break;*/
/*                    case 4: case 11:*/
/*                        // Right edge to bottom edge*/
/*                        printf("Line from (%f, %d) to (%d, %f)\n", x + interpolate(topRight, bottomRight, level), y + 1, x + interpolate(bottomLeft, bottomRight, level), y + 1);*/
/*                        break;*/
/*                    case 5:*/
/*                        // Left edge to right edge, and top edge to bottom edge (ambiguous)*/
/*                        printf("Line from (%d, %f) to (%d, %f)\n", x, y + interpolate(topLeft, bottomLeft, level), x + 1, y + interpolate(topRight, bottomRight, level));*/
/*                        printf("Line from (%f, %d) to (%f, %d)\n", x + interpolate(topLeft, topRight, level), y, x + interpolate(bottomLeft, bottomRight, level), y + 1);*/
/*                        break;*/
/*                    case 6: case 9:*/
/*                        // Top edge to bottom edge*/
/*                        printf("Line from (%f, %d) to (%f, %d)\n", x + interpolate(topLeft, topRight, level), y, x + interpolate(bottomLeft, bottomRight, level), y + 1);*/
/*                        break;*/
/*                    case 7: case 8:*/
/*                        // Left edge to right edge*/
/*                        printf("Line from (%d, %f) to (%d, %f)\n", x, y + interpolate(topLeft, bottomLeft, level), x + 1, y + interpolate(topRight, bottomRight, level));*/
/*                        break;*/
/*                    // Cases 0, 15: no line needed (all inside or all outside)*/
/*                }*/
/*            }*/
/*        }*/
/*    }*/
/*}*/
/**/
/*int main() {*/
/*    int map[WIDTH * HEIGHT];*/
/**/
/*    // Assume `map` is filled with data from the Diamond-Square algorithm*/
/*    // (For simplicity, the data is not shown here)*/
/**/
/*    int contourInterval = 5;*/
/*    generateContours(map, WIDTH, HEIGHT, contourInterval);*/
/**/
/*    return 0;*/
/*}*/
