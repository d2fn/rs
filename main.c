#include <poll.h>
#include <SDL2/SDL.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "SDL_events.h"
#include "SDL_mouse.h"
#include "SDL_render.h"
#include "SDL_scancode.h"
#include "SDL_stdinc.h"
#include "SDL_timer.h"

#include "rs_grid.h"
#include "rs_perlin.h"
#include "rs_player.h"
#include "rs_render.h"
#include "rs_terra.h"
#include "rs_tween.h"
#include "rs_types.h"

#define WIDTH                           1200
#define HEIGHT                           900
#define PIXEL_SIZE                         1
#define FPS                              120
#define WORLD_WIDTH               ((1<<9)+1)
#define WORLD_HEIGHT              ((1<<9)+1)
#define WORLD_CENTER_X      (((1<<9)+1)/2.0)
#define WORLD_CENTER_Y      (((1<<9)+1)/2.0)

struct {
    u8 player_up, player_down, player_left, player_right;
    u8 light_up, light_down, light_left, light_right, light_in, light_out;
    u8 zoom_in, zoom_out;
} nav_state;

void reset_nav() {
    memset(&nav_state, 0, sizeof(nav_state));
}

float fclamp(float in, float lo, float hi) {
    if (in > hi) return hi;
    if (in < lo) return lo;
    return in;
}

void move_player(rs_player* p, rs_grid* g) {
    float x_dir = (nav_state.player_left ? -1 : 0) + (nav_state.player_right ? 1 : 0);
    float y_dir = (nav_state.player_up ? -1 : 0) + (nav_state.player_down ? 1 : 0);
    float p_x = fclamp(rs_tween_poll_target(p->map_x) + x_dir*10, 0, g->width-1);
    float p_y = fclamp(rs_tween_poll_target(p->map_y) + y_dir*10, 0, g->height-1);
    rs_tween_target(p->map_x, p_x);
    rs_tween_target(p->map_y, p_y);
}

int move_light(rs_light* light) {
    float x_dir = (nav_state.light_left ? -1 : 0) + (nav_state.light_right ? 1 : 0);
    float y_dir = (nav_state.light_up ? -1 : 0) + (nav_state.light_down ? 1 : 0);
    light->x += x_dir*10;
    light->y += y_dir*10;


    if (nav_state.light_in) {
        light->z *= 0.95;
    }
    else if (nav_state.light_out) {
        light->z *= 1.1;
    }

    return nav_state.light_left || nav_state.light_right || nav_state.light_up || nav_state.light_down || nav_state.light_in || nav_state.light_out;
}

void pan_and_zoom(rs_grid* world, rs_camera* camera, float point_at_x, float point_at_y) {
    rs_tween_target(camera->point_at_x, point_at_x);
    rs_tween_target(camera->point_at_y, point_at_y);
    if (nav_state.zoom_in || nav_state.zoom_out) {
        float fov = rs_tween_poll(camera->fov);
        float new_fov = fov;
        if (nav_state.zoom_in) new_fov *= 0.95;
        if (nav_state.zoom_out) new_fov *= 1.15;
        if (new_fov > world->width) new_fov = (float)world->width;
        if (new_fov < 5) new_fov = 5.0;
        rs_tween_target(camera->fov, new_fov);
    }
}

void make_basic_world(rs_grid* world) {
    float center_x = (float)world->width/2;
    float center_y = (float)world->height/2;
    for (u32 x = 0; x < world->width; x++) {
        for (u32 y = 0; y < world->height; y++) {
            float dx = (center_x - x) / 100;
            float dy = (center_y - y) / 100;
            float x2 = (float)(dx * dx);
            float y2 = (float)(dy * dy);
            /*float z = exp(-(x2+y2));*/
            /*float z = sin((x/25.0)*(y/25.0));*/
            float z = sin(x/5.0)*cos(y/5.0);
            world->data[x + y * world->width] = z;
        }
    }
    rs_grid_norm(world, 100, 128);
}

void debug_to_console(rs_scene* scene, int mouse_x, int mouse_y) {
    float world_x, world_y;
    rs_screen2map(&world_x, &world_y, mouse_x, mouse_y, scene->screen->buf, scene->camera);
    printf("************************************************\n");
    printf("== GAME STATE                                 ==\n");
    printf("mouse            : x=%d, y=%d\n", mouse_x, mouse_y);
    printf("world            : x=%.4f, y=%.4f\n", world_x, world_y);
    printf("terrain height   : %.4f\n", rs_grid_get(scene->world, (u32)round(world_x), (u32)round(world_y)));
    printf("light intensity  : %.4f\n", rs_grid_get(scene->lightmap, (u32)round(world_x), (u32)round(world_y)));
    printf("perlin noise test: %.6f\n", noise2(world_x, world_y));
    printf("************************************************\n");
}

