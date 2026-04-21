#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "background.h"

#define SCREEN_W 1920
#define SCREEN_H 1080
#define PLAYER_SPD 10
#define SCORE_INTERVAL 3

int main()
{
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window *win = SDL_CreateWindow(
        "Game",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_W,
        SCREEN_H,
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    Background bg;
    initBackground(&bg, renderer, SCREEN_W, SCREEN_H);
    initTemps(&bg);

    SDL_Surface *guideSurf = IMG_Load("images/guideimage.png");
    SDL_Texture *guideTex = NULL;

    if (guideSurf)
    {
        guideTex = SDL_CreateTextureFromSurface(renderer, guideSurf);
        SDL_FreeSurface(guideSurf);
    }

    SDL_Rect p1 = {300,620,50,50};
    SDL_Rect p2 = {300,620,50,50};

    SDL_Color c1 = {0, 255, 0, 255};
    SDL_Color c2 = {0, 100, 255, 255};

    int twoPlayer = 0;
    int lastScoreTick = 0;
    int quit = 0;

    SDL_Event e;

    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                quit = 1;

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_p)
                twoPlayer = !twoPlayer;
        }

        const Uint8 *ks = SDL_GetKeyboardState(NULL);

        int elapsed = getElapsedSeconds(&bg);
        int scoreTick = elapsed / SCORE_INTERVAL;

        if (scoreTick > lastScoreTick)
        {
            bg.score += scoreTick - lastScoreTick;
            lastScoreTick = scoreTick;
        }

        if (!bg.finJeu)
        {
            if (ks[SDL_SCANCODE_RIGHT])
                scrolling(&bg, PLAYER_SPD);

            if (ks[SDL_SCANCODE_LEFT])
                scrolling(&bg, -PLAYER_SPD);

            if (ks[SDL_SCANCODE_UP])
            {
                p1.y -= PLAYER_SPD;
                if (p1.y < 0) p1.y = 0;
            }

            if (ks[SDL_SCANCODE_DOWN])
            {
                p1.y += PLAYER_SPD;
                if (p1.y > SCREEN_H - p1.h)
                    p1.y = SCREEN_H - p1.h;
            }

            if (bg.level == 1 && bg.scrollAbsoluX > bg.largeurNiveau * 1.75)
            {
                loadLevel(&bg, renderer, 2);
                bg.scrollAbsoluX = 0;
            }
            else if (bg.level == 2 && bg.scrollAbsoluX > bg.largeurNiveau)
            {
                bg.finJeu = 1;
            }
        }

       if (twoPlayer)
{
    
    if (ks[SDL_SCANCODE_UP])
        p1.y -= PLAYER_SPD;

    if (ks[SDL_SCANCODE_DOWN])
        p1.y += PLAYER_SPD;

    if (ks[SDL_SCANCODE_LEFT])
        p1.x -= PLAYER_SPD;

    if (ks[SDL_SCANCODE_RIGHT])
        p1.x += PLAYER_SPD;

    if (p1.x < 0) p1.x = 0;
    if (p1.y < 0) p1.y = 0;
    if (p1.x > SCREEN_W - p1.w) p1.x = SCREEN_W - p1.w;
    if (p1.y > SCREEN_H - p1.h) p1.y = SCREEN_H - p1.h;

    /* PLAYER 2 (BLUE) */
    if (ks[SDL_SCANCODE_D]) p2.x += PLAYER_SPD;
    if (ks[SDL_SCANCODE_A]) p2.x -= PLAYER_SPD;
    if (ks[SDL_SCANCODE_W]) p2.y -= PLAYER_SPD;
    if (ks[SDL_SCANCODE_S]) p2.y += PLAYER_SPD;

    if (p2.x < 0) p2.x = 0;
    if (p2.y < 0) p2.y = 0;
    if (p2.x > SCREEN_W - p2.w) p2.x = SCREEN_W - p2.w;
    if (p2.y > SCREEN_H - p2.h) p2.y = SCREEN_H - p2.h;
}

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (!twoPlayer)
        {
            SDL_RenderSetViewport(renderer, NULL);

            afficherBackground(&bg, renderer, SCREEN_W, SCREEN_H);
            afficherTemps(&bg, renderer, 0);
            afficherScore(&bg, renderer, 0);

            SDL_SetRenderDrawColor(renderer, c1.r, c1.g, c1.b, c1.a);
            SDL_RenderFillRect(renderer, &p1);
        }
        else
        {
            int halfW = SCREEN_W / 2;

            SDL_RenderSetViewport(renderer, &(SDL_Rect){0, 0, halfW, SCREEN_H});
            SDL_RenderCopy(renderer, bg.image, &bg.camera,
                           &(SDL_Rect){0, 0, halfW, SCREEN_H});

            afficherTemps(&bg, renderer, 0);
            afficherScore(&bg, renderer, 0);

            SDL_SetRenderDrawColor(renderer, c1.r, c1.g, c1.b, c1.a);
            SDL_RenderFillRect(renderer,
                               &(SDL_Rect){p1.x * halfW / SCREEN_W, p1.y, p1.w, p1.h});

            SDL_RenderSetViewport(renderer, &(SDL_Rect){halfW, 0, halfW, SCREEN_H});
            SDL_RenderCopy(renderer, bg.image, &bg.camera,
                           &(SDL_Rect){0, 0, halfW, SCREEN_H});

            afficherTemps(&bg, renderer, 0);
            afficherScore(&bg, renderer, 0);

            SDL_SetRenderDrawColor(renderer, c2.r, c2.g, c2.b, c2.a);
            SDL_RenderFillRect(renderer,
                               &(SDL_Rect){p2.x * halfW / SCREEN_W, p2.y, p2.w, p2.h});

            SDL_RenderSetViewport(renderer, NULL);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawLine(renderer, halfW, 0, halfW, SCREEN_H);
        }

        SDL_RenderSetViewport(renderer, NULL);

        if (bg.finJeu)
            afficherWinMessage(&bg, renderer, SCREEN_W, SCREEN_H);

        if (ks[SDL_SCANCODE_G] && guideTex)
            SDL_RenderCopy(renderer, guideTex, NULL, NULL);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (guideTex)
        SDL_DestroyTexture(guideTex);

    saveScore(bg.score);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);

    IMG_Quit();
    SDL_Quit();

    return 0;
}
