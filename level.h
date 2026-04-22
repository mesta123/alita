#ifndef LEVEL_H
#define LEVEL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "player.h"
#include "utils.h"

#define MAX_LEVELS 3
#define PIXEL_COLLISION_THRESHOLD 128
#define MAX_LADDERS 5

#define GAME_WIDTH 650
#define GAME_HEIGHT 350

typedef struct {
    int x, y, w, h;
    int active;
} Ladder;

typedef struct {
    int x, y, w, h;
    int active;
    int triggered;
} LevelEndPortal;

typedef struct {
    int id;
    SDL_Texture *bgTexture;
    SDL_Surface *collisionMap;
    int width, height;
    char bgFile[64];
    Ladder ladders[MAX_LADDERS];
    int ladderCount;
    LevelEndPortal portal;
    int hasSafetyNet;
} Level;

typedef struct {
    Level levels[MAX_LEVELS];
    int currentLevel;
    int cameraX, cameraY;
    int shakeTimer;
    int shakeIntensity;
    int showLevelUp;
    int levelUpTimer;
    int transitioning;
} LevelManager;

void levelManagerInit(LevelManager *lm, SDL_Renderer *r);
void levelManagerLoad(LevelManager *lm, SDL_Renderer *r, int levelId);
void levelManagerUpdate(LevelManager *lm, Player *player, SDL_Renderer *r);  // Ajout renderer
void levelManagerRender(LevelManager *lm, SDL_Renderer *r);
void levelManagerDestroy(LevelManager *lm);

int levelCheckPixelCollision(LevelManager *lm, int x, int y);
void levelResolvePlayerCollision(LevelManager *lm, Player *player);
void levelTriggerShake(LevelManager *lm, int intensity, int duration);
void levelSwitchTo(LevelManager *lm, SDL_Renderer *r, int levelId);
int levelGetCurrentId(LevelManager *lm);
int levelCheckLadder(LevelManager *lm, Player *player);
int levelCheckPortal(LevelManager *lm, Player *player);
void levelStartLevelUp(LevelManager *lm, SDL_Renderer *r);  // Ajout renderer

#endif
