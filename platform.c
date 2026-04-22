#include "platform.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

void platformManagerInit(PlatformManager *pm, SDL_Renderer *r) {
    (void)r;
    pm->count = 0;
}

void platformManagerAdd(PlatformManager *pm, PlatformType type,
                        float x, float y, float w, float h,
                        float ex, float ey, float speed) {
    if (pm->count >= MAX_PLATFORMS) return;
    
    Platform *p = &pm->platforms[pm->count];
    p->rect.x = (int)x;
    p->rect.y = (int)y;
    p->rect.w = (int)w;
    p->rect.h = (int)h;
    p->type = type;
    p->startX = x;
    p->startY = y;
    p->endX = ex;
    p->endY = ey;
    p->speed = speed;
    p->currentPos = 0.0f;
    p->direction = 1;
    p->active = 1;
    p->hitPoints = (type == PLATFORM_DESTRUCTIBLE) ? 3 : 999;
    p->shakeFrames = 0;
    p->texture = NULL;
    
    pm->count++;
}

void platformManagerUpdate(PlatformManager *pm) {
    for (int i = 0; i < pm->count; i++) {
        Platform *p = &pm->platforms[i];
        if (!p->active) continue;
        
        if (p->shakeFrames > 0) {
            p->shakeFrames--;
            p->rect.x = (int)p->startX + (rand() % 6 - 3);
        } else {
            p->rect.x = (int)p->startX;
        }
        
        if (p->type == PLATFORM_MOVING_H || p->type == PLATFORM_MOVING_V) {
            p->currentPos += p->speed * p->direction;
            if (p->currentPos >= 1.0f) { p->currentPos = 1.0f; p->direction = -1; }
            else if (p->currentPos <= 0.0f) { p->currentPos = 0.0f; p->direction = 1; }
            
            p->rect.x = (int)lerp(p->startX, p->endX, p->currentPos);
            p->rect.y = (int)lerp(p->startY, p->endY, p->currentPos);
        }
    }
}

void platformManagerCheckCollisions(PlatformManager *pm, Player *player, LevelManager *lm) {
    (void)lm;
    
    // === COLLISION HORIZONTALE (mur) - Uniquement pour niveau 1 ===
    if (lm->currentLevel == 0 && !player->onLadder) {
        for (int i = 0; i < pm->count; i++) {
            Platform *p = &pm->platforms[i];
            if (!p->active) continue;
            
            // Vérifier collision horizontale (mur)
            SDL_Rect playerRect = player->rect;
            int prevX = (int)(player->x - player->vx); // Position précédente
            
            // Détection mur gauche: joueur arrive de la gauche et touche le côté droit de la plateforme
            if (player->vx > 0 && 
                playerRect.x + playerRect.w > p->rect.x &&
                playerRect.x < p->rect.x &&
                playerRect.y + playerRect.h > p->rect.y + 5 &&  // +5 pour permettre de marcher sur le dessus
                playerRect.y < p->rect.y + p->rect.h - 5) {
                
                // Bloquer le mouvement à droite
                player->x = p->rect.x - PLAYER_WIDTH - 0.1f;
                player->vx = 0;
                player->rect.x = (int)player->x;
            }
            
            // Détection mur droit: joueur arrive de la droite et touche le côté gauche de la plateforme
            if (player->vx < 0 &&
                playerRect.x < p->rect.x + p->rect.w &&
                playerRect.x + playerRect.w > p->rect.x + p->rect.w &&
                playerRect.y + playerRect.h > p->rect.y + 5 &&
                playerRect.y < p->rect.y + p->rect.h - 5) {
                
                // Bloquer le mouvement à gauche
                player->x = p->rect.x + p->rect.w + 0.1f;
                player->vx = 0;
                player->rect.x = (int)player->x;
            }
        }
    }
    
    // === COLLISION VERTICALE (sol/plafond) ===
    if (player->onLadder && player->vy < 0) return;
    
    // Collision avec le plafond (tête qui tape)
    if (player->vy < -2) {
        for (int i = 0; i < pm->count; i++) {
            Platform *p = &pm->platforms[i];
            if (!p->active) continue;
            
            if (player->rect.y <= p->rect.y + p->rect.h &&
                player->rect.y >= p->rect.y + p->rect.h - 10 &&
                player->rect.x + player->rect.w > p->rect.x &&
                player->rect.x < p->rect.x + p->rect.w) {
                player->y = p->rect.y + p->rect.h;
                player->vy = 0;
                player->rect.y = (int)player->y;
            }
        }
        return;
    }
    
    // Atterrissage sur plateforme
    for (int i = 0; i < pm->count; i++) {
        Platform *p = &pm->platforms[i];
        if (!p->active) continue;
        
        int playerBottom = player->rect.y + player->rect.h;
        int platformTop = p->rect.y;
        
        if (player->vy >= 0 &&
            playerBottom >= platformTop - 5 &&
            playerBottom <= platformTop + 10 &&
            player->rect.x + player->rect.w > p->rect.x &&
            player->rect.x < p->rect.x + p->rect.w &&
            player->rect.y < platformTop) {
            
            player->y = platformTop - PLAYER_HEIGHT;
            player->vy = 0;
            player->onGround = 1;
            player->onLadder = 0;
            
            if (p->type == PLATFORM_MOVING_H) {
                player->x += (p->endX - p->startX) * p->speed * p->direction;
            }
            
            player->rect.x = (int)player->x;
            player->rect.y = (int)player->y;
        }
    }
}

