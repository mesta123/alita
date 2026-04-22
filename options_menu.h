#ifndef OPTIONS_MENU_H
#define OPTIONS_MENU_H

#include <SDL2/SDL.h>
#include "img_button.h"
#include "game_state.h"

typedef struct {
    SDL_Texture *bg;
    SDL_Texture *label_volume;
    SDL_Texture *label_affichage;
    ImgButton btn_plus;
    ImgButton btn_moin;
    ImgButton btn_plein;
    ImgButton btn_normal;
    ImgButton btn_return;
    int volume;
    int fullscreen;
    Mix_Chunk *sfx_hover;
    Mix_Chunk *sfx_click;
    int requested_state;
} OptionsMenu;

void omInit(OptionsMenu *om, SDL_Renderer *r);
void omHandleEvent(OptionsMenu *om, SDL_Window *w, SDL_Event *e);
void omRender(OptionsMenu *om, SDL_Renderer *r, SDL_Window *w);
void omDestroy(OptionsMenu *om);

#endif
