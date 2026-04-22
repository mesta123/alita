#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include "game_state.h"
#include "img_button.h"
#include "main_menu.h"
#include "options_menu.h"
#include "utils.h"
#include "player.h"
#include "level.h"
#include "platform.h"
#include "minimap.h"
#include "save_menu.h"
#include "gameplay_state.h"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    StateManager state;
    MainMenu mainMenu;
    OptionsMenu optionsMenu;
    GameplayState gameplay;
    int gameplayInitialized;
    int windowFocused;
} Game;

int initSDL(Game *g) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init error: %s", SDL_GetError());
        return 0;
    }
    
    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & (IMG_INIT_JPG | IMG_INIT_PNG))) {
        SDL_Log("IMG_Init error: %s", IMG_GetError());
        return 0;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        SDL_Log("Mix_OpenAudio error: %s", Mix_GetError());
        return 0;
    }
    
    g->window = SDL_CreateWindow("Platform Game - Level 2 Edition",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        650, 350, SDL_WINDOW_SHOWN);
    if (!g->window) return 0;
    
    printf("Fenetre cree: 650x350\n");
    
    g->renderer = SDL_CreateRenderer(g->window, -1, SDL_RENDERER_SOFTWARE);
    
    if (!g->renderer) {
        printf("ERREUR: %s\n", SDL_GetError());
        return 0;
    }
    
    g->windowFocused = 1;
    
    return 1;
}

void cleanup(Game *g) {
    if (g->gameplayInitialized) gpDestroy(&g->gameplay);
    mmDestroy(&g->mainMenu);
    omDestroy(&g->optionsMenu);
    SDL_DestroyRenderer(g->renderer);
    SDL_DestroyWindow(g->window);
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    Game game = {0};
    game.gameplayInitialized = 0;
    game.windowFocused = 1;
    
    if (!initSDL(&game)) return 1;
    
    initStateManager(&game.state);
    mmInit(&game.mainMenu, game.renderer);
    omInit(&game.optionsMenu, game.renderer);
    
    int running = 1;
    
    while (running && game.state.current != STATE_EXIT) {
        SDL_Event e;
        
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
                break;
            }
            
            if (e.type == SDL_WINDOWEVENT) {
                switch (e.window.event) {
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        game.windowFocused = 1;
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        game.windowFocused = 1;
                        break;
                }
            }
            
            switch (game.state.current) {
                case STATE_MAIN_MENU:
                    mmHandleEvent(&game.mainMenu, game.window, &e);
                    break;
                case STATE_OPTIONS:
                    omHandleEvent(&game.optionsMenu, game.window, &e);
                    break;
                case STATE_GAMEPLAY:
                    if (game.gameplayInitialized) {
                        gpHandleEvent(&game.gameplay, game.window, &e, &game.state);
                    }
                    break;
                default:
                    break;
            }
        }
        
        if (game.mainMenu.requested_state != -1) {
            changeState(&game.state, game.mainMenu.requested_state);
            
            if (game.state.current == STATE_GAMEPLAY && !game.gameplayInitialized) {
                gpInit(&game.gameplay, game.renderer);
                game.gameplayInitialized = 1;
            }
            
            game.mainMenu.requested_state = -1;
        }
        
        if (game.optionsMenu.requested_state != -1) {
            changeState(&game.state, game.optionsMenu.requested_state);
            game.optionsMenu.requested_state = -1;
        }
        
        if (game.gameplayInitialized && game.gameplay.saveMenu.requested_state != -1) {
            if (game.gameplay.saveMenu.requested_state == STATE_MAIN_MENU) {
                changeState(&game.state, STATE_MAIN_MENU);
                game.gameplay.isPaused = 0;
            } else if (game.gameplay.saveMenu.requested_state == STATE_GAMEPLAY) {
                game.gameplay.isPaused = 0;
            }
            game.gameplay.saveMenu.requested_state = -1;
        }
        
        if (game.state.current == STATE_GAMEPLAY && game.gameplayInitialized) {
            gpUpdate(&game.gameplay);
        }
        
        SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
        SDL_RenderClear(game.renderer);
        
        switch (game.state.current) {
            case STATE_MAIN_MENU:
                mmRender(&game.mainMenu, game.renderer, game.window);
                break;
            case STATE_OPTIONS:
                omRender(&game.optionsMenu, game.renderer, game.window);
                break;
            case STATE_GAMEPLAY:
                if (game.gameplayInitialized) {
                    gpRender(&game.gameplay, game.renderer, game.window);
                }
                break;
            default:
                break;
        }
        
        SDL_RenderPresent(game.renderer);
        
        SDL_Delay(16);
    }
    
    cleanup(&game);
    return 0;
}
