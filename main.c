#include <poll.h>
#include <SDL2/SDL.h>
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

    rs_map_viewport* v = rs_make_map_viewport(m->width/2, m->height/2, VIEWPORT_MAX_ZOOM);
    rs_player* p = rs_make_player(rs_tween_poll(v->map_x), rs_tween_poll(v->map_y));

    rs_scene* scene = rs_make_scene(s, m, v, p, FPS);

    while (!quit) {

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
                            p->map_x--;
                            if (p->map_x < 0) p->map_x = 0;
                            break;
                        case SDLK_l:
                            p->map_x++;
                            if (p->map_x >= m->width) p->map_x = m->width - 1;
                            break;
                        case SDLK_j:
                            p->map_y++;
                            if (p->map_y >= m->height) p->map_y = m->height - 1;
                            break;
                        case SDLK_k:
                            p->map_y--;
                            if (p->map_y < 0) p->map_y = 0;
                            break;
                        case SDLK_i:
                            if (v->zoom_level != VIEWPORT_MAX_ZOOM) {
                                v->zoom_level++;
                            }
                            break;
                        case SDLK_o:
                            if (v->zoom_level != 0) {
                                v->zoom_level--;
                            }
                            break;
                    }
            }
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        rs_map_viewport_pan_to(v, m, s, p->map_x, p->map_y);

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
