#include "options_menu.h"
#include <stdio.h>

static void getCurrentSize(SDL_Window *w, int *width, int *height) {
    SDL_DisplayMode dm;
    int isFullscreen = SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN;
    if (isFullscreen) {
        int displayIndex = SDL_GetWindowDisplayIndex(w);
        SDL_GetCurrentDisplayMode(displayIndex, &dm);
        *width = dm.w;
        *height = dm.h;
    } else {
        SDL_GetWindowSize(w, width, height);
    }
}

static void omRecalculatePositions(OptionsMenu *om, SDL_Window *w) {
    int windowW, windowH;
    getCurrentSize(w, &windowW, &windowH);
    
    int gateCenterX = windowW / 2;
    int offsetX = 150;
    int centerX = gateCenterX + offsetX;
    
    int barWidth = 200;
    int volBtnSize = 60;
    int spacing = 20;
    
    int volY = (windowH - 350) / 2;
    
    om->btn_moin.rect.x = centerX - barWidth/2 - volBtnSize - spacing;
    om->btn_moin.rect.y = volY;
    om->btn_moin.rect.w = volBtnSize;
    om->btn_moin.rect.h = volBtnSize;
    
    om->btn_plus.rect.x = centerX + barWidth/2 + spacing;
    om->btn_plus.rect.y = volY;
    om->btn_plus.rect.w = volBtnSize;
    om->btn_plus.rect.h = volBtnSize;
    
    int affY = volY + 180;
    int affBtnW = 220;
    int affBtnH = 70;
    int affSpacing = 40;
    
    om->btn_plein.rect.x = centerX - affBtnW - affSpacing/2;
    om->btn_plein.rect.y = affY;
    om->btn_plein.rect.w = affBtnW;
    om->btn_plein.rect.h = affBtnH;
    
    om->btn_normal.rect.x = centerX + affSpacing/2;
    om->btn_normal.rect.y = affY;
    om->btn_normal.rect.w = affBtnW;
    om->btn_normal.rect.h = affBtnH;
}

void omInit(OptionsMenu *om, SDL_Renderer *r) {
    om->requested_state = -1;
    om->volume = Mix_VolumeMusic(-1);
    om->fullscreen = 0;
    
    om->sfx_hover = Mix_LoadWAV("hover.mp3");
    om->sfx_click = Mix_LoadWAV("click.wav");
    
    om->bg = loadTex(r, "bg.png");
    om->label_volume = loadTex(r, "volume.jpeg");
    om->label_affichage = loadTex(r, "aff.jpeg");
    
    initButton(r, &om->btn_moin, (SDL_Rect){0, 0, 60, 60}, "moin.jpeg", NULL, NULL);
    initButton(r, &om->btn_plus, (SDL_Rect){0, 0, 60, 60}, "plus.jpeg", NULL, NULL);
    initButton(r, &om->btn_plein, (SDL_Rect){0, 0, 220, 70}, "plein.jpeg", NULL, NULL);
    initButton(r, &om->btn_normal, (SDL_Rect){0, 0, 220, 70}, "normal.jpeg", NULL, NULL);
    initButton(r, &om->btn_return, (SDL_Rect){50, 50, 100, 50}, "return.jpeg", NULL, NULL);
}

void omApplyVolume(OptionsMenu *om) {
    Mix_VolumeMusic(om->volume);
}

void omToggleFullscreen(OptionsMenu *om, SDL_Window *w, int enable) {
    om->fullscreen = enable;
    if (enable) {
        SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(w, 0);
    }
}

