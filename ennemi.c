#include "ennemi.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* ═══════════════════════════════════════════════════════════════════
   UTILITY
   ═══════════════════════════════════════════════════════════════════ */

SDL_Texture *loadTexture(const char *file, SDL_Renderer *r) {
    SDL_Surface *s = IMG_Load(file);
    if (!s) { printf("ERROR: Cannot load %s -> %s\n", file, IMG_GetError()); return NULL; }
    printf("Loaded %s (%dx%d)\n", file, s->w, s->h);
    SDL_Texture *t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    return t;
}

void setSrc(SDL_Rect *src, int frame, int row) {
    src->x = frame * FRAME_W;
    src->y = row   * FRAME_H;
    src->w = FRAME_W;
    src->h = FRAME_H;
}

int checkCollision(SDL_Rect a, SDL_Rect b) {
    a.x += 12; a.w -= 24;
    b.x += 12; b.w -= 24;
    a.y +=  8; a.h -= 16;
    b.y +=  8; b.h -= 16;
    if (a.x + a.w < b.x) return 0;
    if (a.x       > b.x + b.w) return 0;
    if (a.y + a.h < b.y) return 0;
    if (a.y       > b.y + b.h) return 0;
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════
   PLAYER
   ═══════════════════════════════════════════════════════════════════ */

void setPlayerState(Player *p, AnimState s) {
    if (p->state == s) return;
    /* leaving attack → ready for next hit */
    if (p->state == ANIM_ATTACK && s != ANIM_ATTACK)
        p->hitLanded = 0;
    p->state = s; p->frame = 0; p->frameTimer = 0;
}

void initPlayer(Player *p, SDL_Renderer *r) {
    p->texture     = loadTexture("alita.png", r);
    p->dest        = (SDL_Rect){80, GROUND_Y - DISP_H, DISP_W, DISP_H};
    p->vx = p->vy  = 0;
    p->onGround    = 1;
    p->frame       = 0;
    p->frameTimer  = 0;
    p->frameDelay  = 8;
    p->state       = ANIM_IDLE;
    p->alive       = 1;
    p->facingRight = 1;
    p->hp          = 5;
    p->maxHP       = 5;
    p->invincible  = 0;
    p->hitLanded   = 0;
    setSrc(&p->src, 0, ALITA_ROW_IDLE);
}

void handleInput(Player *p, SDL_Event e) {
    if (!p->alive) return;
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_RIGHT: case SDLK_d:
                p->vx = 5; p->facingRight = 1;
                if (p->state != ANIM_ATTACK && p->state != ANIM_JUMP)
                    setPlayerState(p, ANIM_WALK);
                break;
            case SDLK_LEFT: case SDLK_a:
                p->vx = -5; p->facingRight = 0;
                if (p->state != ANIM_ATTACK && p->state != ANIM_JUMP)
                    setPlayerState(p, ANIM_WALK);
                break;
            case SDLK_SPACE: case SDLK_UP: case SDLK_w:
                if (p->onGround) { p->vy = -17; p->onGround = 0; setPlayerState(p, ANIM_JUMP); }
                break;
            case SDLK_z: case SDLK_LCTRL: case SDLK_x:
                setPlayerState(p, ANIM_ATTACK);
                break;
        }
    }
    if (e.type == SDL_KEYUP) {
        if (e.key.keysym.sym == SDLK_LEFT  || e.key.keysym.sym == SDLK_a ||
            e.key.keysym.sym == SDLK_RIGHT || e.key.keysym.sym == SDLK_d) {
            p->vx = 0;
            if (p->onGround && p->state == ANIM_WALK) setPlayerState(p, ANIM_IDLE);
        }
    }
}

