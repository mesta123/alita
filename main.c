#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "ennemi.h"

/* ── Damage tuning ──────────────────────────────────────────────── */
/* Both player and enemy have 5 HP and die after exactly 5 hits.
   - Player attack: 1 hit scored per full attack animation (hitLanded flag).
   - Enemy attack:  1 hit scored per contact, gated by invincible frames.
   - INVINCIBLE_F must be >= one full attack animation length so a single
     swing can never land twice.                                         */
#define INVINCIBLE_F   60    /* i-frames after player is hit (~1 attack cycle) */

/* ── Simple overlay helpers ─────────────────────────────────────── */
static void drawOverlay(SDL_Renderer *r, Uint8 alpha, Uint8 rr, Uint8 gg, Uint8 bb) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, rr, gg, bb, alpha);
    SDL_Rect full = {0, 0, SCREEN_W, SCREEN_H};
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ── Minimal 5×7 pixel font ─────────────────────────────────────
   Each letter is a 5-column × 7-row bitmap stored as 7 bytes,
   one byte per row, bits 4..0 = columns left to right.
   We scale up by GLYPH_SCALE to get big chunky letters.
   ─────────────────────────────────────────────────────────────── */
#define GLYPH_SCALE  10   /* each pixel → 10×10 screen pixels */
#define GLYPH_W       5
#define GLYPH_H       7
#define GLYPH_GAP     2   /* gap between letters in pixel-units */

