#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "rs.h"
#include "rs_perlin.h"

//
// utility functions
//

float rand_range(float range) {
    return ((float)rand() / RAND_MAX) * 2 * range - range;
}

int index(int x, int y, int size) {
    return ( y * size ) + x;
}

int find_index(float x, float* fx, int len) {

    int lo = 0;
    int hi = len-1;

    while(lo <= hi) {

        int mid = lo + (hi - lo)/2;

        if (x > fx[mid]) {
            lo = mid+1;
        }
        else {
            hi = mid-1;
        }
    }

    return lo;
}

float linterp(float x, float* fx, float* fy, int len) {
    if (x < fx[0]) return fy[0];
    if (x > fx[len-1]) return fy[len-1];

    int j = find_index(x, fx, len);
    int i = j - 1;

    if (i <= 0) return fy[0];
    if (j >= len-1) return fy[len-1];

    float r = (x - fx[i])/(fx[j] - fx[i]);
    return fy[i] + r * (fy[j] - fy[i]);
}

float rs_remap(float value, float input_min, float input_max, float output_min, float output_max) {
    // Handle cases where input_max < input_min by swapping the input range
    if (input_max < input_min) {
        float temp = input_max;
        input_max = input_min;
        input_min = temp;
    }

    // Handle cases where output_max < output_min by swapping the output range
    if (output_max < output_min) {
        float temp = output_max;
        output_max = output_min;
        output_min = temp;
    }

    // Apply the standard mapping formula
    return output_min + (value - input_min) * (output_max - output_min) / (input_max - input_min);
}

//
// terrain functions
//

void perlin_fill(rs_grid* g, float scale, float octaves, float persistence, float lacunarity) {
    for (u32 y = 0; y < g->height; y++) {
        for (u32 x = 0; x < g->width; x++) {
            float noise = cnoise2((float)x/scale, (float)y/scale, octaves, persistence, lacunarity);
            rs_grid_set(g, x, y, noise);
        }
    }
}


rs_terra* rs_build_world(u32 w, u32 h) {

    rs_grid* base = rs_make_grid(w, h);
    perlin_fill(base, 750.0, 8.0, 0.5, 2.0);
    rs_grid_norm(base, -0.25, 1);

    rs_grid* continentalness = rs_make_grid(w, h);
    perlin_fill(continentalness, 500.0, 2.0, 0.5, 2.0);
    rs_grid_norm(continentalness, 0, 1);

    float offset_fx[] = {  0.0,  0.80,   .89,   .90,   .91 };
    float offset_fy[] = { 20.0, 25.00, 85.00, 90.00, 95.00 };
    float offset_n = 5;

    rs_grid* erosion = rs_make_grid(w, h);
    perlin_fill(erosion, 1000.0, 1.0, 2.0, 1.1);
    rs_grid_norm(erosion, 0, 1);
    float erosion_fx[] = {   0.0,  0.1, 0.8,   .92,   .99 };
    float erosion_fy[] = { 100.0, 80.0, 3.0,  2.00,   .30 };
    float erosion_n = 4;

    rs_grid* map = rs_make_grid(w, h);
    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            float offset = linterp(rs_grid_get(continentalness, x, y), offset_fx, offset_fy, offset_n);
            float scale = linterp(rs_grid_get(erosion, x, y), erosion_fx, erosion_fy, erosion_n);
            rs_grid_set(map, x, y, offset + scale * rs_grid_get(base, x, y));
        }
    }

    rs_grid_norm(map, 100, 200);

    rs_terra* world = malloc(sizeof(rs_terra));
    world->base = base;
    world->continentalness = continentalness;
    world->erosion = erosion;
    world->map = map;
 
    return world;
}

void rs_free_terra(rs_terra* t) {
    rs_free_grid(t->base);
    rs_free_grid(t->continentalness);
    rs_free_grid(t->erosion);
    rs_free_grid(t->map);
    free(t);
}


//
// rs_grid functions
//

rs_grid* rs_make_grid(u32 width, u32 height) {
    rs_grid* g = malloc(sizeof(rs_grid));
    g->width = width;
    g->height = height;
    g->size = width * height;
    g->data = malloc(g->size * sizeof(float));
    memset(g->data, 0, g->size * sizeof(int));
    return g;
}

