#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"

#include "rs.h"

#define WIDTH                           1500
#define HEIGHT                           700
#define PIXEL_SIZE                         1
#define FPS                              200
#define WORLD_WIDTH              ((1<<11)+1)
#define WORLD_HEIGHT             ((1<<11)+1)
#define WORLD_CENTER_X     (((1<<11)+1)/2.0)
#define WORLD_CENTER_Y     (((1<<11)+1)/2.0)

typedef struct {
    Vector2 ul;
    Vector2 lr;
} bb;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    Vector2 mass;
} particle;

typedef struct {
    float G;
    int num_particles;
} scene_info;

#define MAX_PARTICLES 1000

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

    /**
    int maxWorkGroupSize[3] = { 0 };
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize[2]);
    **/

    //printf("Max Work Group Size: X=%d, Y=%d, Z=%d\n", maxWorkGroupSize[0], maxWorkGroupSize[1], maxWorkGroupSize[2]);

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

    Rectangle player = { 0, 0, 20, 20 };
    Camera2D camera = { 0 };
    camera.target = (Vector2){ player.x + player.width/2.0, player.y + player.height/2.0 };
    camera.offset = (Vector2){ WIDTH/2.0, HEIGHT/2.0 };
    /*camera.target = (Vector2){ 0, 0 };*/
    /*camera.offset = (Vector2){ 0, 0 };*/
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;

    char *compute_forces_code = LoadFileText("resources/compute_forces.glsl");
    unsigned int compute_forces_shader = rlCompileShader(compute_forces_code, RL_COMPUTE_SHADER);
    unsigned int compute_forces_program = rlLoadComputeShaderProgram(compute_forces_shader);
    UnloadFileText(compute_forces_code);

    particle* src_particles = malloc(MAX_PARTICLES * sizeof(particle));
    for (int i = 0; i < MAX_PARTICLES; i++) {
        src_particles[i].position = (Vector2) { GetRandomValue(-50, 50), GetRandomValue(-50, 50) };
        src_particles[i].velocity = (Vector2) { 0, 0 };
        src_particles[i].acceleration = (Vector2) { 0, 0 };
        src_particles[i].mass = (Vector2) { 1.0, 0 };
        /*src_particles[i].force = (Vector2) { 0, 0 };*/
    }

    particle* dst_particles = malloc(MAX_PARTICLES * sizeof(particle));
    memcpy(dst_particles, src_particles, sizeof(particle) * MAX_PARTICLES);

    scene_info info = { 0.0081f, MAX_PARTICLES };

    unsigned int ssboA = rlLoadShaderBuffer(sizeof(particle) * MAX_PARTICLES, src_particles, RL_DYNAMIC_COPY);
    unsigned int ssboB = rlLoadShaderBuffer(sizeof(particle) * MAX_PARTICLES, dst_particles, RL_DYNAMIC_COPY);
    unsigned int info_buffer = rlLoadShaderBuffer(sizeof(scene_info), &info, RL_DYNAMIC_COPY);

    while (!WindowShouldClose()) {


        /*
        for(int i = 0; i < info.num_particles; i++) {
            Vector2 force = (Vector2) { 0, 0 };
            particles[i].force = force;
            for (int j = 0; j < info.num_particles; j++) {
                if (i != j) {
                    Vector2 r;
                    r.x = particles[i].position.x - particles[j].position.x;
                    r.y = particles[i].position.y - particles[j].position.y;
                    float dist = sqrtf(r.x * r.x + r.y * r.y);
                    Vector2 dir;
                    dir.x = r.x / dist;
                    dir.y = r.y / dist;
                    if (dist != 0) {
                        float F = info.G * (particles[i].mass * particles[j].mass) / (dist * dist);
                        force.x += F * dir.x;
                        force.y += F * dir.y;
                    }
                }
            }
            particles[i].position.x += force.x;
            particles[i].position.y += force.y;
        }
        */


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
           /* 
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

            //DrawRectangle(-6000, 320, 13000, 8000, DARKGRAY);//
            DrawRectangleRec(player, RED);
        */

            DrawLine((int)camera.target.x, -HEIGHT*10, (int)camera.target.x, HEIGHT*10, GREEN);
            DrawLine(-WIDTH*10, (int)camera.target.y, HEIGHT*10, (int)camera.target.y, GREEN);

            for (int i = 0; i < MAX_PARTICLES; i++) {
                DrawCircleV((Vector2){ src_particles[i].position.x, src_particles[i].position.y }, 2, RED);
            }

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

        rlUpdateShaderBuffer(ssboA, src_particles, MAX_PARTICLES * sizeof(particle), 0);
        rlUpdateShaderBuffer(info_buffer, &info, sizeof(scene_info), 0);

        rlEnableShader(compute_forces_program);
        rlBindShaderBuffer(ssboA, 1);
        rlBindShaderBuffer(ssboB, 2);
        rlBindShaderBuffer(info_buffer, 3);
        rlComputeShaderDispatch(MAX_PARTICLES, 1, 1);
        rlDisableShader();

        rlReadShaderBuffer(ssboB, src_particles, sizeof(particle) * MAX_PARTICLES, 0);
        rlReadShaderBuffer(ssboA, dst_particles, sizeof(particle) * MAX_PARTICLES, 0);

        unsigned int tmp = ssboA;
        ssboA = ssboB;
        ssboB = tmp;
    }

    rlUnloadShaderBuffer(ssboA);
    rlUnloadShaderBuffer(ssboB);
    rlUnloadShaderBuffer(info_buffer);
    rlUnloadShaderProgram(compute_forces_program);

    CloseWindow();
    return 0;
}