void updatePlayer(Player *p) {
    if (!p->alive) return;
    p->vy += 1;
    p->dest.x += p->vx;
    p->dest.y += p->vy;
    int ground = GROUND_Y - DISP_H;
    if (p->dest.y >= ground) {
        p->dest.y = ground; p->vy = 0;
        if (!p->onGround) {
            p->onGround = 1;
            if (p->state == ANIM_JUMP) setPlayerState(p, p->vx != 0 ? ANIM_WALK : ANIM_IDLE);
        }
    }
    if (p->dest.x < 0)               p->dest.x = 0;
    if (p->dest.x > SCREEN_W - DISP_W) p->dest.x = SCREEN_W - DISP_W;
    if (p->invincible > 0) p->invincible--;
    if (p->hp <= 0) { p->alive = 0; p->hp = 0; }
    int row;
    switch (p->state) {
        case ANIM_WALK:   row = ALITA_ROW_WALK;   break;
        case ANIM_ATTACK: row = ALITA_ROW_ATTACK; break;
        case ANIM_JUMP:   row = ALITA_ROW_JUMP;   break;
        default:          row = ALITA_ROW_IDLE;   break;
    }
    p->frameTimer++;
    if (p->frameTimer >= p->frameDelay) {
        p->frameTimer = 0; p->frame++;
        if (p->state == ANIM_ATTACK && p->frame >= SHEET_COLS) {
            p->frame = 0;
            p->hitLanded = 0;   /* ready to score on next attack */
            setPlayerState(p, p->vx != 0 ? ANIM_WALK : ANIM_IDLE);
        } else if (p->frame >= SHEET_COLS) p->frame = 0;
    }
    setSrc(&p->src, p->frame, row);
}

void renderPlayer(Player p, SDL_Renderer *r) {
    if (!p.texture) return;
    if (p.invincible > 0 && (p.invincible / 4) % 2 == 0) return;
    SDL_RendererFlip flip = p.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(r, p.texture, &p.src, &p.dest, 0.0, NULL, flip);
}

/* ═══════════════════════════════════════════════════════════════════
   ENEMY AI
   ───────────────────────────────────────────────────────────────────
   Three behaviour modes, but Zapan NEVER stops moving:

   BEHAV_PATROL  – random walk, totally ignores player.
                   Changes direction every 60-150 frames.
                   Bounces off screen edges.
                   Level-2: also moves vertically.

   BEHAV_CHASE   – player entered awareness radius (RADIUS_CHASE).
                   Zapan moves toward the player at chase speed.
                   Still moves even if perfectly aligned (slides past,
                   bounces back) so he never freezes.
                   Gives up and returns to PATROL after chaseTimer
                   exceeds CHASE_GIVE_UP frames without catching up.

   BEHAV_ATTACK  – player within RADIUS_ATTACK.
                   Zapan locks into an attack animation AND keeps a
                   small residual drift so he's always in motion.
                   Attack has a cooldown; between strikes he briefly
                   re-enters CHASE to close any gap.

   In ALL modes the velocity is never (0,0) for more than 1 frame:
   if both components land at zero a minimum jitter is injected.
   ═══════════════════════════════════════════════════════════════════ */

#define PATROL_SPEED     2
#define CHASE_SPEED      3
#define CHASE_SPEED_L2   4
#define CHASE_GIVE_UP  180   /* frames before abandoning a chase */
#define ATTACK_COOLDOWN 55   /* frames between strikes            */
#define DRIFT_SPEED      1   /* residual drift speed during attack */

/* ── helpers ───────────────────────────────────────────────────── */

/* Return horizontal distance between centres */
int centreDist(SDL_Rect a, SDL_Rect b) {
    int ax = a.x + a.w / 2;
    int bx = b.x + b.w / 2;
    int dx = ax - bx;
    return dx < 0 ? -dx : dx;
}

/* Pick a fresh random patrol direction – never zero velocity */
void newPatrolDir(Enemy *e) {
    int spd = PATROL_SPEED;
    /* horizontal: -spd or +spd (50/50, no zero) */
    e->patrolVx = (rand() % 2 == 0) ? spd : -spd;

    if (e->aerial) {
        /* vertical: -spd, 0, or +spd with weights 40/20/40 */
        int r = rand() % 5;
        e->patrolVy = (r < 2) ? -spd : (r < 3) ? 0 : spd;
    } else {
        e->patrolVy = 0;
    }

    /* duration: 60-150 frames */
    e->patrolTimer = 60 + rand() % 91;
    /* face direction we're moving */
    if (e->patrolVx > 0) e->facingRight = 1;
    else                 e->facingRight = 0;
}

/* Ensure velocity is never (0,0) – inject minimum jitter */
void enforceMotion(Enemy *e) {
    if (e->vx == 0 && e->vy == 0) {
        e->vx = (rand() % 2 == 0) ? 1 : -1;
    }
}