int main() {

    // this seed looks nice
    int t = 1723874298;//time(NULL);
    srand(t);

    printf("Seeded random number generator with time t = %d\n", t);

    int quit = 0;
    SDL_Event event;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window =
        SDL_CreateWindow("My SDL Test Window",
                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         WIDTH, HEIGHT, 0);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture =
        SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            WIDTH, HEIGHT);

    rs_screen* screen = rs_make_screen(WIDTH, HEIGHT, PIXEL_SIZE);

    rs_grid* world = rs_make_grid(WORLD_WIDTH, WORLD_HEIGHT);
    rs_build_world(world); 

    rs_player* player = rs_make_player(WORLD_CENTER_X, WORLD_CENTER_Y);
    rs_camera* camera = rs_make_camera(WORLD_CENTER_X, WORLD_CENTER_Y, WORLD_WIDTH / 4.0);

    rs_light* light = rs_make_light(WORLD_WIDTH / 4.0, WORLD_WIDTH / 4.0, 0, 100);
    // put light 100' above the ground
    light->z = rs_grid_get(world, light->x, light->y) + 100;

    rs_grid* lightmap = rs_make_grid(WORLD_WIDTH, WORLD_HEIGHT);
    rs_calculate_lighting(lightmap, world, light);

    rs_scene* scene = rs_make_scene(screen, world, camera, light, lightmap, player, FPS);


    while (!quit) {
        u32 millis = SDL_GetTicks();
        if ((millis - scene->last_millis) <= (1000/scene->fps)) {
            continue;
        }
        scene->last_millis = millis;
        scene->frame_num++;
        if ((scene->frame_num % 100) == 0) {
            printf("frame number: %d\n", scene->frame_num);
        }

        rs_update_scene(scene);

        // clear screen buffer
        memset(screen->buf->pixels, 0, screen->buf->num_pixels * sizeof(u32));
        rs_camera_render_to(scene, 0, 0, WIDTH, HEIGHT);
        /*rs_render(scene, SDL_GetTicks());*/

        SDL_UpdateTexture(texture, NULL, rs_capture_output_buffer(screen), WIDTH * sizeof(u32));

        reset_nav();
        const Uint8* state = SDL_GetKeyboardState(NULL);

        int mouse_down = 0;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                mouse_down = 1;
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                debug_to_console(scene, mouse_x, mouse_y);
            }
        }

        SDL_PumpEvents();

        if (state[SDL_SCANCODE_Q] || state[SDL_SCANCODE_ESCAPE]) {
            quit = 1;
        }
        else if (state[SDL_SCANCODE_LSHIFT]) {
            if (state[SDL_SCANCODE_H]) {
                nav_state.light_left = 1;
            }
            if (state[SDL_SCANCODE_J]) {
                nav_state.light_down = 1;
            }
            if (state[SDL_SCANCODE_K]) {
                nav_state.light_up = 1;
            }
            if (state[SDL_SCANCODE_L]) {
                nav_state.light_right = 1;
            }
            if (state[SDL_SCANCODE_I]) {
                nav_state.light_in = 1;
            }
            if (state[SDL_SCANCODE_O]) {
                nav_state.light_out = 1;
            }
        }
        else {
            if (state[SDL_SCANCODE_H]) {
                nav_state.player_left = 1;
            }
            if (state[SDL_SCANCODE_J]) {
                nav_state.player_down = 1;
            }
            if (state[SDL_SCANCODE_K]) {
                nav_state.player_up = 1;
            }
            if (state[SDL_SCANCODE_L]) {
                nav_state.player_right = 1;
            }
            if (state[SDL_SCANCODE_I]) {
                nav_state.zoom_in = 1;
            }
            if (state[SDL_SCANCODE_O]) {
                nav_state.zoom_out = 1;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        pan_and_zoom(world, camera, rs_tween_poll_target(player->map_x), rs_tween_poll_target(player->map_y));
        move_player(player, world);
        if (move_light(light)) {
            rs_calculate_lighting(lightmap, world, light);
        }

        poll(NULL, 0, 1000/FPS);
    }

    SDL_Quit();

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);

    // todo -- free everything from the scene
    rs_free_scene(scene);

    return 0;
}
