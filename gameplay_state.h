#ifndef GAMEPLAY_STATE_H
#define GAMEPLAY_STATE_H

#include <SDL2/SDL.h>
#include "player.h"
#include "level.h"
#include "platform.h"
#include "minimap.h"
#include "save_menu.h"
#include "game_state.h"

typedef struct {
    Player player;
    LevelManager levelManager;
    PlatformManager platformManager;
    Minimap minimap;
    SaveMenu saveMenu;
    int isPaused;
    int showGameOver;
    int gameOverTimer;
    Mix_Chunk *sfx_jump;
    SDL_Renderer *renderer;
} GameplayState;

void gpInit(GameplayState *gp, SDL_Renderer *r);
void gpHandleEvent(GameplayState *gp, SDL_Window *w, SDL_Event *e, StateManager *sm);
void gpUpdate(GameplayState *gp);
void gpRender(GameplayState *gp, SDL_Renderer *r, SDL_Window *w);
void gpDestroy(GameplayState *gp);
void gpSetupLevel(GameplayState *gp, int levelId);

#endif
