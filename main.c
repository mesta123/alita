#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "ennemi.h"

int main(void) {
    srand((unsigned int)SDL_GetTicks());

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window *win = SDL_CreateWindow(
        "Alita vs Zapan",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, 0);

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    /* ── Init game objects ────────────────────────────── */
    Background bg;
    initBackground(&bg, ren);

    Player p;
    initPlayer(&p, ren);

    Enemy e;
    initEnemy(&e, ren);

    SDL_Color green  = {80,  220,  80, 255};
    SDL_Color red    = {220,  60,  60, 255};
    SDL_Color darkBg = {30,   30,  30, 200};

    HealthBar playerHP, enemyHP;
    initHealth(&playerHP, 20,  green, darkBg);   /* left side  */
    initHealth(&enemyHP,  600, red,   darkBg);   /* right side */

    /* ── Game loop ────────────────────────────────────── */
    SDL_Event ev;
    int running = 1;

    while (running) {
        /* Events */
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            handleInput(&p, ev);
        }

        /* Update */
        updatePlayer(&p);
        updateEnemy(&e, &p);

        /* Collision: player attacking enemy */
        if (p.state == ANIM_ATTACK && checkCollision(p.dest, e.dest)) {
            if (enemyHP.current > 0) {
                enemyHP.current -= 1;
                if (enemyHP.current <= 0) {
                    enemyHP.current = 0;
                    setEnemyState(&e, ANIM_DEATH);
                }
            }
        }

        /* Collision: enemy attacking player */
        if (e.state == ANIM_ATTACK && checkCollision(e.dest, p.dest)) {
            if (playerHP.current > 0)
                playerHP.current -= 1;
        }

        /* Render */
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        renderBackground(bg, ren);
        renderPlayer(p, ren);
        renderEnemy(e, ren);
        renderHealth(playerHP, ren);
        renderHealth(enemyHP,  ren);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    /* Cleanup */
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
