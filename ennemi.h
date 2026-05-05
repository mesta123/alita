#ifndef ENNEMI_H
#define ENNEMI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define FRAME_W    96
#define FRAME_H    96
#define SHEET_COLS  4

#define GROUND_Y   370

#define ALITA_ROW_IDLE   0
#define ALITA_ROW_WALK   1
#define ALITA_ROW_ATTACK 2
#define ALITA_ROW_JUMP   3

#define ZAPAN_ROW_IDLE   0
#define ZAPAN_ROW_WALK   1
#define ZAPAN_ROW_ATTACK 2
#define ZAPAN_ROW_DEATH  3

typedef enum {
    ANIM_IDLE,
    ANIM_WALK,
    ANIM_ATTACK,
    ANIM_JUMP,
    ANIM_DEATH
} AnimState;

typedef struct {
    SDL_Texture *texture;
    SDL_Rect     src, dest;
    int          frame, frameTimer, frameDelay;
    AnimState    state;
    int          vx, vy, onGround, alive, facingRight;
} Player;

typedef struct {
    SDL_Texture *texture;
    SDL_Rect     src, dest;
    int          frame, frameTimer, frameDelay;
    AnimState    state;
    int          vx, alive, facingRight;
} Enemy;

typedef struct {
    SDL_Texture *texture;
    SDL_Rect     dest;
} Background;

typedef struct {
    SDL_Rect  bar;
    SDL_Color color;
    SDL_Color bgColor;
    int       max;
    int       current;
} HealthBar;

/* Player */
void initPlayer     (Player *p, SDL_Renderer *r);
void handleInput    (Player *p, SDL_Event e);
void updatePlayer   (Player *p);
void renderPlayer   (Player  p, SDL_Renderer *r);
void setPlayerState (Player *p, AnimState s);

/* Enemy */
void initEnemy     (Enemy *e, SDL_Renderer *r);
void updateEnemy   (Enemy *e, Player *p);
void renderEnemy   (Enemy  e, SDL_Renderer *r);
void setEnemyState (Enemy *e, AnimState s);

/* Background */
void initBackground   (Background *bg, SDL_Renderer *r);
void renderBackground (Background  bg, SDL_Renderer *r);

/* Health bar */
void initHealth   (HealthBar *hb, int x, SDL_Color color, SDL_Color bgColor);
void renderHealth (HealthBar  hb, SDL_Renderer *r);

/* Utility */
int checkCollision (SDL_Rect a, SDL_Rect b);

#endif
