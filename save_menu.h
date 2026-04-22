#ifndef SAVE_MENU_H
#define SAVE_MENU_H

#include <SDL2/SDL.h>
#include "img_button.h"
#include "player.h"
#include "level.h"
#include "platform.h"
#include "game_state.h"

#define MAX_SAVES 3
#define SAVE_VERSION 1

typedef struct {
    char name[32];
    int levelId;
    float playerX, playerY;
    int timestamp;
    int exists;
} SaveSlot;

typedef struct {
    ImgButton btn_slots[MAX_SAVES];
    ImgButton btn_save;
    ImgButton btn_load;
    ImgButton btn_delete;
    ImgButton btn_back;
    Mix_Chunk *sfx_hover;
    Mix_Chunk *sfx_click;
    SDL_Texture *bg;
    int selectedSlot;
    int requested_state;
    SaveSlot saves[MAX_SAVES];
    char message[128];
    Uint32 messageTimer;
} SaveMenu;

void smInit(SaveMenu *sm, SDL_Renderer *r);
void smHandleEvent(SaveMenu *sm, SDL_Window *w, SDL_Event *e, 
                   Player *player, LevelManager *lm, PlatformManager *pm);
void smRender(SaveMenu *sm, SDL_Renderer *r, SDL_Window *w);
void smDestroy(SaveMenu *sm);
void smRefreshSlots(SaveMenu *sm);
int smSaveGame(SaveMenu *sm, int slot, Player *player, LevelManager *lm, PlatformManager *pm);
int smLoadGame(SaveMenu *sm, int slot, Player *player, LevelManager *lm, PlatformManager *pm, SDL_Renderer *r);
void smDeleteSave(SaveMenu *sm, int slot);

#endif
