#ifndef PLATFORM_H
#define PLATFORM_H

#include <SDL2/SDL.h>
#include "player.h"
#include "level.h"

#define MAX_PLATFORMS 20

typedef enum {
    PLATFORM_STATIC,
    PLATFORM_MOVING_H,
    PLATFORM_MOVING_V,
    PLATFORM_DESTRUCTIBLE
} PlatformType;

typedef struct {
    SDL_Rect rect;
    PlatformType type;
    float startX, startY;
    float endX, endY;
    float speed;
    float currentPos;
    int direction;
    int active;
    SDL_Texture *texture;
    int hitPoints;
    int shakeFrames;
} Platform;

typedef struct {
    Platform platforms[MAX_PLATFORMS];
    int count;
    SDL_Texture *texMobile;
    SDL_Texture *texDestructible;
} PlatformManager;

void platformManagerInit(PlatformManager *pm, SDL_Renderer *r);
void platformManagerAdd(PlatformManager *pm, PlatformType type, 
                        float x, float y, float w, float h,
                        float ex, float ey, float speed);
void platformManagerUpdate(PlatformManager *pm);
void platformManagerRender(PlatformManager *pm, SDL_Renderer *r, int camX, int camY);
void platformManagerCheckCollisions(PlatformManager *pm, Player *player, LevelManager *lm);
void platformManagerDestroy(PlatformManager *pm);
void platformDamage(PlatformManager *pm, int index, int damage);
void platformReset(PlatformManager *pm);

#endif
