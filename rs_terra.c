#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "rs_terra.h"
#include "rs_grid.h"
#include "rs_types.h"
#include "rs_perlin.h"

float rand_range(float range) {
    return ((float)rand() / RAND_MAX) * 2 * range - range;
}

int index(int x, int y, int size) {
    return ( y * size ) + x;
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
    float erosion_fx[] = {   0.0,  0.1,   .92,   .99 };
    float erosion_fy[] = { 100.0, 80.0, 12.00,  5.00 };
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

void perlin_fill(rs_grid* g, float scale, float octaves, float persistence, float lacunarity) {
    for (u32 y = 0; y < g->height; y++) {
        for (u32 x = 0; x < g->width; x++) {
            float noise = cnoise2((float)x/scale, (float)y/scale, octaves, persistence, lacunarity);
            rs_grid_set(g, x, y, noise);
        }
    }
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

// Function to calculate the light intensity for each cell considering a point light source
/*
void calculateLighting(int *terrain, float *lighting, int width, int height, float lightX, float lightY, float lightZ) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Get the current height at (x, y)
            float terrainHeight = (float)terrain[y * width + x];

            // Calculate the vector from the light source to the current point on the terrain
            float lightVectorX = x - lightX;
            float lightVectorY = y - lightY;
            float lightVectorZ = terrainHeight - lightZ;

            // Normalize the light vector
            float lightVectorLength = sqrtf(lightVectorX * lightVectorX + lightVectorY * lightVectorY + lightVectorZ * lightVectorZ);
            lightVectorX /= lightVectorLength;
            lightVectorY /= lightVectorLength;
            lightVectorZ /= lightVectorLength;

            // Approximate the surface normal at this point using the height differences with neighboring points
            float leftHeight = (x > 0) ? (float)terrain[y * width + (x - 1)] : terrainHeight;
            float rightHeight = (x < width - 1) ? (float)terrain[y * width + (x + 1)] : terrainHeight;
            float topHeight = (y > 0) ? (float)terrain[(y - 1) * width + x] : terrainHeight;
            float bottomHeight = (y < height - 1) ? (float)terrain[(y + 1) * width + x] : terrainHeight;

            // Calculate the terrain normal vector using cross products
            float normalX = leftHeight - rightHeight;
            float normalY = topHeight - bottomHeight;
            float normalZ = 2.0f; // Scale factor to adjust the influence of height differences

            // Normalize the normal vector
            float normalLength = sqrtf(normalX * normalX + normalY * normalY + normalZ * normalZ);
            normalX /= normalLength;
            normalY /= normalLength;
            normalZ /= normalLength;

            // Calculate the dot product of the light vector and the normal vector
            float dotProduct = lightVectorX * normalX + lightVectorY * normalY + lightVectorZ * normalZ;

            // The light intensity is proportional to the dot product (clamped between 0 and 1)
            float intensity = fmaxf(0.0f, dotProduct);

            // Store the calculated intensity
            lighting[y * width + x] = intensity;
        }
    }
}

int main() {
    int terrain[WIDTH * HEIGHT];
    float lighting[WIDTH * HEIGHT];

    // Assume `terrain` is filled with height data from the Diamond-Square algorithm

    // Define the light source's position in 3D space (e.g., above and to the left of the terrain)
    float lightX = -50.0f;
    float lightY = -50.0f;
    float lightZ = 200.0f;

    // Calculate lighting
    calculateLighting(terrain, lighting, WIDTH, HEIGHT, lightX, lightY, lightZ);

    // Example: Print the generated lighting values
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            printf("Lighting at (%d, %d): %f\n", x, y, lighting[y * WIDTH + x]);
        }
    }

    return 0;
}
*/
