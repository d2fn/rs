#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "raylib.h"

#include "rs.h"

#define WIDTH                           1500
#define HEIGHT                           700
#define PIXEL_SIZE                         1
#define FPS                               60
#define WORLD_WIDTH              ((1<<11)+1)
#define WORLD_HEIGHT             ((1<<11)+1)
#define WORLD_CENTER_X     (((1<<11)+1)/2.0)
#define WORLD_CENTER_Y     (((1<<11)+1)/2.0)

typedef struct {
    Vector2 ul;
    Vector2 lr;
} bb;

float min(float a, float b, float c, float d) {
    float m = a;
    if (b < m) m = b;
    if (c < m) m = c;
    if (d < m) m = d;
    return m;
}

float max(float a, float b, float c, float d) {
    float m = a;
    if (b > m) m = b;
    if (c > m) m = c;
    if (d > m) m = d;
    return m;
}

void calc_world_bb(bb* dst, Camera2D camera) {
    Vector2 ul = GetScreenToWorld2D((Vector2){    0,      0}, camera);
    Vector2 ur = GetScreenToWorld2D((Vector2){WIDTH,      0}, camera);
    Vector2 lr = GetScreenToWorld2D((Vector2){WIDTH, HEIGHT}, camera);
    Vector2 ll = GetScreenToWorld2D((Vector2){    0, HEIGHT}, camera);
    dst->ul.x = min(ul.x, ur.x, lr.x, ll.x);
    dst->ul.y = min(ul.y, ur.y, lr.y, ll.y);
    dst->lr.x = max(ul.x, ur.x, lr.x, ll.x);
    dst->lr.y = max(ul.y, ur.y, lr.y, ll.y);
}

int main() {

    // this seed looks nice
    int t = 1723874298;//time(NULL);
    srand(t);

    bb visible_world = {0};

    printf("Seeded random number generator with time t = %d\n", t);

    rs_terra* world = rs_build_world(WORLD_WIDTH, WORLD_HEIGHT);
    rs_light* light = rs_make_light(WORLD_WIDTH / 4.0, WORLD_WIDTH / 4.0, 300, 2000);
    light->z = rs_grid_get(world->map, light->x, light->y) + 100;
    rs_grid* lightmap = rs_make_grid(WORLD_WIDTH, WORLD_HEIGHT);
    rs_calculate_lighting(lightmap, world->map, light);

    InitWindow(WIDTH, HEIGHT, "RS");
    SetTargetFPS(FPS);

    Rectangle player = { WIDTH/2.0, HEIGHT/2.0, 20, 20 };
    Camera2D camera = { 0 };
    camera.target = (Vector2){ player.x + player.width/2.0, player.y + player.height/2.0 };
    camera.offset = (Vector2){ WIDTH/2.0, HEIGHT/2.0 };
    camera.rotation = 0.0f;
    camera.zoom = 100.0f;

    while (!WindowShouldClose()) {

        if (IsKeyDown(KEY_RIGHT)) {
            player.x += 2;
        }
        if (IsKeyDown(KEY_LEFT)) {
            player.x -= 2;
        }
        if (IsKeyDown(KEY_UP)) {
            player.y -= 2;
        }
        if (IsKeyDown(KEY_DOWN)) {
            player.y += 2;
        }

        camera.target = (Vector2){ player.x + player.width/2.0, player.y + player.height/2.0 };

        if (IsKeyDown(KEY_A)) {
            camera.rotation--;
        }
        if (IsKeyDown(KEY_S)) {
            camera.rotation++;
        }

        if (camera.rotation >  40) camera.rotation =  40;
        if (camera.rotation < -40) camera.rotation = -40;

        // Camera zoom controls
        
        camera.zoom += ((float)GetMouseWheelMove()*0.05f);

        /*if (camera.zoom > 30.0f) camera.zoom = 30.0f;*/
        /*else if (camera.zoom < 0.01f) camera.zoom = 0.01f;*/

        // Camera reset (zoom and rotation)
        if (IsKeyPressed(KEY_R))
        {
            camera.zoom = 1.0f;
            camera.rotation = 0.0f;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        BeginMode2D(camera);
            
            calc_world_bb(&visible_world, camera);

            int x0 = (int)floor((double)visible_world.ul.x);
            int y0 = (int)floor((double)visible_world.ul.y);
            int xf = (int)ceil((double)visible_world.lr.x);
            int yf = (int)ceil((double)visible_world.lr.y);


            if (x0 < 0) x0 = 0;
            if (xf < 0) xf = 0;
            if (y0 < 0) y0 = 0;
            if (yf < 0) yf = 0;

            if (x0 >= (int)world->map->width) x0 = world->map->width-1;
            if (xf >= (int)world->map->width) xf = world->map->width-1;
            if (y0 >= (int)world->map->height) y0 = world->map->height-1;
            if (yf >= (int)world->map->height) yf = world->map->height-1;
            
            
            for (u32 y = y0; y <= yf; y++) {
                for (u32 x = x0; x < xf; x++) {
                    if (x < 0 || x >= world->map->width) continue;
                    if (y < 0 || y >= world->map->height) continue;
                    float light_intensity = rs_grid_get(lightmap, x, y);
                    Color c = calc_world_color(world->map, x, y, 0);
                    c = calc_lit_color(c, light_intensity);
                    DrawRectangle(x, y, 1, 1, c);
                }
            }

            /*DrawRectangle(-6000, 320, 13000, 8000, DARKGRAY);*/
            DrawRectangleRec(player, RED);

            DrawLine((int)camera.target.x, -HEIGHT*10, (int)camera.target.x, HEIGHT*10, GREEN);
            DrawLine(-WIDTH*10, (int)camera.target.y, HEIGHT*10, (int)camera.target.y, GREEN);

        EndMode2D();

        DrawText("SCREEN AREA", 640, 10, 20, RED);

        DrawRectangle(0, 0, WIDTH, 5, RED);
        DrawRectangle(0, 5, 5, HEIGHT - 10, RED);
        DrawRectangle(WIDTH - 5, 5, 5, HEIGHT - 10, RED);
        DrawRectangle(0, HEIGHT - 5, WIDTH, 5, RED);

        DrawRectangle( 10, 10, 250, 113, Fade(SKYBLUE, 0.5f));
        DrawRectangleLines( 10, 10, 250, 113, BLUE);

        DrawText("Free 2d camera controls:", 20, 20, 10, BLACK);
        DrawText("- Right/Left to move Offset", 40, 40, 10, DARKGRAY);
        DrawText("- Mouse Wheel to Zoom in-out", 40, 60, 10, DARKGRAY);
        DrawText("- A / S to Rotate", 40, 80, 10, DARKGRAY);
        DrawText("- R to reset Zoom and Rotation", 40, 100, 10, DARKGRAY);

        EndDrawing();


    }

    CloseWindow();
    return 0;
}
