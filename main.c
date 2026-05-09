/* main.c – Cyberpunk Puzzle Enigme */
#include "enigme.h"

#define LOADING_MS 2200

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    srand((unsigned)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & (IMG_INIT_JPG | IMG_INIT_PNG))) {
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        SDL_Quit(); return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "ENIGME // CYBERPUNK PUZZLE",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    if (!window) { fprintf(stderr, "%s\n", SDL_GetError()); IMG_Quit(); SDL_Quit(); return 1; }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { fprintf(stderr, "%s\n", SDL_GetError());
        SDL_DestroyWindow(window); IMG_Quit(); SDL_Quit(); return 1; }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    Game g;
    game_init(&g);
    g.window   = window;
    g.renderer = renderer;

    /* ── Loading phase ── */
    Uint32 load_start = SDL_GetTicks();
    game_load_images(&g);

    SDL_Event ev;
    int running = 1;
    Uint32 prev = SDL_GetTicks();

    while (running && g.state == STATE_LOADING) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) running = 0;
        }
        Uint32 now = SDL_GetTicks();
        float dt = (now - prev) / 1000.f;
        prev = now;
        game_update(&g, dt);
        game_draw(&g);
        if (now - load_start >= LOADING_MS) break;
    }

    /* ── Start first round ── */
    if (running) {
        g.round = 0;
        g.score = 0;
        game_start_round(&g);
    }

    /* ── Main loop ── */
    prev = SDL_GetTicks();
    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - prev) / 1000.f;
        if (dt > 0.1f) dt = 0.1f;
        prev = now;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { running = 0; }
            else if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = 0;
                    break;
                case SDLK_r:
                    /* Restart current round */
                    if (g.state == STATE_PLAYING || g.state == STATE_FAILURE) {
                        game_start_round(&g);
                    }
                    break;
                case SDLK_RETURN:
                case SDLK_SPACE:
                    /* From success/failure, restart game */
                    if (g.state == STATE_SUCCESS || g.state == STATE_FAILURE) {
                        g.round = 0;
                        g.score = 0;
                        game_start_round(&g);
                    }
                    break;
                default: break;
                }
            }
            else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                if (g.state == STATE_PLAYING)
                    game_on_mouse_down(&g, ev.button.x, ev.button.y);
                else if (g.state == STATE_SUCCESS || g.state == STATE_FAILURE) {
                    g.round = 0;
                    g.score = 0;
                    game_start_round(&g);
                }
            }
            else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                game_on_mouse_up(&g, ev.button.x, ev.button.y);
            }
            else if (ev.type == SDL_MOUSEMOTION) {
                game_on_mouse_move(&g, ev.motion.x, ev.motion.y);
            }
        }

        game_update(&g, dt);
        game_draw(&g);
    }

    game_free(&g);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