/* Clamp enemy to screen boundaries, bouncing velocity on hit */
void clampBounce(Enemy *e) {
    if (e->dest.x < 0) {
        e->dest.x = 0;
        e->vx     = -e->vx;
        if (e->vx <= 0) e->vx = PATROL_SPEED;   /* never stick to wall */
        e->facingRight = 1;
        /* also refresh patrol so he doesn't immediately re-pick same dir */
        if (e->behav == BEHAV_PATROL) newPatrolDir(e);
    }
    if (e->dest.x > SCREEN_W - DISP_W) {
        e->dest.x = SCREEN_W - DISP_W;
        e->vx     = -e->vx;
        if (e->vx >= 0) e->vx = -PATROL_SPEED;
        e->facingRight = 0;
        if (e->behav == BEHAV_PATROL) newPatrolDir(e);
    }
    if (e->aerial) {
        if (e->dest.y < CEIL_Y) {
            e->dest.y = CEIL_Y;
            e->vy = (e->vy < 0) ? -e->vy : e->vy;
            if (e->vy == 0) e->vy = PATROL_SPEED;
        }
        if (e->dest.y > GROUND_Y - DISP_H) {
            e->dest.y = GROUND_Y - DISP_H;
            e->vy = (e->vy > 0) ? -e->vy : e->vy;
            if (e->vy == 0) e->vy = -PATROL_SPEED;
        }
    } else {
        /* ground-only: stay on ground */
        e->dest.y = GROUND_Y - DISP_H;
        e->vy     = 0;
    }
}

/* ── public functions ──────────────────────────────────────────── */

void setEnemyState(Enemy *e, AnimState s) {
    if (e->state == s) return;
    e->state = s; e->frame = 0; e->frameTimer = 0;
}

void initEnemy(Enemy *e, SDL_Renderer *r, GameLevel lvl) {
    e->texture      = loadTexture("zapan.png", r);
    e->dest         = (SDL_Rect){620, GROUND_Y - DISP_H, DISP_W, DISP_H};
    e->frame        = 0;
    e->frameTimer   = 0;
    e->facingRight  = 0;
    e->alive        = 1;
    e->attackCooldown = 0;
    e->chaseTimer   = 0;
    e->behav        = BEHAV_PATROL;
    e->state        = ANIM_WALK;
    e->vx = e->vy   = 0;

    if (lvl == LEVEL_1) {
        e->aerial      = 0;
        e->hp = e->maxHP = 5;
        e->frameDelay  = 10;
    } else {
        e->aerial      = 1;
        e->hp = e->maxHP = 5;
        e->frameDelay  = 7;
        e->dest.y      = CEIL_Y + 60;
    }

    e->patrolTimer = 0;
    e->patrolVx    = 0;
    e->patrolVy    = 0;
    newPatrolDir(e);   /* start with a valid direction immediately */
    setSrc(&e->src, 0, ZAPAN_ROW_WALK);
}

