#include "main_menu.h"
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

void mmInit(MainMenu *mm, SDL_Renderer *r) {
    mm->requested_state = -1;
    mm->sfx_hover = Mix_LoadWAV("hover.mp3");
    mm->sfx_click = Mix_LoadWAV("click.wav");
    mm->music = Mix_LoadMUS("bgmusic.mp3");
    
    if (mm->music) {
        Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
        Mix_PlayMusic(mm->music, -1);
    }
    
    // Charger le background du menu (grande image)
    mm->bg = loadTex(r, "bg.png");
    
    // Taille des boutons réduite pour 650x350
    int btnW = 140;
    int btnH = 35;
    int startX = 450;  // Plus à droite pour la petite fenêtre
    int startY = 80;
    
    initButton(r, &mm->btn_start, 
               (SDL_Rect){startX, startY, btnW, btnH},
               "start.jpeg", NULL, NULL);
    initButton(r, &mm->btn_load,
               (SDL_Rect){startX, startY + 50, btnW, btnH},
               "load.jpeg", NULL, NULL);
    initButton(r, &mm->btn_option,
               (SDL_Rect){startX, startY + 100, btnW, btnH},
               "option.jpeg", NULL, NULL);
    initButton(r, &mm->btn_quit,
               (SDL_Rect){startX, startY + 150, btnW, btnH},
               "quit.jpeg", NULL, NULL);
}

static void mmRecalculatePositions(MainMenu *mm, SDL_Window *w) {
    int windowW, windowH;
    getCurrentSize(w, &windowW, &windowH);
    
    // Fenêtre 650x350, boutons à droite
    int btnW = 140;
    int btnH = 35;
    int btnX = windowW - btnW - 20;  // 20px du bord droit
    int startY = 60;
    int spacing = 45;
    
    mm->btn_start.rect.x = btnX;
    mm->btn_start.rect.y = startY;
    mm->btn_start.rect.w = btnW;
    mm->btn_start.rect.h = btnH;
    
    mm->btn_load.rect.x = btnX;
    mm->btn_load.rect.y = startY + spacing;
    mm->btn_load.rect.w = btnW;
    mm->btn_load.rect.h = btnH;
    
    mm->btn_option.rect.x = btnX;
    mm->btn_option.rect.y = startY + spacing * 2;
    mm->btn_option.rect.w = btnW;
    mm->btn_option.rect.h = btnH;
    
    mm->btn_quit.rect.x = btnX;
    mm->btn_quit.rect.y = startY + spacing * 3;
    mm->btn_quit.rect.w = btnW;
    mm->btn_quit.rect.h = btnH;
}

void mmHandleEvent(MainMenu *mm, SDL_Window *w, SDL_Event *e) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int mouseDown = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    
    mmRecalculatePositions(mm, w);
    
    updateButtonState(&mm->btn_start, mx, my, mouseDown, mm->sfx_hover);
    updateButtonState(&mm->btn_load, mx, my, mouseDown, mm->sfx_hover);
    updateButtonState(&mm->btn_option, mx, my, mouseDown, mm->sfx_hover);
    updateButtonState(&mm->btn_quit, mx, my, mouseDown, mm->sfx_hover);
    
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        if (mouseInside(mm->btn_start.rect, mx, my)) {
            mm->btn_start.is_down = 1;
            mm->btn_load.is_down = 0;
            mm->btn_option.is_down = 0;
            mm->btn_quit.is_down = 0;
        }
        else if (mouseInside(mm->btn_load.rect, mx, my)) {
            mm->btn_start.is_down = 0;
            mm->btn_load.is_down = 1;
            mm->btn_option.is_down = 0;
            mm->btn_quit.is_down = 0;
        }
        else if (mouseInside(mm->btn_option.rect, mx, my)) {
            mm->btn_start.is_down = 0;
            mm->btn_load.is_down = 0;
            mm->btn_option.is_down = 1;
            mm->btn_quit.is_down = 0;
        }
        else if (mouseInside(mm->btn_quit.rect, mx, my)) {
            mm->btn_start.is_down = 0;
            mm->btn_load.is_down = 0;
            mm->btn_option.is_down = 0;
            mm->btn_quit.is_down = 1;
        }
    }
    
    if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT) {
        int action_taken = 0;
        
        if (mm->btn_start.is_down && mouseInside(mm->btn_start.rect, mx, my) && !action_taken) {
            if (mm->sfx_click) Mix_PlayChannel(-1, mm->sfx_click, 0);
            printf("LANCEMENT DU MINI-JEU!\n");
            mm->requested_state = STATE_GAMEPLAY;
            action_taken = 1;
        }
        mm->btn_start.is_down = 0;
        
        if (mm->btn_load.is_down && mouseInside(mm->btn_load.rect, mx, my) && !action_taken) {
            if (mm->sfx_click) Mix_PlayChannel(-1, mm->sfx_click, 0);
            printf("Load Game!\n");
            action_taken = 1;
        }
        mm->btn_load.is_down = 0;
        
        if (mm->btn_option.is_down && mouseInside(mm->btn_option.rect, mx, my) && !action_taken) {
            if (mm->sfx_click) Mix_PlayChannel(-1, mm->sfx_click, 0);
            printf("Options selected!\n");
            mm->requested_state = STATE_OPTIONS;
            action_taken = 1;
        }
        mm->btn_option.is_down = 0;
        
        if (mm->btn_quit.is_down && mouseInside(mm->btn_quit.rect, mx, my) && !action_taken) {
            if (mm->sfx_click) Mix_PlayChannel(-1, mm->sfx_click, 0);
            printf("Quit Game selected!\n");
            mm->requested_state = STATE_EXIT;
            action_taken = 1;
        }
        mm->btn_quit.is_down = 0;
    }
}

void mmRender(MainMenu *mm, SDL_Renderer *r, SDL_Window *w) {
    int windowW, windowH;
    SDL_GetWindowSize(w, &windowW, &windowH);
    
    // Afficher le background du menu (redimensionné pour la fenêtre 650x350)
    if (mm->bg) {
        SDL_Rect dst = {0, 0, windowW, windowH};
        SDL_RenderCopy(r, mm->bg, NULL, &dst);
    }
    
    mmRecalculatePositions(mm, w);
    
    renderButton(r, &mm->btn_start);
    renderButton(r, &mm->btn_load);
    renderButton(r, &mm->btn_option);
    renderButton(r, &mm->btn_quit);
}

void mmDestroy(MainMenu *mm) {
    if (mm->bg) SDL_DestroyTexture(mm->bg);
    if (mm->sfx_hover) Mix_FreeChunk(mm->sfx_hover);
    if (mm->sfx_click) Mix_FreeChunk(mm->sfx_click);
    if (mm->music) Mix_FreeMusic(mm->music);
    
    destroyButton(&mm->btn_start);
    destroyButton(&mm->btn_load);
    destroyButton(&mm->btn_option);
    destroyButton(&mm->btn_quit);
}
