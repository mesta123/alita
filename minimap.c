#include "minimap.h"
#include <stdio.h>

void minimapInit(Minimap *mm, SDL_Renderer *r, LevelManager *lm) {
    (void)r;
    (void)lm;
    
    mm->position.x = GAME_WIDTH - MINIMAP_WIDTH - MINIMAP_MARGIN;
    mm->position.y = MINIMAP_MARGIN;
    mm->position.w = MINIMAP_WIDTH;
    mm->position.h = MINIMAP_HEIGHT;
    
    mm->visible = 1;
    mm->scaleX = 1.0f;
    mm->scaleY = 1.0f;
    mm->levelPreview = NULL;
}

void minimapSetLevel(Minimap *mm, LevelManager *lm) {
    if (lm->currentLevel < 0) return;
    
    Level *lvl = &lm->levels[lm->currentLevel];
    
    float scaleX = (float)(MINIMAP_WIDTH - 4) / (float)lvl->width;
    float scaleY = (float)(MINIMAP_HEIGHT - 4) / (float)lvl->height;
    mm->scaleX = (scaleX < scaleY) ? scaleX : scaleY;
    mm->scaleY = mm->scaleX;
}

void minimapUpdate(Minimap *mm, SDL_Renderer *r, LevelManager *lm,
                   Player *player, PlatformManager *pm) {
    (void)r;
    (void)lm;
    (void)pm;
    (void)player;
    
    if (!mm->visible) return;
    
    mm->position.x = GAME_WIDTH - MINIMAP_WIDTH - MINIMAP_MARGIN;
    mm->position.y = MINIMAP_MARGIN;
}

void minimapRender(Minimap *mm, SDL_Renderer *r, LevelManager *lm,
                   Player *player, PlatformManager *pm) {
    if (!mm->visible || !r || lm->currentLevel < 0) return;
    
    Level *lvl = &lm->levels[lm->currentLevel];
    
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderFillRect(r, &mm->position);
    
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDrawRect(r, &mm->position);
    
    SDL_Rect inner = {
        mm->position.x + 2,
        mm->position.y + 2,
        MINIMAP_WIDTH - 4,
        MINIMAP_HEIGHT - 4
    };
    
    float scaleX = (float)(inner.w) / (float)lvl->width;
    float scaleY = (float)(inner.h) / (float)lvl->height;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    
    int mapW = (int)(lvl->width * scale);
    int mapH = (int)(lvl->height * scale);
    
    int offsetX = (inner.w - mapW) / 2;
    int offsetY = (inner.h - mapH) / 2;
    
    SDL_Rect mapBg = {
        inner.x + offsetX,
        inner.y + offsetY,
        mapW,
        mapH
    };
    if (lm->currentLevel == 1) {
        SDL_SetRenderDrawColor(r, 60, 30, 80, 255);
    } else {
        SDL_SetRenderDrawColor(r, 135, 206, 235, 255);
    }
    SDL_RenderFillRect(r, &mapBg);
    
    if (lm->currentLevel == 1) {
        SDL_SetRenderDrawColor(r, 255, 200, 100, 255);
        for (int i = 0; i < lvl->ladderCount; i++) {
            if (!lvl->ladders[i].active) continue;
            int lx = inner.x + offsetX + (int)(lvl->ladders[i].x * scale);
            int ly = inner.y + offsetY + (int)(lvl->ladders[i].y * scale);
            int lw = (int)(lvl->ladders[i].w * scale);
            int lh = (int)(lvl->ladders[i].h * scale);
            if (lw < 2) lw = 2;
            if (lh < 2) lh = 2;
            SDL_Rect ml = {lx, ly, lw, lh};
            SDL_RenderFillRect(r, &ml);
        }
    }
    
    if (pm) {
        for (int i = 0; i < pm->count; i++) {
            Platform *p = &pm->platforms[i];
            if (!p->active) continue;
            
            int px = inner.x + offsetX + (int)(p->rect.x * scale);
            int py = inner.y + offsetY + (int)(p->rect.y * scale);
            int pw = (int)(p->rect.w * scale);
            int ph = (int)(p->rect.h * scale);
            
            if (pw < 1) pw = 1;
            if (ph < 1) ph = 1;
            
            if (px < inner.x + offsetX) px = inner.x + offsetX;
            if (py < inner.y + offsetY) py = inner.y + offsetY;
            if (px + pw > inner.x + offsetX + mapW) pw = inner.x + offsetX + mapW - px;
            if (py + ph > inner.y + offsetY + mapH) ph = inner.y + offsetY + mapH - py;
            
            SDL_Rect mp = {px, py, pw, ph};
            
            switch(p->type) {
                case PLATFORM_DESTRUCTIBLE: 
                    SDL_SetRenderDrawColor(r, 255, 80, 80, 255);
                    break;
                case PLATFORM_MOVING_H:
                case PLATFORM_MOVING_V: 
                    SDL_SetRenderDrawColor(r, 80, 200, 255, 255);
                    break;
                default: 
                    SDL_SetRenderDrawColor(r, 34, 139, 34, 255);
                    break;
            }
            SDL_RenderFillRect(r, &mp);
        }
    }
    
    int jx = inner.x + offsetX + (int)(player->x * scale);
    int jy = inner.y + offsetY + (int)(player->y * scale);
    
    int jw = (int)(PLAYER_WIDTH * scale);
    int jh = (int)(PLAYER_HEIGHT * scale);
    
    if (jw < 2) jw = 2;
    if (jh < 2) jh = 2;
    
    if (jx < inner.x + offsetX) jx = inner.x + offsetX;
    if (jx + jw > inner.x + offsetX + mapW) jx = inner.x + offsetX + mapW - jw;
    if (jy < inner.y + offsetY) jy = inner.y + offsetY;
    if (jy + jh > inner.y + offsetY + mapH) jy = inner.y + offsetY + mapH - jh;
    
    SDL_Rect mj = {jx, jy, jw, jh};
    
    SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
    SDL_RenderFillRect(r, &mj);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDrawRect(r, &mj);
    
    SDL_Rect levelInd = {mm->position.x + 5, mm->position.y + MINIMAP_HEIGHT + 5, 10, 10};
    if (lm->currentLevel == 0) {
        SDL_SetRenderDrawColor(r, 100, 200, 100, 255);
    } else {
        SDL_SetRenderDrawColor(r, 200, 100, 200, 255);
    }
    SDL_RenderFillRect(r, &levelInd);
}

void minimapToggle(Minimap *mm) {
    mm->visible = !mm->visible;
}

void minimapDestroy(Minimap *mm) {
    if (mm->levelPreview) SDL_DestroyTexture(mm->levelPreview);
}
