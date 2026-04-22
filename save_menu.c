#include "save_menu.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

#define SAVE_FILE "saves.dat"

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

void smRefreshSlots(SaveMenu *sm) {
    FILE *f = fopen(SAVE_FILE, "rb");
    if (!f) {
        for (int i = 0; i < MAX_SAVES; i++) {
            sm->saves[i].exists = 0;
            strcpy(sm->saves[i].name, "Vide");
        }
        return;
    }
    
    int version;
    fread(&version, sizeof(int), 1, f);
    
    for (int i = 0; i < MAX_SAVES; i++) {
        fread(&sm->saves[i], sizeof(SaveSlot), 1, f);
    }
    fclose(f);
}

void smInit(SaveMenu *sm, SDL_Renderer *r) {
    sm->requested_state = -1;
    sm->selectedSlot = 0;
    sm->message[0] = '\0';
    sm->messageTimer = 0;
    
    sm->sfx_hover = Mix_LoadWAV("hover.mp3");
    sm->sfx_click = Mix_LoadWAV("click.wav");
    sm->bg = loadTex(r, "bg.png");
    
    // Slots - utiliser des rectangles colorés si pas d'image
    for (int i = 0; i < MAX_SAVES; i++) {
        initButton(r, &sm->btn_slots[i],
                   (SDL_Rect){540, 150 + i * 100, 200, 80},
                   "slot.jpeg", NULL, NULL);
        // Si pas de texture, on utilisera le rendu couleur dans smRender
    }
    
    // Actions
    initButton(r, &sm->btn_save, (SDL_Rect){400, 500, 150, 60}, "save.jpeg", NULL, NULL);
    initButton(r, &sm->btn_load, (SDL_Rect){565, 500, 150, 60}, "load.jpeg", NULL, NULL);
    initButton(r, &sm->btn_delete, (SDL_Rect){730, 500, 150, 60}, "delete.jpeg", NULL, NULL);
    initButton(r, &sm->btn_back, (SDL_Rect){50, 50, 120, 50}, "return.jpeg", NULL, NULL);
    
    smRefreshSlots(sm);
}

void smHandleEvent(SaveMenu *sm, SDL_Window *w, SDL_Event *e,
                   Player *player, LevelManager *lm, PlatformManager *pm) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    int mouseDown = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    
    for (int i = 0; i < MAX_SAVES; i++) {
        updateButtonState(&sm->btn_slots[i], mx, my, mouseDown, sm->sfx_hover);
    }
    updateButtonState(&sm->btn_save, mx, my, mouseDown, sm->sfx_hover);
    updateButtonState(&sm->btn_load, mx, my, mouseDown, sm->sfx_hover);
    updateButtonState(&sm->btn_delete, mx, my, mouseDown, sm->sfx_hover);
    updateButtonState(&sm->btn_back, mx, my, mouseDown, sm->sfx_hover);
    
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        for (int i = 0; i < MAX_SAVES; i++) {
            if (mouseInside(sm->btn_slots[i].rect, mx, my)) {
                sm->selectedSlot = i;
                if (sm->sfx_click) Mix_PlayChannel(-1, sm->sfx_click, 0);
            }
        }
    }
    
    if (e->type == SDL_MOUSEBUTTONUP && e->button.button == SDL_BUTTON_LEFT) {
        int action_taken = 0;
        
        if (sm->btn_save.is_down && mouseInside(sm->btn_save.rect, mx, my) && !action_taken) {
            if (smSaveGame(sm, sm->selectedSlot, player, lm, pm)) {
                snprintf(sm->message, sizeof(sm->message), "Sauvegarde slot %d!", sm->selectedSlot + 1);
            } else {
                snprintf(sm->message, sizeof(sm->message), "Erreur sauvegarde!");
            }
            sm->messageTimer = SDL_GetTicks() + 2000;
            if (sm->sfx_click) Mix_PlayChannel(-1, sm->sfx_click, 0);
            action_taken = 1;
        }
        sm->btn_save.is_down = 0;
        
        if (sm->btn_load.is_down && mouseInside(sm->btn_load.rect, mx, my) && !action_taken) {
            if (sm->saves[sm->selectedSlot].exists) {
                smLoadGame(sm, sm->selectedSlot, player, lm, pm, NULL);
                snprintf(sm->message, sizeof(sm->message), "Charge slot %d!", sm->selectedSlot + 1);
            } else {
                snprintf(sm->message, sizeof(sm->message), "Slot vide!");
            }
            sm->messageTimer = SDL_GetTicks() + 2000;
            if (sm->sfx_click) Mix_PlayChannel(-1, sm->sfx_click, 0);
            action_taken = 1;
        }
        sm->btn_load.is_down = 0;
        
        if (sm->btn_delete.is_down && mouseInside(sm->btn_delete.rect, mx, my) && !action_taken) {
            smDeleteSave(sm, sm->selectedSlot);
            snprintf(sm->message, sizeof(sm->message), "Supprime slot %d!", sm->selectedSlot + 1);
            sm->messageTimer = SDL_GetTicks() + 2000;
            if (sm->sfx_click) Mix_PlayChannel(-1, sm->sfx_click, 0);
            action_taken = 1;
        }
        sm->btn_delete.is_down = 0;
        
        if (sm->btn_back.is_down && mouseInside(sm->btn_back.rect, mx, my) && !action_taken) {
            if (sm->sfx_click) Mix_PlayChannel(-1, sm->sfx_click, 0);
            sm->requested_state = STATE_GAMEPLAY;
            action_taken = 1;
        }
        sm->btn_back.is_down = 0;
    }
    
    if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
        sm->requested_state = STATE_GAMEPLAY;
    }
}