void rs_free_grid(rs_grid* g) {
    free(g->data);
    free(g);
}


void rs_grid_fill(rs_grid* g, float value) {
    for (u32 i = 0; i < g->width * g->height; i++) {
        g->data[i] = value;
    }
}

void rs_grid_random_fill(rs_grid* g) {
    srand(time(NULL));
    for (u32 i = 0; i < g->width * g->height; i++) {
        g->data[i] = rand() % 5;
    }
}

void rs_grid_seq_fill(rs_grid* g) {
    for (u32 i = 0; i < g->width * g->height; i++) {
        g->data[i] = i;
    }
}

void rs_grid_norm(rs_grid* g, float lo, float hi) {
    float min = FLT_MAX;
    float max = FLT_MIN;
    for (u32 i = 0; i < g->size; i++) {
        float d = g->data[i];
        if (d > max) max = d;
        if (d < min) min = d;
    }
    for (u32 i = 0; i < g->size; i++) {
        g->data[i] = rs_remap(g->data[i], min, max, lo, hi);
    }
}

float rs_grid_get(rs_grid* g, u32 x, u32 y) {
    if (x >= g->width) return 0;
    if (y >= g->height) return 0;
    return g->data[x + (y * g->width)];
}

void rs_grid_set(rs_grid* g, u32 x, u32 y, float value) {
    if (x >= g->width) return;
    if (y >= g->height) return;
    g->data[x + y * g->width] = value;
}

void diamond_square(float* map, int size, float roughness) {

    map[index(0, 0, size)]
        = map[index(0, size-1, size)]
        = map[index(size-1, 0, size)]
        = map[index(size-1, size-1, size)]
        = rand() % 256;

    int step = size - 1;

    while (step > 1) {
        int half_step = step / 2;
        // Diamond step
        for (int y = 0; y < size - 1; y += step) {
            for (int x = 0; x < size - 1; x += step) {
                int mid_x = x + half_step;
                int mid_y = y + half_step;
                // Calculate the average of the corners
                float avg = (map[index(x, y,        size)]  + map[index(x + step, y,        size)] +
                             map[index(x, y + step, size)]  + map[index(x + step, y + step, size)]) / 4.0f;

                // Set the midpoint with a random offset
                map[index(mid_x, mid_y, size)] = avg + rand_range(roughness);
            }
        }

        // Square step
        for (int y = 0; y < size; y += half_step) {
            for (int x = (y + half_step) % step; x < size; x += step) {
                int mid_x = x;
                int mid_y = y;

                // Calculate the average of the neighbors
                float avg = 0;
                int count = 0;

                if (mid_x >= half_step) { // Left neighbor
                    avg += map[index(mid_x - half_step, mid_y, size)];
                    count++;
                }
                if (mid_x + half_step < size) { // Right neighbor
                    avg += map[index(mid_x + half_step, mid_y, size)];
                    count++;
                }
                if (mid_y >= half_step) { // Top neighbor
                    avg += map[index(mid_x, mid_y - half_step, size)];
                    count++;
                }
                if (mid_y + half_step < size) { // Bottom neighbor
                    avg += map[index(mid_x, mid_y + half_step, size)];
                    count++;
                }

                avg /= count;

                // Set the midpoint with a random offset
                map[index(mid_x, mid_y, size)] = avg + rand_range(roughness);
            }
        }

        // Reduce the random range as the steps get smaller
        roughness *= 0.5f;
        step /= 2;
    }
    
}

// Convert the terrain heightmap to a simple 1s and 0s map based on a threshold
void threshold(float map[], int size, float threshold) {
    for (int i = 0; i < size * size; i++) {
        map[i] = map[i] > threshold ? 1 : 0;
    }
}

