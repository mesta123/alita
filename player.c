#include "player.h"
#include "level.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>

static int jumpFrames = 0;

void playerInit(Player *p, SDL_Renderer *r, float startX, float startY) {
    (void)r;
    
    p->x = startX;
    p->y = startY;
    p->vx = 0;
    p->vy = 0;
    p->rect.w = PLAYER_WIDTH;
    p->rect.h = PLAYER_HEIGHT;
    p->onGround = 1;
    p->facingRight = 1;
    p->anim = ANIM_IDLE;
    p->frame = 0;
    p->lastAnimTime = 0;
    p->spriteSheet = NULL;
    p->onLadder = 0;
    p->dead = 0;
    p->deathTimer = 0;
    jumpFrames = 0;
    
    p->rect.x = (int)startX;
    p->rect.y = (int)startY;
    
    printf("Player init: x=%.0f y=%.0f\n", startX, startY);
}

void playerUpdate(Player *p, const Uint8 *keys, int levelWidth, int levelHeight, int onLadder) {
    if (p->dead) {
        p->deathTimer--;
        if (p->deathTimer <= 0) {
            p->dead = 0;
            p->x = 50;
            p->y = 200;
            p->vx = 0;
            p->vy = 0;
        }
        return;
    }
    
    // Gestion échelle - prioritaire
    if (onLadder) {
        p->onLadder = 1;
        p->vx = 0;
        p->vy = 0;
        
        // Monter
        if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_Z]) {
            p->vy = -LADDER_SPEED;
            p->anim = ANIM_CLIMB;
        }
        // Descendre
        else if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) {
            p->vy = LADDER_SPEED;
            p->anim = ANIM_CLIMB;
        }
        // Immobile sur échelle
        else {
            p->vy = 0;
            p->anim = ANIM_CLIMB_IDLE;
        }
        
        // Sortir de l'échelle avec gauche/droite
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_Q] || keys[SDL_SCANCODE_A]) {
            p->onLadder = 0;
            p->vx = -PLAYER_SPEED;  // ← utilise 4.5f maintenant
            p->facingRight = 0;
        }
        else if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
            p->onLadder = 0;
            p->vx = PLAYER_SPEED;  // ← utilise 4.5f maintenant
            p->facingRight = 1;
        }
        
        p->onGround = 0;
    } else {
        // Mouvement normal
        p->onLadder = 0;
        
        int jumpPressed = keys[SDL_SCANCODE_SPACE] || 
                          keys[SDL_SCANCODE_Z] || 
                          keys[SDL_SCANCODE_W] ||
                          keys[SDL_SCANCODE_UP];
        
        if (jumpPressed && p->onGround && jumpFrames == 0) {
            p->vy = -JUMP_FORCE;
            p->onGround = 0;
            jumpFrames = 15;
        }
        
        if (jumpFrames > 0) {
            jumpFrames--;
        }
        
        p->vx = 0;
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_Q] || keys[SDL_SCANCODE_A]) {
            p->vx = -PLAYER_SPEED;  // ← utilise 4.5f maintenant
            p->facingRight = 0;
        }
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
            p->vx = PLAYER_SPEED;  // ← utilise 4.5f maintenant
            p->facingRight = 1;
        }
        
        p->vy += GRAVITY;
        if (p->vy > 15) p->vy = 15;
    }
    
    // Appliquer mouvement
    p->x += p->vx;
    p->y += p->vy;
    
    // Limites
    if (p->x < 0) p->x = 0;
    if (p->x > levelWidth - PLAYER_WIDTH) p->x = levelWidth - PLAYER_WIDTH;
    if (p->y < 0) {
        p->y = 0;
        p->vy = 0;
    }
    
    if (p->y > levelHeight + 100) {
        p->dead = 1;
        p->deathTimer = 60;
        printf("GAME OVER! Le joueur est tombe!\n");
    }
    
    p->rect.x = (int)p->x;
    p->rect.y = (int)p->y;
    
    // Animation
    if (p->onLadder) {
        if (p->vy != 0) p->anim = ANIM_CLIMB;
        else p->anim = ANIM_CLIMB_IDLE;
    } else if (!p->onGround) {
        if (p->vy < 0) p->anim = ANIM_JUMP;
        else p->anim = ANIM_FALL;
    } else if (p->vx != 0) {
        p->anim = ANIM_WALK;
    } else {
        p->anim = ANIM_IDLE;
    }
    
    Uint32 now = SDL_GetTicks();
    if (now - p->lastAnimTime > ANIM_SPEED) {
        p->frame = (p->frame + 1) % ANIM_FRAMES;
        p->lastAnimTime = now;
    }
}

void playerDraw(Player *p, SDL_Renderer *r, int camX, int camY) {
    if (p->dead) return;
    
    int screenX = (int)p->x - camX;
    int screenY = (int)p->y - camY;
    
    if (screenX < -PLAYER_WIDTH || screenX > GAME_WIDTH || 
        screenY < -PLAYER_HEIGHT || screenY > GAME_HEIGHT) {
        return;
    }
    
    SDL_Rect dst = {screenX, screenY, PLAYER_WIDTH, PLAYER_HEIGHT};
    
    switch(p->anim) {
        case ANIM_CLIMB:
        case ANIM_CLIMB_IDLE:
            SDL_SetRenderDrawColor(r, 255, 165, 0, 255); // Orange
            break;
        case ANIM_JUMP:
            SDL_SetRenderDrawColor(r, 100, 200, 255, 255);
            break;
        case ANIM_FALL:
            SDL_SetRenderDrawColor(r, 255, 100, 100, 255);
            break;
        default:
            SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
    }
    
    SDL_RenderFillRect(r, &dst);
    
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    for (int i = 0; i < 2; i++) {
        SDL_Rect border = {screenX + i, screenY + i, PLAYER_WIDTH - 2*i, PLAYER_HEIGHT - 2*i};
        SDL_RenderDrawRect(r, &border);
    }
    
    int eyeX = p->facingRight ? screenX + 16 : screenX + 4;
    SDL_Rect eye = {eyeX, screenY + 8, 4, 6};
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &eye);
    
    // Indicateur échelle
    if (p->onLadder) {
        SDL_Rect indicator = {screenX + 8, screenY - 10, 8, 8};
        SDL_SetRenderDrawColor(r, 255, 255, 0, 255);
        SDL_RenderFillRect(r, &indicator);
    }
}

void playerDestroy(Player *p) {
    if (p->spriteSheet) SDL_DestroyTexture(p->spriteSheet);
}

void playerSetPosition(Player *p, float x, float y) {
    p->x = x;
    p->y = y;
    p->rect.x = (int)x;
    p->rect.y = (int)y;
}

void playerForceGround(Player *p, float groundY) {
    p->y = groundY - PLAYER_HEIGHT;
    p->vy = 0;
    p->onGround = 1;
    p->rect.y = (int)p->y;
}

int playerIsDead(Player *p) {
    return p->dead;
}

void playerKill(Player *p) {
    p->dead = 1;
    p->deathTimer = 60;
}