// Fonction helper pour dessiner un bouton avec texte si pas de texture
static void drawButtonFallback(SDL_Renderer *r, ImgButton *btn, const char *text, SDL_Color color) {
    // Fond
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, 200);
    SDL_RenderFillRect(r, &btn->rect);
    
    // Bordure
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDrawRect(r, &btn->rect);
    
    // Effet hover/pressed
    if (btn->state == BTN_STATE_HOVER) {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 50);
        SDL_RenderFillRect(r, &btn->rect);
    } else if (btn->state == BTN_STATE_PRESSED) {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 50);
        SDL_RenderFillRect(r, &btn->rect);
    }
}

void smRender(SaveMenu *sm, SDL_Renderer *r, SDL_Window *w) {
    int windowW, windowH;
    getCurrentSize(w, &windowW, &windowH);
    
    // Fond semi-transparent
    SDL_SetRenderDrawColor(r, 0, 0, 0, 220);
    SDL_Rect overlay = {0, 0, windowW, windowH};
    SDL_RenderFillRect(r, &overlay);
    
    // Panel central
    SDL_SetRenderDrawColor(r, 50, 50, 70, 255);
    SDL_Rect panel = {300, 100, 680, 520};
    SDL_RenderFillRect(r, &panel);
    
    // Titre
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_Rect title = {450, 120, 380, 40};
    SDL_RenderFillRect(r, &title);
    
    // Slots
    for (int i = 0; i < MAX_SAVES; i++) {
        // Highlight sélection
        if (i == sm->selectedSlot) {
            SDL_SetRenderDrawColor(r, 255, 215, 0, 255);
            SDL_Rect hl = {sm->btn_slots[i].rect.x - 5, sm->btn_slots[i].rect.y - 5,
                          sm->btn_slots[i].rect.w + 10, sm->btn_slots[i].rect.h + 10};
            SDL_RenderDrawRect(r, &hl);
        }
        
        // Dessiner le slot (texture ou fallback)
        if (sm->btn_slots[i].tex_normal) {
            renderButton(r, &sm->btn_slots[i]);
        } else {
            SDL_Color slotColor = sm->saves[i].exists ? 
                (SDL_Color){100, 200, 100, 255} : (SDL_Color){100, 100, 100, 255};
            drawButtonFallback(r, &sm->btn_slots[i], sm->saves[i].name, slotColor);
        }
        
        // Info slot (nom)
        SDL_Rect info = {sm->btn_slots[i].rect.x + 220, sm->btn_slots[i].rect.y + 20, 300, 40};
        if (sm->saves[i].exists) {
            SDL_SetRenderDrawColor(r, 100, 255, 100, 255);
        } else {
            SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
        }
        SDL_RenderFillRect(r, &info);
    }
    
    // Boutons d'action (avec fallback si pas de texture)
    if (sm->btn_save.tex_normal) renderButton(r, &sm->btn_save);
    else drawButtonFallback(r, &sm->btn_save, "SAVE", (SDL_Color){50, 150, 50, 255});
    
    if (sm->btn_load.tex_normal) renderButton(r, &sm->btn_load);
    else drawButtonFallback(r, &sm->btn_load, "LOAD", (SDL_Color){50, 50, 150, 255});
    
    if (sm->btn_delete.tex_normal) renderButton(r, &sm->btn_delete);
    else drawButtonFallback(r, &sm->btn_delete, "DELETE", (SDL_Color){150, 50, 50, 255});
    
    if (sm->btn_back.tex_normal) renderButton(r, &sm->btn_back);
    else drawButtonFallback(r, &sm->btn_back, "BACK", (SDL_Color){100, 100, 100, 255});
    
    // Message
    if (sm->message[0] && SDL_GetTicks() < sm->messageTimer) {
        SDL_Rect msgBg = {400, 640, 480, 40};
        SDL_SetRenderDrawColor(r, 0, 0, 0, 200);
        SDL_RenderFillRect(r, &msgBg);
    }
}