void updateEnemy(Enemy *e, Player *p) {
    if (!e->alive) return;

    /* ── Death animation: play out then flag dead ──────────────── */
    if (e->state == ANIM_DEATH) {
        e->frameTimer++;
        if (e->frameTimer >= e->frameDelay) {
            e->frameTimer = 0; e->frame++;
            if (e->frame >= SHEET_COLS) { e->frame = SHEET_COLS - 1; e->alive = 0; }
        }
        setSrc(&e->src, e->frame, ZAPAN_ROW_DEATH);
        return;
    }

    /* ══════════════════════════════════════════════════════════════
       STEP 1 – compute distance to player
       ══════════════════════════════════════════════════════════════ */
    int dist = centreDist(e->dest, p->dest);
    int chaseSpd = (e->aerial) ? CHASE_SPEED_L2 : CHASE_SPEED;

    /* ══════════════════════════════════════════════════════════════
       STEP 2 – decide / update behaviour mode
       ══════════════════════════════════════════════════════════════ */

    if (e->attackCooldown > 0) e->attackCooldown--;

    if (dist <= RADIUS_ATTACK && e->attackCooldown == 0) {
        /* ── ATTACK ZONE ── */
        e->behav      = BEHAV_ATTACK;
        e->chaseTimer = 0;
        setEnemyState(e, ANIM_ATTACK);
        e->attackCooldown = ATTACK_COOLDOWN;

    } else if (dist <= RADIUS_CHASE) {
        /* ── CHASE ZONE ── */
        if (e->behav != BEHAV_ATTACK) {   /* don't interrupt current swing */
            e->behav = BEHAV_CHASE;
            e->chaseTimer++;
            if (e->chaseTimer > CHASE_GIVE_UP) {
                /* lost the player, return to patrol */
                e->behav      = BEHAV_PATROL;
                e->chaseTimer = 0;
                newPatrolDir(e);
            }
        }
    } else {
        /* ── PATROL ZONE ── */
        if (e->behav != BEHAV_PATROL) {
            e->behav      = BEHAV_PATROL;
            e->chaseTimer = 0;
            newPatrolDir(e);
        }
    }

    /* ══════════════════════════════════════════════════════════════
       STEP 3 – compute velocity based on current behaviour
       ══════════════════════════════════════════════════════════════ */

    if (e->behav == BEHAV_PATROL) {
        /* countdown to next direction change */
        e->patrolTimer--;
        if (e->patrolTimer <= 0) newPatrolDir(e);

        e->vx = e->patrolVx;
        e->vy = e->patrolVy;

    } else if (e->behav == BEHAV_CHASE) {
        /* move toward player horizontally */
        int px = p->dest.x + p->dest.w / 2;
        int ex = e->dest.x + e->dest.w / 2;
        e->vx         = (px > ex) ? chaseSpd : -chaseSpd;
        e->facingRight = (e->vx > 0);

        if (e->aerial) {
            /* also close vertical gap */
            int py = p->dest.y + p->dest.h / 2;
            int ey = e->dest.y + e->dest.h / 2;
            if      (py > ey + 10) e->vy =  chaseSpd;
            else if (py < ey - 10) e->vy = -chaseSpd;
            else                   e->vy =  0;
        } else {
            e->vy = 0;
        }

    } else {
        /* BEHAV_ATTACK:
           keep a small drift so he never freezes.
           Direction = toward player so he stays threatening. */
        int px = p->dest.x + p->dest.w / 2;
        int ex = e->dest.x + e->dest.w / 2;
        e->vx         = (px > ex) ? DRIFT_SPEED : -DRIFT_SPEED;
        e->facingRight = (e->vx > 0);
        e->vy          = 0;
    }

    /* ══════════════════════════════════════════════════════════════
       STEP 4 – safety: velocity must never be (0,0)
       ══════════════════════════════════════════════════════════════ */
    enforceMotion(e);

    /* ══════════════════════════════════════════════════════════════
       STEP 5 – apply velocity + boundary bouncing
       ══════════════════════════════════════════════════════════════ */
    e->dest.x += e->vx;
    e->dest.y += e->vy;
    clampBounce(e);

    /* ══════════════════════════════════════════════════════════════
       STEP 6 – choose animation row
       After an attack finishes, re-enter CHASE if still close,
       else PATROL – so there's no idle frame between strikes.
       ══════════════════════════════════════════════════════════════ */
    int row;
    if (e->state == ANIM_ATTACK) {
        row = ZAPAN_ROW_ATTACK;
    } else if (e->behav == BEHAV_PATROL || e->behav == BEHAV_CHASE) {
        setEnemyState(e, ANIM_WALK);
        row = ZAPAN_ROW_WALK;
    } else {
        /* behav_attack but animation finished → WALK drift */
        setEnemyState(e, ANIM_WALK);
        row = ZAPAN_ROW_WALK;
    }

    /* ══════════════════════════════════════════════════════════════
       STEP 7 – advance frame
       ══════════════════════════════════════════════════════════════ */
    e->frameTimer++;
    if (e->frameTimer >= e->frameDelay) {
        e->frameTimer = 0; e->frame++;
        if (e->state == ANIM_ATTACK && e->frame >= SHEET_COLS) {
            e->frame = 0;
            /* attack finished – decide next behaviour right now */
            if (dist <= RADIUS_CHASE)
                e->behav = BEHAV_CHASE;
            else {
                e->behav = BEHAV_PATROL;
                newPatrolDir(e);
            }
            setEnemyState(e, ANIM_WALK);
            row = ZAPAN_ROW_WALK;
        } else if (e->frame >= SHEET_COLS) {
            e->frame = 0;
        }
    }

    setSrc(&e->src, e->frame, row);
}