/* Letters used: B I N G O L E R S 2 V (for BINGO LOSER LEVEL2) */
static const Uint8 GLYPHS[][GLYPH_H] = {
    /* 0: B */ {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    /* 1: I */ {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F},
    /* 2: N */ {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    /* 3: G */ {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E},
    /* 4: O */ {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    /* 5: L */ {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    /* 6: S */ {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E},
    /* 7: E */ {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    /* 8: R */ {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    /* 9: 2 */ {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F},
    /*10: V */ {0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04},
    /*11: space*/{0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

/* Draw one glyph at pixel position (px, py) */
static void drawGlyph(SDL_Renderer *r, int idx, int px, int py) {
    int s = GLYPH_SCALE;
    for (int row = 0; row < GLYPH_H; row++) {
        for (int col = 0; col < GLYPH_W; col++) {
            if (GLYPHS[idx][row] & (0x10 >> col)) {
                SDL_Rect dot = {px + col*s, py + row*s, s, s};
                SDL_RenderFillRect(r, &dot);
            }
        }
    }
}

/* Draw a string of glyph indices, centred vertically at cy */
static void drawWord(SDL_Renderer *r, const int *indices, int len,
                     SDL_Color col, int cy) {
    int s   = GLYPH_SCALE;
    int gap = GLYPH_GAP * s;
    int totalW = len * (GLYPH_W * s) + (len - 1) * gap;
    int startX = (SCREEN_W - totalW) / 2;
    int startY = cy - (GLYPH_H * s) / 2;

    /* Shadow pass (dark, offset 2px) */
    SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
    for (int i = 0; i < len; i++)
        drawGlyph(r, indices[i], startX + i*(GLYPH_W*s + gap) + 2, startY + 2);

    /* Main colour pass */
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    for (int i = 0; i < len; i++)
        drawGlyph(r, indices[i], startX + i*(GLYPH_W*s + gap), startY);
}

/* msgID: 0 = LEVEL 2, 1 = BINGO, 2 = LOSER */
static void drawBigMsg(SDL_Renderer *r, int msgID) {
    /* Dark band behind the text */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 200);
    int bandH = GLYPH_H * GLYPH_SCALE + 40;
    SDL_Rect band = {0, SCREEN_H/2 - bandH/2, SCREEN_W, bandH};
    SDL_RenderFillRect(r, &band);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    int cy = SCREEN_H / 2;

    if (msgID == 0) {
        /* "LEVEL 2" — cyan */
        SDL_Color c = {80, 220, 255, 255};
        /* L E V E L   2  →  indices: 5 7 10 7 5 11 9 */
        int idx[] = {5, 7, 10, 7, 5, 11, 9};
        drawWord(r, idx, 7, c, cy);

    } else if (msgID == 1) {
        /* "BINGO" — bright gold */
        SDL_Color c = {255, 215, 0, 255};
        /* B I N G O  →  0 1 2 3 4 */
        int idx[] = {0, 1, 2, 3, 4};
        drawWord(r, idx, 5, c, cy);
        /* Sub-line: press R to restart */
        SDL_SetRenderDrawColor(r, 200, 200, 100, 200);
        SDL_RenderDrawLine(r, SCREEN_W/2-80, cy + GLYPH_H*GLYPH_SCALE/2 + 18,
                              SCREEN_W/2+80, cy + GLYPH_H*GLYPH_SCALE/2 + 18);

    } else {
        /* "LOSER" — blood red */
        SDL_Color c = {255, 50, 50, 255};
        /* L O S E R  →  5 4 6 7 8 */
        int idx[] = {5, 4, 6, 7, 8};
        drawWord(r, idx, 5, c, cy);
        /* Sub-line */
        SDL_SetRenderDrawColor(r, 180, 60, 60, 200);
        SDL_RenderDrawLine(r, SCREEN_W/2-80, cy + GLYPH_H*GLYPH_SCALE/2 + 18,
                              SCREEN_W/2+80, cy + GLYPH_H*GLYPH_SCALE/2 + 18);
    }
}

static void drawLevelBanner(SDL_Renderer *r, GameLevel lvl) {
    SDL_Color c = (lvl == LEVEL_1) ? (SDL_Color){60,220,120,220} : (SDL_Color){160,60,255,220};
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect top = {0, 0, SCREEN_W, 11};
    SDL_RenderFillRect(r, &top);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ── Level setup ────────────────────────────────────────────────── */
static void setupLevel(GameLevel lvl, Background *bg, Player *p,
                       Enemy *e, HealthBar *phb, HealthBar *ehb,
                       SDL_Renderer *ren) {
    SDL_Color green  = {80, 220, 80, 255};
    SDL_Color red    = {220, 60, 60, 255};
    SDL_Color purple = {180, 60,255, 255};
    SDL_Color dark   = {20,  20, 20, 200};

    initBackground(bg, ren, lvl);

    p->dest.x   = 80;
    p->dest.y   = GROUND_Y - DISP_H;
    p->vx = p->vy = 0;
    p->onGround = 1;
    p->alive    = 1;
    p->invincible = 0;
    setPlayerState(p, ANIM_IDLE);

    initEnemy(e, ren, lvl);

    SDL_Color ec = (lvl == LEVEL_1) ? red : purple;
    initHealth(phb, 20,  p->maxHP, green, dark);
    initHealth(ehb, 600, e->maxHP, ec,    dark);
    updateHealth(phb, p->hp);
}

/* ═══════════════════════════════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════════════════════════════ */
int main(void) {
    srand((unsigned int)time(NULL));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init: %s\n", SDL_GetError()); return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        printf("IMG_Init: %s\n", IMG_GetError()); return 1;
    }

    SDL_Window *win = SDL_CreateWindow(
        "Alita vs Zapan  –  2 Levels",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, 0);
    if (!win) { printf("Window: %s\n", SDL_GetError()); return 1; }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { printf("Renderer: %s\n", SDL_GetError()); return 1; }

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    Background bg;
    Player     p;
    Enemy      e;
    HealthBar  phb, ehb;

    /* zero the bg struct so tex1/tex2 are NULL before first load */
    bg.tex1 = NULL;
    bg.tex2 = NULL;

    GameLevel  lvl      = LEVEL_1;
    GameState  gstate   = STATE_PLAYING;
    int        transTimer = 0;

    initPlayer(&p, ren);
    setupLevel(lvl, &bg, &p, &e, &phb, &ehb, ren);

    SDL_Event ev;
    int running = 1;

    while (running) {

        /* ── Events ─────────────────────────────────────────── */
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = 0;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
                running = 0;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_r) {
                if (gstate == STATE_GAME_OVER || gstate == STATE_WIN) {
                    lvl = LEVEL_1; gstate = STATE_PLAYING; transTimer = 0;
                    initPlayer(&p, ren);
                    setupLevel(lvl, &bg, &p, &e, &phb, &ehb, ren);
                }
            }
            if (gstate == STATE_PLAYING) handleInput(&p, ev);
        }

        /* ── Update ─────────────────────────────────────────── */
        if (gstate == STATE_PLAYING) {
            updatePlayer(&p);
            updateEnemy(&e, &p);

            /* ── Player attacks enemy: 1 hit per attack animation ── */
            if (p.state == ANIM_ATTACK && !p.hitLanded &&
                e.alive && checkCollision(p.dest, e.dest)) {
                p.hitLanded = 1;        /* lock out further hits this swing */
                e.hp -= 1;
                if (e.hp <= 0) { e.hp = 0; setEnemyState(&e, ANIM_DEATH); }
            }

            /* ── Enemy hits player: 1 hit per contact window ───── */
            if (e.alive && p.alive && p.invincible == 0 &&
                checkCollision(e.dest, p.dest)) {
                p.hp -= 1;
                p.invincible = INVINCIBLE_F;  /* can't be hit again until frames expire */
                if (p.hp <= 0) { p.hp = 0; p.alive = 0; }
            }

            updateHealth(&phb, p.hp);
            updateHealth(&ehb, e.hp);

            if (!p.alive) {
                gstate = STATE_GAME_OVER;
            } else if (!e.alive) {
                gstate     = (lvl == LEVEL_1) ? STATE_LEVEL_CLEAR : STATE_WIN;
                transTimer = 130;
            }

        } else if (gstate == STATE_LEVEL_CLEAR) {
            transTimer--;
            if (transTimer <= 0) {
                lvl = LEVEL_2; gstate = STATE_PLAYING;
                setupLevel(lvl, &bg, &p, &e, &phb, &ehb, ren);
            }
        }

        /* ── Render ─────────────────────────────────────────── */
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        renderBackground(bg, ren);
        renderEnemy(e, ren);
        renderPlayer(p, ren);
        renderHealth(phb, ren, "ALITA", ren);
        renderHealth(ehb, ren, "ZAPAN", ren);
        drawLevelBanner(ren, lvl);

        if (gstate == STATE_LEVEL_CLEAR) {
            Uint8 a = (Uint8)(200 * (1.0f - (float)transTimer / 130.0f));
            drawOverlay(ren, a, 15, 5, 45);
            drawBigMsg(ren, 0);
        } else if (gstate == STATE_WIN) {
            drawOverlay(ren, 160, 0, 15, 0);
            drawBigMsg(ren, 1);
        } else if (gstate == STATE_GAME_OVER) {
            drawOverlay(ren, 160, 40, 0, 0);
            drawBigMsg(ren, 2);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if (p.texture)   SDL_DestroyTexture(p.texture);
    if (e.texture)   SDL_DestroyTexture(e.texture);
    if (bg.tex1)     SDL_DestroyTexture(bg.tex1);
    if (bg.tex2)     SDL_DestroyTexture(bg.tex2);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