void smDestroy(SaveMenu *sm) {
    if (sm->bg) SDL_DestroyTexture(sm->bg);
    if (sm->sfx_hover) Mix_FreeChunk(sm->sfx_hover);
    if (sm->sfx_click) Mix_FreeChunk(sm->sfx_click);
    for (int i = 0; i < MAX_SAVES; i++) destroyButton(&sm->btn_slots[i]);
    destroyButton(&sm->btn_save);
    destroyButton(&sm->btn_load);
    destroyButton(&sm->btn_delete);
    destroyButton(&sm->btn_back);
}

int smSaveGame(SaveMenu *sm, int slot, Player *player, LevelManager *lm, PlatformManager *pm) {
    if (slot < 0 || slot >= MAX_SAVES) return 0;
    
    SaveSlot *save = &sm->saves[slot];
    snprintf(save->name, 32, "Save %d", slot + 1);
    save->levelId = lm->currentLevel;
    save->playerX = player->x;
    save->playerY = player->y;
    save->timestamp = (int)time(NULL);
    save->exists = 1;
    
    FILE *f = fopen(SAVE_FILE, "r+b");
    if (!f) {
        f = fopen(SAVE_FILE, "wb");
        if (!f) return 0;
        int version = SAVE_VERSION;
        fwrite(&version, sizeof(int), 1, f);
        SaveSlot empty[MAX_SAVES] = {0};
        fwrite(empty, sizeof(SaveSlot), MAX_SAVES, f);
        fclose(f);
        f = fopen(SAVE_FILE, "r+b");
    }
    
    fseek(f, sizeof(int) + slot * sizeof(SaveSlot), SEEK_SET);
    fwrite(save, sizeof(SaveSlot), 1, f);
    fclose(f);
    
    smRefreshSlots(sm);
    return 1;
}

int smLoadGame(SaveMenu *sm, int slot, Player *player, LevelManager *lm, 
               PlatformManager *pm, SDL_Renderer *r) {
    if (slot < 0 || slot >= MAX_SAVES || !sm->saves[slot].exists) return 0;
    
    SaveSlot *save = &sm->saves[slot];
    
    levelSwitchTo(lm, r, save->levelId);
    playerSetPosition(player, save->playerX, save->playerY);
    platformReset(pm);
    
    return 1;
}

void smDeleteSave(SaveMenu *sm, int slot) {
    if (slot < 0 || slot >= MAX_SAVES) return;
    
    sm->saves[slot].exists = 0;
    
    FILE *f = fopen(SAVE_FILE, "r+b");
    if (f) {
        fseek(f, sizeof(int) + slot * sizeof(SaveSlot), SEEK_SET);
        fwrite(&sm->saves[slot], sizeof(SaveSlot), 1, f);
        fclose(f);
    }
    
    smRefreshSlots(sm);
}