void renderEnemy(Enemy e, SDL_Renderer *r) {
    if (!e.texture) return;

    /* Level-2 aerial: purple glow halo */
    if (e.aerial && e.alive) {
        SDL_SetTextureColorMod(e.texture, 180, 80, 255);
        SDL_SetTextureAlphaMod(e.texture, 80);
        SDL_Rect glow = {e.dest.x - 4, e.dest.y - 4, e.dest.w + 8, e.dest.h + 8};
        SDL_RendererFlip f2 = e.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        SDL_RenderCopyEx(r, e.texture, &e.src, &glow, 0.0, NULL, f2);
        SDL_SetTextureAlphaMod(e.texture, 255);
        SDL_SetTextureColorMod(e.texture, 255, 255, 255);
    }

    SDL_RendererFlip flip = e.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(r, e.texture, &e.src, &e.dest, 0.0, NULL, flip);
}

/* ═══════════════════════════════════════════════════════════════════
   BACKGROUND
   ═══════════════════════════════════════════════════════════════════ */

void initBackground(Background *bg, SDL_Renderer *r, GameLevel lvl) {
    /* Load both textures once on first call (tex1/tex2 may already be set
       if we're transitioning from level 1 to level 2 — skip reload). */
    if (!bg->tex1) bg->tex1 = loadTexture("background1.png", r);
    if (!bg->tex2) bg->tex2 = loadTexture("background2.png", r);
    bg->dest  = (SDL_Rect){0, 0, SCREEN_W, SCREEN_H};
    bg->level = lvl;
}

void renderBackground(Background bg, SDL_Renderer *r) {
    SDL_Texture *tex = (bg.level == LEVEL_1) ? bg.tex1 : bg.tex2;

    if (tex) {
        /* Draw the correct background image — no tinting, clean as-is */
        SDL_RenderCopy(r, tex, NULL, &bg.dest);
    } else {
        /* Fallback solid colour if image failed to load */
        if (bg.level == LEVEL_1) {
            SDL_SetRenderDrawColor(r, 20, 10, 35, 255);
        } else {
            SDL_SetRenderDrawColor(r, 5, 0, 20, 255);
        }
        SDL_RenderFillRect(r, &bg.dest);
    }
}

/* ═══════════════════════════════════════════════════════════════════
   HEALTH BAR
   ═══════════════════════════════════════════════════════════════════ */

void initHealth(HealthBar *hb, int x, int maxHP, SDL_Color col, SDL_Color bg) {
    hb->max = hb->current = maxHP;
    hb->bar     = (SDL_Rect){x, 16, 180, 20};
    hb->color   = col;
    hb->bgColor = bg;
}

void updateHealth(HealthBar *hb, int cur) {
    hb->current = cur < 0 ? 0 : (cur > hb->max ? hb->max : cur);
}

void renderHealth(HealthBar hb, SDL_Renderer *r, const char *lbl, SDL_Renderer *ren) {
    (void)lbl; (void)ren;

    int total  = hb.max > 0 ? hb.max : 5;
    int filled = hb.current < 0 ? 0 : hb.current;
    int pip_gap = 4;
    int pip_w   = (hb.bar.w - pip_gap * (total - 1)) / total;
    int pip_h   = hb.bar.h;

    for (int i = 0; i < total; i++) {
        int px = hb.bar.x + i * (pip_w + pip_gap);
        int py = hb.bar.y;
        SDL_Rect pip = {px, py, pip_w, pip_h};

        /* background (empty pip) */
        SDL_SetRenderDrawColor(r, hb.bgColor.r, hb.bgColor.g, hb.bgColor.b, 200);
        SDL_RenderFillRect(r, &pip);

        /* filled pip */
        if (i < filled) {
            SDL_SetRenderDrawColor(r, hb.color.r, hb.color.g, hb.color.b, 255);
            SDL_RenderFillRect(r, &pip);
        }

        /* border */
        SDL_SetRenderDrawColor(r, 210, 210, 255, 255);
        SDL_RenderDrawRect(r, &pip);
    }
}