void omHandleEvent(OptionsMenu *om, SDL_Window *w, SDL_Event *e) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int mouseDown = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    
    omRecalculatePositions(om, w);
    
    updateButtonState(&om->btn_plus, mx, my, mouseDown, om->sfx_hover);
    updateButtonState(&om->btn_moin, mx, my, mouseDown, om->sfx_hover);
    updateButtonState(&om->btn_plein, mx, my, mouseDown, om->sfx_hover);
    updateButtonState(&om->btn_normal, mx, my, mouseDown, om->sfx_hover);
    updateButtonState(&om->btn_return, mx, my, mouseDown, om->sfx_hover);
    
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        if (mouseInside(om->btn_moin.rect, mx, my)) {
            om->btn_moin.is_down = 1;
            om->btn_plus.is_down = 0;
            om->btn_plein.is_down = 0;
            om->btn_normal.is_down = 0;
            om->btn_return.is_down = 0;
        }
        else if (mouseInside(om->btn_plus.rect, mx, my)) {
            om->btn_moin.is_down = 0;
            om->btn_plus.is_down = 1;
            om->btn_plein.is_down = 0;
            om->btn_normal.is_down = 0;
            om->btn_return.is_down = 0;
        }
        else if (mouseInside(om->btn_plein.rect, mx, my)) {
            om->btn_moin.is_down = 0;
            om->btn_plus.is_down = 0;
            om->btn_plein.is_down = 1;
            om->btn_normal.is_down = 0;
            om->btn_return.is_down = 0;
        }
        else if (mouseInside(om->btn_normal.rect, mx, my)) {
            om->btn_moin.is_down = 0;
            om->btn_plus.is_down = 0;
            om->btn_plein.is_down = 0;
            om->btn_normal.is_down = 1;
            om->btn_return.is_down = 0;
        }
        else if (mouseInside(om->btn_return.rect, mx, my)) {
            om->btn_moin.is_down = 0;
            om->btn_plus.is_down = 0;
            om->btn_plein.is_down = 0;
            om->btn_normal.is_down = 0;
            om->btn_return.is_down = 1;
        }
    }
    
    if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT) {
        int action_taken = 0;
        
        if (om->btn_moin.is_down && mouseInside(om->btn_moin.rect, mx, my) && !action_taken) {
            if (om->sfx_click) Mix_PlayChannel(-1, om->sfx_click, 0);
            om->volume -= 16;
            if (om->volume < 0) om->volume = 0;
            omApplyVolume(om);
            action_taken = 1;
        }
        om->btn_moin.is_down = 0;
        
        if (om->btn_plus.is_down && mouseInside(om->btn_plus.rect, mx, my) && !action_taken) {
            if (om->sfx_click) Mix_PlayChannel(-1, om->sfx_click, 0);
            om->volume += 16;
            if (om->volume > MIX_MAX_VOLUME) om->volume = MIX_MAX_VOLUME;
            omApplyVolume(om);
            action_taken = 1;
        }
        om->btn_plus.is_down = 0;
        
        if (om->btn_plein.is_down && mouseInside(om->btn_plein.rect, mx, my) && !action_taken) {
            if (om->sfx_click) Mix_PlayChannel(-1, om->sfx_click, 0);
            omToggleFullscreen(om, w, 1);
            action_taken = 1;
        }
        om->btn_plein.is_down = 0;
        
        if (om->btn_normal.is_down && mouseInside(om->btn_normal.rect, mx, my) && !action_taken) {
            if (om->sfx_click) Mix_PlayChannel(-1, om->sfx_click, 0);
            omToggleFullscreen(om, w, 0);
            action_taken = 1;
        }
        om->btn_normal.is_down = 0;
        
        if (om->btn_return.is_down && mouseInside(om->btn_return.rect, mx, my) && !action_taken) {
            if (om->sfx_click) Mix_PlayChannel(-1, om->sfx_click, 0);
            om->requested_state = STATE_MAIN_MENU;
            action_taken = 1;
        }
        om->btn_return.is_down = 0;
    }
    
    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
        om->requested_state = STATE_MAIN_MENU;
    }
}

void omRender(OptionsMenu *om, SDL_Renderer *r, SDL_Window *w) {
    SDL_RenderClear(r);
    if (om->bg) SDL_RenderCopy(r, om->bg, NULL, NULL);
    omRecalculatePositions(om, w);
    
    int windowW, windowH;
    getCurrentSize(w, &windowW, &windowH);
    int gateCenterX = windowW / 2;
    int offsetX = 150;
    int centerX = gateCenterX + offsetX;
    
    int barWidth = 200;
    int volBtnSize = 60;
    int volY = om->btn_moin.rect.y;
    int affY = om->btn_plein.rect.y;
    
    if (om->label_volume) {
        int labelW = 150;
        int labelH = 50;
        SDL_Rect dst = {centerX - labelW/2, volY - 70, labelW, labelH};
        SDL_RenderCopy(r, om->label_volume, NULL, &dst);
    }
    
    SDL_Rect volBar = {centerX - barWidth/2, volY + 20, barWidth, 20};
    SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
    SDL_RenderFillRect(r, &volBar);
    
    SDL_Rect volFill = {centerX - barWidth/2, volY + 20, 
                        (int)(barWidth * om->volume / MIX_MAX_VOLUME), 20};
    SDL_SetRenderDrawColor(r, 0, 200, 0, 255);
    SDL_RenderFillRect(r, &volFill);
    
    if (om->label_affichage) {
        int labelW = 250;
        int labelH = 50;
        SDL_Rect dst = {centerX - labelW/2, affY - 70, labelW, labelH};
        SDL_RenderCopy(r, om->label_affichage, NULL, &dst);
    }
    
    renderButton(r, &om->btn_moin);
    renderButton(r, &om->btn_plus);
    renderButton(r, &om->btn_plein);
    renderButton(r, &om->btn_normal);
    renderButton(r, &om->btn_return);
}

void omDestroy(OptionsMenu *om) {
    if (om->bg) SDL_DestroyTexture(om->bg);
    if (om->label_volume) SDL_DestroyTexture(om->label_volume);
    if (om->label_affichage) SDL_DestroyTexture(om->label_affichage);
    if (om->sfx_hover) Mix_FreeChunk(om->sfx_hover);
    if (om->sfx_click) Mix_FreeChunk(om->sfx_click);
    
    destroyButton(&om->btn_plus);
    destroyButton(&om->btn_moin);
    destroyButton(&om->btn_plein);
    destroyButton(&om->btn_normal);
    destroyButton(&om->btn_return);
}
