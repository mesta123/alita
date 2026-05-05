#include "ennemi.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define DISP_W  80
#define DISP_H  80

/* ── Internal helpers (no static, as requested) ─────────────────── */

void setSrc(SDL_Rect *src, int frame, int row) {
    src->x = frame * FRAME_W;
    src->y = row   * FRAME_H;
    src->w = FRAME_W;
    src->h = FRAME_H;
}

SDL_Texture *loadTexture(const char *file, SDL_Renderer *r) {
    SDL_Surface *s = IMG_Load(file);
    if (!s) {
        printf("ERROR: Cannot load %s -> %s\n", file, IMG_GetError());
        return NULL;
    }
    printf("Loaded %s (%dx%d)\n", file, s->w, s->h);
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    return t;
}

/* ── Player ──────────────────────────────────────────────────────── */

void setPlayerState(Player *p, AnimState s) {
    if (p->state == s) return;
    p->state = s; p->frame = 0; p->frameTimer = 0;
}

void initPlayer(Player *p, SDL_Renderer *renderer) {
    p->texture     = loadTexture("alita.png", renderer);
    p->dest        = (SDL_Rect){80, GROUND_Y - DISP_H, DISP_W, DISP_H};
    p->vx          = 0;
    p->vy          = 0;
    p->onGround    = 1;
    p->frame       = 0;
    p->frameTimer  = 0;
    p->frameDelay  = 8;
    p->state       = ANIM_IDLE;
    p->alive       = 1;
    p->facingRight = 1;
    setSrc(&p->src, 0, ALITA_ROW_IDLE);
}

void handleInput(Player *p, SDL_Event e) {
    if (!p->alive) return;

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_RIGHT:
                p->vx = 5; p->facingRight = 1;
                setPlayerState(p, ANIM_WALK); break;
            case SDLK_LEFT:
                p->vx = -5; p->facingRight = 0;
                setPlayerState(p, ANIM_WALK); break;
            case SDLK_SPACE:
                if (p->onGround) {
                    p->vy = -16; p->onGround = 0;
                    setPlayerState(p, ANIM_JUMP);
                }
                break;
            case SDLK_z: case SDLK_LCTRL:
                setPlayerState(p, ANIM_ATTACK); break;
        }
    }
    if (e.type == SDL_KEYUP) {
        if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_RIGHT) {
            p->vx = 0;
            if (p->onGround && p->state == ANIM_WALK)
                setPlayerState(p, ANIM_IDLE);
        }
    }
}

void updatePlayer(Player *p) {
    p->dest.x += p->vx;
    p->vy += 1;  /* gravity */
    p->dest.y += p->vy;

    int ground = GROUND_Y - DISP_H;
    if (p->dest.y >= ground) {
        p->dest.y = ground;
        p->vy = 0;
        if (!p->onGround) {
            p->onGround = 1;
            if (p->state == ANIM_JUMP)
                setPlayerState(p, p->vx != 0 ? ANIM_WALK : ANIM_IDLE);
        }
    }

    if (p->dest.x < 0) p->dest.x = 0;
    if (p->dest.x > 800 - DISP_W) p->dest.x = 800 - DISP_W;

    int row;
    switch (p->state) {
        case ANIM_WALK:   row = ALITA_ROW_WALK;   break;
        case ANIM_ATTACK: row = ALITA_ROW_ATTACK; break;
        case ANIM_JUMP:   row = ALITA_ROW_JUMP;   break;
        default:          row = ALITA_ROW_IDLE;   break;
    }

    p->frameTimer++;
    if (p->frameTimer >= p->frameDelay) {
        p->frameTimer = 0;
        p->frame++;
        if (p->state == ANIM_ATTACK) {
            if (p->frame >= SHEET_COLS) {
                p->frame = 0;
                setPlayerState(p, p->vx != 0 ? ANIM_WALK : ANIM_IDLE);
            }
        } else {
            if (p->frame >= SHEET_COLS) p->frame = 0;
        }
    }

    setSrc(&p->src, p->frame, row);
}

void renderPlayer(Player p, SDL_Renderer *r) {
    if (!p.texture) return;
    SDL_RendererFlip flip = p.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(r, p.texture, &p.src, &p.dest, 0.0, NULL, flip);
}

/* ── Enemy ───────────────────────────────────────────────────────── */

void setEnemyState(Enemy *e, AnimState s) {
    if (e->state == s) return;
    e->state = s; e->frame = 0; e->frameTimer = 0;
}

void initEnemy(Enemy *e, SDL_Renderer *r) {
    e->texture     = loadTexture("zapan.png", r);
    e->dest        = (SDL_Rect){620, GROUND_Y - DISP_H, DISP_W, DISP_H};
    e->vx          = -2;
    e->frame       = 0;
    e->frameTimer  = 0;
    e->frameDelay  = 10;
    e->state       = ANIM_WALK;
    e->alive       = 1;
    e->facingRight = 0;
    setSrc(&e->src, 0, ZAPAN_ROW_WALK);
}