void rs_calculate_lighting(rs_grid* lightmap, rs_grid* world, rs_light* light) {
    if (lightmap->size != world->size) {
        return;
    }

    float z_at_light = rs_grid_get(world, round(light->x), round(light->y));

    if (z_at_light >= light->z) {
        // light is below surface, set intensity to 0 across the board
        rs_grid_fill(lightmap, 0.0);
        return;
    }

    float min = FLT_MAX;
    float max = FLT_MIN;

    for (int x = 0; x < (int)world->width; x++) {
        for(int y = 0; y < (int)world->height; y++) {

            /*
            float dx = (float)x - (float)light->x;
            float dy = (float)y - (float)light->y;
            float dz = (float)z - (float)light->z;
            float dist_from_light = sqrt((dx*dx) + (dy*dy) + (dz*dz));
            int value = round(dist_from_light);
            if (value < min) min = value;
            if (value > max) max = value;
            rs_set_griddata(lightmap, x, y, value);
            */
            float z = rs_grid_get(world, (u32)x, (u32)y);
            /*if (z < 110) { z = 110.0; }*/


            /*
            float light_vec_x = (float)x - light->x;
            float light_vec_y = (float)y - light->y;
            float light_vec_z = (float)z - light->z;
            */
            float light_vec_x = (float)x - 1;
            float light_vec_y = (float)y - 1;
            float light_vec_z = (float)z - light->z;

            // normalize light vector
            float light_vec_len = sqrtf(light_vec_x * light_vec_x + light_vec_y * light_vec_y + light_vec_z * light_vec_z);
            light_vec_x /= light_vec_len;
            light_vec_y /= light_vec_len;
            light_vec_z /= light_vec_len;

            float left_height     = (x > 0)                         ? rs_grid_get(world, (u32)x - 1u, (u32)y     ) : z;
            float right_height    = (x < (int)lightmap->width - 1)  ? rs_grid_get(world, (u32)x + 1u, (u32)y     ) : z;
            float top_height      = (y > 0)                         ? rs_grid_get(world, (u32)x     , (u32)y - 1u) : z;
            float bottom_height   = (y < (int)lightmap->height - 1) ? rs_grid_get(world, (u32)x     , (u32)y + 1u) : z;

            /*
            if (left_height < 110) left_height = 110;
            if (right_height < 110) right_height = 110;
            if (top_height < 110) top_height = 110;
            if (bottom_height < 110) bottom_height = 110;
            */

            float norm_x = left_height - right_height;
            float norm_y = top_height - bottom_height;
            float norm_z = 2.0f;

            float norm_len = sqrtf(norm_x * norm_x + norm_y * norm_y + norm_z * norm_z);
            norm_x /= norm_len;
            norm_y /= norm_len;
            norm_z /= norm_len;

            float dot_product = light_vec_x * norm_x + light_vec_y * norm_y + light_vec_z * norm_z;
            
            float intensity = fmaxf(0.0, -dot_product);

            rs_grid_set(lightmap, x, y, intensity);
            if (intensity < min) min = intensity;
            if (intensity > max) max = intensity;
        }
    }

    printf("rs_calculate_lighting: min_intensity = %.6f, max_intensity = %.6f, light->z = %.2f\n", min, max, light->z);
}

rs_light* rs_make_light(float x, float y, float z, float intensity) {
    rs_light* light = malloc(sizeof(rs_light));
    light->x = x;
    light->y = y;
    light->z = z;
    light->intensity = intensity;
    return light;
}

Color calc_world_color(rs_grid* world, u32 x_cell, u32 y_cell, u32 frame_num) {

    u8 alpha = 255;

    Color deep_water_color    = { 20, 50, 250, alpha };
    Color shallow_water_color = { 20, 130, 200, alpha };
    Color lo_land_color       = { 20, 200, 130, alpha };
    Color hi_land_color       = { 200, 200, 50, alpha };
    Color ice_land_color      = { 200, 200, 200, alpha };

    u32 pixel_num = x_cell + y_cell * world->width;

    float map_val = rs_grid_get(world, x_cell, y_cell);//world->data[x_cell + y_cell*world->width];
    
    if (map_val <= 100) {
        return (Color) { 0, 0, (u8)((pixel_num+frame_num)%128), 255 };
    }
    else if(map_val <= 110) {
        return BLUE;
    }
    else if(map_val <= 135) {
        return lo_land_color;
    }
    else if(map_val <= 140) {
        return hi_land_color;
    }
    
    return ice_land_color;
}

Color calc_lit_color(Color c, float intensity) {
    float brightness = rs_remap(intensity, 0.0, 1.0, -1.0, 1.0);
    return ColorBrightness(c, brightness);
}
