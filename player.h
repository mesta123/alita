#ifndef PLAYER_H
#define PLAYER_H

#include <SDL2/SDL.h>

#define PLAYER_WIDTH 24
#define PLAYER_HEIGHT 32
#define PLAYER_SPEED 4.5f  // ← MODIFIÉ: 3 → 4.5f
#define JUMP_FORCE 12
#define GRAVITY 0.6f
#define LADDER_SPEED 2.5f

#define ANIM_FRAMES 4
#define ANIM_SPEED 100

typedef enum {
    ANIM_IDLE,
    ANIM_WALK,
    ANIM_JUMP,
    ANIM_FALL,
    ANIM_CLIMB,
    ANIM_CLIMB_IDLE
} PlayerAnimState;

typedef struct {
    float x, y;
    float vx, vy;
    SDL_Rect rect;
    int onGround;
    int onLadder;
    int facingRight;
    PlayerAnimState anim;
    int frame;
    Uint32 lastAnimTime;
    SDL_Texture *spriteSheet;
    SDL_Rect frames[ANIM_FRAMES];
    int dead;
    int deathTimer;
} Player;

void playerInit(Player *p, SDL_Renderer *r, float startX, float startY);
void playerUpdate(Player *p, const Uint8 *keys, int levelWidth, int levelHeight, int onLadder);
void playerDraw(Player *p, SDL_Renderer *r, int camX, int camY);
void playerDestroy(Player *p);
void playerSetPosition(Player *p, float x, float y);
void playerForceGround(Player *p, float groundY);
int playerIsDead(Player *p);
void playerKill(Player *p);

#endif