void updateEnemy(Enemy *e, Player *p) {
    if (!e->alive) return;

    int px   = p->dest.x + p->dest.w / 2;
    int ex   = e->dest.x + e->dest.w / 2;
    int dist = abs(px - ex);

    if (e->state != ANIM_ATTACK && e->state != ANIM_DEATH) {
        if (dist > 90) {
            e->vx          = (px < ex) ? -2 : 2;
            e->facingRight = (e->vx > 0);
            setEnemyState(e, ANIM_WALK);
        } else {
            e->vx = 0;
            if (rand() % 90 == 0)
                setEnemyState(e, ANIM_ATTACK);
            else if (e->state != ANIM_ATTACK)
                setEnemyState(e, ANIM_IDLE);
        }
    }

    e->dest.x += e->vx;
    if (e->dest.x < 0)            e->dest.x = 0;
    if (e->dest.x > 800 - DISP_W) e->dest.x = 800 - DISP_W;

    int row;
    switch (e->state) {
        case ANIM_WALK:   row = ZAPAN_ROW_WALK;   break;
        case ANIM_ATTACK: row = ZAPAN_ROW_ATTACK; break;
        case ANIM_DEATH:  row = ZAPAN_ROW_DEATH;  break;
        default:          row = ZAPAN_ROW_IDLE;   break;
    }

    e->frameTimer++;
    if (e->frameTimer >= e->frameDelay) {
        e->frameTimer = 0;
        e->frame++;
        if (e->state == ANIM_ATTACK) {
            if (e->frame >= SHEET_COLS) {
                e->frame = 0;
                setEnemyState(e, ANIM_IDLE);
            }
        } else if (e->state == ANIM_DEATH) {
            if (e->frame >= SHEET_COLS) {
                e->frame = SHEET_COLS - 1;
                e->alive = 0;
            }
        } else {
            if (e->frame >= SHEET_COLS) e->frame = 0;
        }
    }

    setSrc(&e->src, e->frame, row);
}

void renderEnemy(Enemy e, SDL_Renderer *r) {
    if (!e.texture) return;
    SDL_RendererFlip flip = e.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(r, e.texture, &e.src, &e.dest, 0.0, NULL, flip);
}

/* ── Background ──────────────────────────────────────────────────── */

void initBackground(Background *bg, SDL_Renderer *r) {
    bg->texture = loadTexture("background.png", r);
    bg->dest    = (SDL_Rect){0, 0, 800, 600};
}

void renderBackground(Background bg, SDL_Renderer *r) {
    if (!bg.texture) {
        SDL_SetRenderDrawColor(r, 8, 5, 20, 255);
        SDL_Rect sky = {0, 0, 800, 600};
        SDL_RenderFillRect(r, &sky);

        SDL_SetRenderDrawColor(r, 20, 16, 28, 255);
        SDL_Rect ground = {0, GROUND_Y, 800, 600};
        SDL_RenderFillRect(r, &ground);

        SDL_SetRenderDrawColor(r, 60, 40, 100, 255);
        SDL_RenderDrawLine(r, 0, GROUND_Y, 800, GROUND_Y);
        return;
    }
    SDL_RenderCopy(r, bg.texture, NULL, &bg.dest);
}

/* ── Health bar ──────────────────────────────────────────────────── */

void initHealth(HealthBar *hb, int x, SDL_Color color, SDL_Color bgColor) {
    hb->max     = 100;
    hb->current = 100;
    hb->bar     = (SDL_Rect){x, 20, 180, 18};
    hb->color   = color;
    hb->bgColor = bgColor;
}

void renderHealth(HealthBar hb, SDL_Renderer *r) {
    SDL_SetRenderDrawColor(r, hb.bgColor.r, hb.bgColor.g, hb.bgColor.b, 200);
    SDL_RenderFillRect(r, &hb.bar);

    SDL_SetRenderDrawColor(r, hb.color.r, hb.color.g, hb.color.b, 255);
    SDL_Rect cur = hb.bar;
    cur.w = (hb.current * hb.bar.w) / hb.max;
    SDL_RenderFillRect(r, &cur);

    SDL_SetRenderDrawColor(r, 200, 200, 255, 255);
    SDL_RenderDrawRect(r, &hb.bar);
}

/* ── Collision ───────────────────────────────────────────────────── */

int checkCollision(SDL_Rect a, SDL_Rect b) {
    a.x += 10; a.w -= 20;
    b.x += 10; b.w -= 20;
    if (a.x + a.w < b.x) return 0;
    if (a.x       > b.x + b.w) return 0;
    if (a.y + a.h < b.y) return 0;
    if (a.y       > b.y + b.h) return 0;
    return 1;
}
