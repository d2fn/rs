#include <poll.h>
#include <SDL2/SDL.h>
#include "SDL_events.h"
#include "SDL_keycode.h"
#include "SDL_timer.h"

#include "rs_map.h"
#include "rs_player.h"
#include "rs_render.h"
#include "rs_tween.h"
#include "rs_input.h"
#include "rs_types.h"

#define WIDTH          1200
#define HEIGHT          900
#define PIXEL_SIZE        1
#define FPS              60

struct {
    u8 up, down, left, right;
    u8 zoom_in, zoom_out;
} nav_state;

float fclamp(float in, float lo, float hi) {
    if (in > hi) return hi;
    if (in < lo) return lo;
    return in;
}

int iclamp(int in, int lo, int hi) {
    if (in > hi) return hi;
    if (in < lo) return lo;
    return in;
}

void move_player(rs_player* p, rs_map* map) {
    float x_dir = (nav_state.left ? -1 : 0) + (nav_state.right ? 1 : 0);
    float y_dir = (nav_state.up ? -1 : 0) + (nav_state.down ? 1 : 0);
    float p_x = fclamp(rs_tween_poll_target(p->map_x) + x_dir, 0, map->width-1);
    float p_y = fclamp(rs_tween_poll_target(p->map_y) + y_dir, 0, map->height-1);
    rs_tween_target(p->map_x, p_x);
    rs_tween_target(p->map_y, p_y);
}

void pan_and_zoom(rs_map_viewport* v, float map_x, float map_y) {
    rs_tween_target(v->map_x, map_x);
    rs_tween_target(v->map_y, map_y);
    int nav_change = (nav_state.zoom_out ? -1 : 0) + (nav_state.zoom_in ? 1 : 0);
    v->zoom_level = (u8)iclamp((int)v->zoom_level + nav_change, 0, VIEWPORT_MAX_ZOOM);
}

int main(int argc, char ** argv) {
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

    rs_screen* s = rs_make_screen(WIDTH, HEIGHT, PIXEL_SIZE);

    rs_map* m = rs_make_map(WIDTH*10, HEIGHT*10);
    rs_map_seq_fill(m); 

    rs_map_viewport* v = rs_make_map_viewport((float)m->width/2.0, (float)m->height/2.0, VIEWPORT_MAX_ZOOM);
    rs_player* p = rs_make_player((float)m->width/2.0, (float)m->height/2.0);

    rs_scene* scene = rs_make_scene(s, m, v, p, FPS);

    printf("main_setup: player_map_x -> %.2f, player_map_y = %.2f\n", rs_tween_poll(scene->player->map_x), rs_tween_poll(scene->player->map_y));
    printf("            viewport_x -> %.2f, viewport_x = %.2f\n", rs_tween_poll(scene->viewport->map_y), rs_tween_poll(scene->viewport->map_y));

    nav_state.up = 0;
    nav_state.down = 0;
    nav_state.left = 0;
    nav_state.right = 0;
    nav_state.zoom_in = 0;
    nav_state.zoom_out = 0;

    while (!quit) {

        printf("main_loop: player_map_x -> %.2f, player_map_y = %.2f\n", rs_tween_poll(scene->player->map_x), rs_tween_poll(scene->player->map_y));

        rs_scene_update(scene);

        rs_render(scene, SDL_GetTicks());

        SDL_UpdateTexture(texture, NULL, rs_capture_output_buffer(s), WIDTH * sizeof(u32));
        while (SDL_PollEvent(&event) != NULL) {
            switch(event.type) {
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            quit = 1;
                            break;
                        case SDLK_h:
                            nav_state.left = 1;
                            break;
                        case SDLK_l:
                            nav_state.right = 1;
                            break;
                        case SDLK_j:
                            nav_state.down = 1;
                            break;
                        case SDLK_k:
                            nav_state.up = 1;
                            break;
                        case SDLK_i:
                            nav_state.zoom_in = 1;
                            break;
                        case SDLK_o:
                            nav_state.zoom_out = 1;
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch(event.key.keysym.sym) {
                        case SDLK_h:
                            nav_state.left = 0;
                            break;
                        case SDLK_l:
                            nav_state.right = 0;
                            break;
                        case SDLK_j:
                            nav_state.down = 0;
                            break;
                        case SDLK_k:
                            nav_state.up = 0;
                            break;
                        case SDLK_i:
                            nav_state.zoom_in = 0;
                            break;
                        case SDLK_o:
                            nav_state.zoom_out = 0;
                            break;
                    }
            }
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        pan_and_zoom(v, rs_tween_poll_target(p->map_x), rs_tween_poll_target(p->map_y));
        move_player(p, m);

        poll(NULL, 0, 1000/FPS);
    }

    SDL_Quit();

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);

    // todo -- free everything from the scene
    rs_free_screen(s);
    rs_free_map(m);
    rs_free_map_viewport(v);
    rs_free_player(p);
    rs_free_scene(scene);

    return 0;
}