void platformManagerRender(PlatformManager *pm, SDL_Renderer *r, int camX, int camY) {
    for (int i = 0; i < pm->count; i++) {
        Platform *p = &pm->platforms[i];
        if (!p->active) continue;
        
        int screenX = p->rect.x - camX;
        int screenY = p->rect.y - camY;
        
        if (screenX + p->rect.w < 0 || screenX > GAME_WIDTH ||
            screenY + p->rect.h < 0 || screenY > GAME_HEIGHT) {
            continue;
        }
        
        SDL_Rect dst = {screenX, screenY, p->rect.w, p->rect.h};
        
        switch(p->type) {
            case PLATFORM_DESTRUCTIBLE: SDL_SetRenderDrawColor(r, 180, 90, 40, 255); break;
            case PLATFORM_MOVING_H:
            case PLATFORM_MOVING_V: SDL_SetRenderDrawColor(r, 50, 150, 255, 255); break;
            default: SDL_SetRenderDrawColor(r, 34, 139, 34, 255); break;
        }
        
        SDL_RenderFillRect(r, &dst);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderDrawRect(r, &dst);
        
        if (p->type == PLATFORM_MOVING_H || p->type == PLATFORM_MOVING_V) {
            SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
            int centerX = dst.x + dst.w / 2;
            int centerY = dst.y + dst.h / 2;
            SDL_RenderDrawLine(r, centerX - 3, centerY, centerX + 3, centerY);
            SDL_RenderDrawLine(r, centerX, centerY - 3, centerX, centerY + 3);
        }
    }
}

void platformDamage(PlatformManager *pm, int index, int damage) {
    if (index < 0 || index >= pm->count) return;
    Platform *p = &pm->platforms[index];
    p->hitPoints -= damage;
    p->shakeFrames = 10;
    if (p->hitPoints <= 0) p->active = 0;
}

void platformManagerDestroy(PlatformManager *pm) {
    (void)pm;
}

void platformReset(PlatformManager *pm) {
    for (int i = 0; i < pm->count; i++) {
        Platform *p = &pm->platforms[i];
        p->active = 1;
        p->hitPoints = (p->type == PLATFORM_DESTRUCTIBLE) ? 3 : 999;
        p->currentPos = 0.0f;
        p->direction = 1;
        p->rect.x = (int)p->startX;
        p->rect.y = (int)p->startY;
    }
}
