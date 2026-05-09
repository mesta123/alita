#ifndef ENIGME_H
#define ENIGME_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_rotozoom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ─── Window ─────────────────────────────────────────────────── */
#define WINDOW_W        1100
#define WINDOW_H        760

/* ─── Puzzle grid ────────────────────────────────────────────── */
#define GRID_COLS       4
#define GRID_ROWS       4
#define GRID_TOTAL      (GRID_COLS * GRID_ROWS)

/* Puzzle board displayed on screen */
#define BOARD_X         60
#define BOARD_Y         80
#define BOARD_W         560
#define BOARD_H         560
#define CELL_W          (BOARD_W / GRID_COLS)
#define CELL_H          (BOARD_H / GRID_ROWS)

/* Choice panel (right side) */
#define PANEL_X         700
#define PANEL_Y         120
#define CHOICE_W        170
#define CHOICE_H        170
#define CHOICE_SPACING  30
#define NUM_CHOICES     3

/* Time limit (seconds) */
#define TIME_LIMIT      60

/* Total rounds per game */
#define TOTAL_ROUNDS    5

/* ─── States ─────────────────────────────────────────────────── */
typedef enum {
    STATE_LOADING,
    STATE_PLAYING,
    STATE_SUCCESS,
    STATE_FAILURE
} GameState;

/* ─── One choice option ──────────────────────────────────────── */
typedef struct {
    SDL_Texture *tex;
    int          is_correct;
    int          sx, sy;          /* resting position */
    int          dragging;
    int          drag_x, drag_y;
    int          drag_off_x, drag_off_y;
} Choice;

/* ─── Game context ───────────────────────────────────────────── */
typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;

    SDL_Texture  *full_images[5];

    int           round;
    int           img_idx;
    int           missing_col;
    int           missing_row;

    SDL_Texture  *cells[GRID_TOTAL];

    Choice        choices[NUM_CHOICES];
    int           correct_choice;

    int           piece_placed;
    int           placed_correct;
    SDL_Texture  *placed_tex;

    Uint32        round_start;
    float         elapsed;

    GameState     state;
    float         anim_timer;
    float         load_angle;
    float         rotozoom_angle;
    float         rotozoom_scale;

    int           score;
} Game;

/* ─── Prototypes ─────────────────────────────────────────────── */
void game_init(Game *g);
void game_load_images(Game *g);
void game_start_round(Game *g);
void game_free_round(Game *g);
void game_free(Game *g);

void game_update(Game *g, float dt);
void game_draw(Game *g);

void game_on_mouse_down(Game *g, int x, int y);
void game_on_mouse_up(Game *g, int x, int y);
void game_on_mouse_move(Game *g, int x, int y);

SDL_Texture *crop_texture(SDL_Renderer *r, SDL_Texture *src,
                           SDL_Rect src_rect, int dst_w, int dst_h);
SDL_Texture *rotozoom_tex(SDL_Renderer *r, SDL_Texture *src,
                           double angle, double scale);
SDL_Rect     cell_rect(int col, int row);
void         draw_glow_rect(SDL_Renderer *r, SDL_Rect rect,
                             Uint8 R, Uint8 G, Uint8 B, int layers);

#endif
