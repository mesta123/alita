#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <SDL2/SDL.h>
#include "img_button.h"
#include "game_state.h"

typedef struct {
    SDL_Texture *bg;
    ImgButton btn_start;
    ImgButton btn_load;
    ImgButton btn_option;
    ImgButton btn_quit;
    Mix_Chunk *sfx_hover;
    Mix_Chunk *sfx_click;
    Mix_Music *music;
    int requested_state;
} MainMenu;

void mmInit(MainMenu *mm, SDL_Renderer *r);
void mmHandleEvent(MainMenu *mm, SDL_Window *w, SDL_Event *e);
void mmRender(MainMenu *mm, SDL_Renderer *r, SDL_Window *w);
void mmDestroy(MainMenu *mm);

#endif
