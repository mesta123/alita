#ifndef ENNEMI_H
#define ENNEMI_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

/* ── Sprite sheet ───────────────────────────────────────────────── */
#define FRAME_W    96
#define FRAME_H    96
#define SHEET_COLS  4

/* ── Arena ──────────────────────────────────────────────────────── */
#define SCREEN_W   800
#define SCREEN_H   600
#define GROUND_Y   450
#define CEIL_Y      40

/* ── Sprite display size ────────────────────────────────────────── */
#define DISP_W  80
#define DISP_H  80

/* ── Alita sprite rows ──────────────────────────────────────────── */
#define ALITA_ROW_IDLE   0
#define ALITA_ROW_WALK   1
#define ALITA_ROW_ATTACK 2
#define ALITA_ROW_JUMP   3

/* ── Zapan sprite rows ──────────────────────────────────────────── */
#define ZAPAN_ROW_IDLE   0
#define ZAPAN_ROW_WALK   1
#define ZAPAN_ROW_ATTACK 2
#define ZAPAN_ROW_DEATH  3

/* ── Enemy awareness radii ──────────────────────────────────────── */
#define RADIUS_ATTACK   90
#define RADIUS_CHASE   260

/* ── Animation states ───────────────────────────────────────────── */
typedef enum {
    ANIM_IDLE,
    ANIM_WALK,
    ANIM_ATTACK,
    ANIM_JUMP,
    ANIM_DEATH
} AnimState;

/* ── Enemy behaviour modes ──────────────────────────────────────── */
typedef enum {
    BEHAV_PATROL,
    BEHAV_CHASE,
    BEHAV_ATTACK
} BehavMode;

/* ── Level ──────────────────────────────────────────────────────── */
typedef enum {
    LEVEL_1 = 1,
    LEVEL_2 = 2
} GameLevel;

/* ── Game state ─────────────────────────────────────────────────── */
typedef enum {
    STATE_PLAYING,
    STATE_LEVEL_CLEAR,
    STATE_GAME_OVER,
    STATE_WIN
} GameState;

/* ── Player ─────────────────────────────────────────────────────── */
typedef struct {
    SDL_Texture *texture;
    SDL_Rect     src, dest;
    int          frame, frameTimer, frameDelay;
    AnimState    state;
    int          vx, vy, onGround, alive, facingRight;
    int          hp, maxHP;       /* maxHP = 5, dies at 0          */
    int          invincible;      /* i-frames after a hit          */
    int          hitLanded;       /* 1 = this attack already scored */
} Player;

/* ── Enemy ──────────────────────────────────────────────────────── */
typedef struct {
    SDL_Texture *texture;
    SDL_Rect     src, dest;
    int          frame, frameTimer, frameDelay;
    AnimState    state;
    BehavMode    behav;
    int          vx, vy;
    int          alive, facingRight;
    int          hp, maxHP;       /* maxHP = 5, dies at 0          */
    int          aerial;
    int          patrolTimer;
    int          patrolVx, patrolVy;
    int          attackCooldown;
    int          chaseTimer;
} Enemy;

/* ── Background ─────────────────────────────────────────────────── */
typedef struct {
    SDL_Texture *tex1;    /* level 1 background  */
    SDL_Texture *tex2;    /* level 2 background  */
    SDL_Rect     dest;
    GameLevel    level;
} Background;

/* ── Health bar ─────────────────────────────────────────────────── */
typedef struct {
    SDL_Rect  bar;
    SDL_Color color;
    SDL_Color bgColor;
    int       max, current;
} HealthBar;

/* ── Utility ────────────────────────────────────────────────────── */
SDL_Texture *loadTexture   (const char *file, SDL_Renderer *r);
void         setSrc        (SDL_Rect *src, int frame, int row);
int          checkCollision(SDL_Rect a, SDL_Rect b);

/* ── Enemy internal helpers (no static) ────────────────────────── */
int  centreDist   (SDL_Rect a, SDL_Rect b);
void newPatrolDir (Enemy *e);
void enforceMotion(Enemy *e);
void clampBounce  (Enemy *e);

/* ── Player ─────────────────────────────────────────────────────── */
void initPlayer    (Player *p, SDL_Renderer *r);
void setPlayerState(Player *p, AnimState s);
void handleInput   (Player *p, SDL_Event e);
void updatePlayer  (Player *p);
void renderPlayer  (Player  p, SDL_Renderer *r);

/* ── Enemy ──────────────────────────────────────────────────────── */
void initEnemy    (Enemy *e, SDL_Renderer *r, GameLevel lvl);
void setEnemyState(Enemy *e, AnimState s);
void updateEnemy  (Enemy *e, Player *p);
void renderEnemy  (Enemy  e, SDL_Renderer *r);

/* ── Background ─────────────────────────────────────────────────── */
void initBackground   (Background *bg, SDL_Renderer *r, GameLevel lvl);
void renderBackground (Background  bg, SDL_Renderer *r);

/* ── Health bar ─────────────────────────────────────────────────── */
void initHealth   (HealthBar *hb, int x, int maxHP, SDL_Color col, SDL_Color bg);
void updateHealth (HealthBar *hb, int current);
void renderHealth (HealthBar  hb, SDL_Renderer *r, const char *lbl, SDL_Renderer *ren);

#endif
