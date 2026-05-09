/* enigme.c */
#include "enigme.h"

/* ══════════════════════════════════════════════════════════════
   HELPERS
══════════════════════════════════════════════════════════════ */

SDL_Rect cell_rect(int col, int row) {
    SDL_Rect r = {
        BOARD_X + col * CELL_W,
        BOARD_Y + row * CELL_H,
        CELL_W, CELL_H
    };
    return r;
}

/* Crop a region of src into a new CELL_W x CELL_H texture */
SDL_Texture *crop_texture(SDL_Renderer *r, SDL_Texture *src,
                           SDL_Rect src_rect, int dst_w, int dst_h) {
    SDL_Texture *tmp = SDL_CreateTexture(r,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        dst_w, dst_h);
    if (!tmp) return NULL;
    SDL_SetRenderTarget(r, tmp);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
    SDL_RenderClear(r);
    SDL_Rect dst_rect = {0, 0, dst_w, dst_h};
    SDL_RenderCopy(r, src, &src_rect, &dst_rect);
    SDL_SetRenderTarget(r, NULL);
    return tmp;
}

/* Rotozoom a texture into a new texture */
SDL_Texture *rotozoom_tex(SDL_Renderer *r, SDL_Texture *src,
                           double angle, double scale) {
    int w, h;
    SDL_QueryTexture(src, NULL, NULL, &w, &h);

    SDL_Texture *tmp = SDL_CreateTexture(r,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!tmp) return NULL;

    SDL_SetRenderTarget(r, tmp);
    SDL_RenderCopy(r, src, NULL, NULL);
    SDL_SetRenderTarget(r, NULL);

    SDL_Surface *surf = SDL_CreateRGBSurface(0, w, h, 32,
        0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    if (!surf) { SDL_DestroyTexture(tmp); return NULL; }

    SDL_SetRenderTarget(r, tmp);
    SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_RGBA8888,
                         surf->pixels, surf->pitch);
    SDL_SetRenderTarget(r, NULL);
    SDL_DestroyTexture(tmp);

    SDL_Surface *rz = rotozoomSurface(surf, angle, scale, SMOOTHING_ON);
    SDL_FreeSurface(surf);
    if (!rz) return NULL;

    SDL_Texture *result = SDL_CreateTextureFromSurface(r, rz);
    SDL_FreeSurface(rz);
    return result;
}

/* Neon glow border */
void draw_glow_rect(SDL_Renderer *r, SDL_Rect rect,
                     Uint8 R, Uint8 G, Uint8 B, int layers) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = layers; i >= 0; i--) {
        Uint8 a = (Uint8)(255 * (1.0f - (float)i / (layers + 1)));
        SDL_SetRenderDrawColor(r, R, G, B, a);
        SDL_Rect br = {rect.x - i, rect.y - i,
                       rect.w + 2*i, rect.h + 2*i};
        SDL_RenderDrawRect(r, &br);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ══════════════════════════════════════════════════════════════
   INIT / FREE
══════════════════════════════════════════════════════════════ */

void game_init(Game *g) {
    memset(g, 0, sizeof(Game));
    g->state      = STATE_LOADING;
    g->load_angle = 0.f;
}

void game_free_round(Game *g) {
    for (int i = 0; i < GRID_TOTAL; i++) {
        if (g->cells[i]) { SDL_DestroyTexture(g->cells[i]); g->cells[i] = NULL; }
    }
    for (int i = 0; i < NUM_CHOICES; i++) {
        if (g->choices[i].tex) {
            SDL_DestroyTexture(g->choices[i].tex);
            g->choices[i].tex = NULL;
        }
    }
    if (g->placed_tex) { SDL_DestroyTexture(g->placed_tex); g->placed_tex = NULL; }
    g->piece_placed  = 0;
    g->placed_correct = 0;
}

void game_free(Game *g) {
    game_free_round(g);
    for (int i = 0; i < 5; i++) {
        if (g->full_images[i]) {
            SDL_DestroyTexture(g->full_images[i]);
            g->full_images[i] = NULL;
        }
    }
}

/* ══════════════════════════════════════════════════════════════
   LOAD IMAGES
══════════════════════════════════════════════════════════════ */

void game_load_images(Game *g) {
    const char *paths[5] = {
        "images/img1.jpg", "images/img2.jpg", "images/img3.jpg",
        "images/img4.jpg", "images/img5.jpg"
    };
    for (int i = 0; i < 5; i++) {
        SDL_Surface *s = IMG_Load(paths[i]);
        if (!s) { fprintf(stderr, "Load fail: %s\n", paths[i]); continue; }
        g->full_images[i] = SDL_CreateTextureFromSurface(g->renderer, s);
        SDL_FreeSurface(s);
    }
}

/* ══════════════════════════════════════════════════════════════
   START ROUND
   - Pick an image (cycles through all 5)
   - Split into 4x4 = 16 pieces
   - Pick one cell to be the HOLE (random, not corner)
   - Generate 3 choice pieces: 1 correct + 2 wrong (from other cells)
══════════════════════════════════════════════════════════════ */

void game_start_round(Game *g) {
    game_free_round(g);

    /* Pick image: use round index mod 5, but shuffle each game start */
    g->img_idx = g->round % 5;

    SDL_Texture *src = g->full_images[g->img_idx];
    if (!src) { g->img_idx = 0; src = g->full_images[0]; }

    int iw, ih;
    SDL_QueryTexture(src, NULL, NULL, &iw, &ih);

    /* Pick the missing cell – avoid corners for visual clarity */
    int mc, mr;
    do {
        mc = 1 + rand() % (GRID_COLS - 2);  /* col 1..2 */
        mr = 1 + rand() % (GRID_ROWS - 2);  /* row 1..2 */
    } while (mc == 0 && mr == 0);

    g->missing_col = mc;
    g->missing_row = mr;

    /* Build all 16 cell textures */
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int idx = row * GRID_COLS + col;
            SDL_Rect src_rect = {
                col  * iw / GRID_COLS,
                row  * ih / GRID_ROWS,
                iw / GRID_COLS,
                ih / GRID_ROWS
            };
            if (col == mc && row == mr) {
                /* This is the hole – don't put it on the board */
                g->cells[idx] = NULL;
            } else {
                g->cells[idx] = crop_texture(g->renderer, src,
                                              src_rect, CELL_W, CELL_H);
            }
        }
    }

    /* ── Build the 3 choice pieces ── */
    /* Correct piece: the actual missing cell, scaled to CHOICE_W x CHOICE_H */
    SDL_Rect correct_src = {
        mc * iw / GRID_COLS,
        mr * ih / GRID_ROWS,
        iw / GRID_COLS,
        ih / GRID_ROWS
    };
    SDL_Texture *correct_tex = crop_texture(g->renderer, src,
                                             correct_src, CHOICE_W, CHOICE_H);

    /* Wrong pieces: 2 random OTHER cells from the SAME image */
    int wrong_indices[GRID_TOTAL];
    int wrong_count = 0;
    for (int i = 0; i < GRID_TOTAL; i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        if (col != mc || row != mr)
            wrong_indices[wrong_count++] = i;
    }
    /* Shuffle wrong candidates */
    for (int i = wrong_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = wrong_indices[i];
        wrong_indices[i] = wrong_indices[j];
        wrong_indices[j] = tmp;
    }

    SDL_Texture *wrong_tex[2];
    for (int w = 0; w < 2; w++) {
        int wi = wrong_indices[w];
        int wc = wi % GRID_COLS;
        int wr = wi / GRID_COLS;
        SDL_Rect wr_rect = {
            wc * iw / GRID_COLS,
            wr * ih / GRID_ROWS,
            iw / GRID_COLS,
            ih / GRID_ROWS
        };
        wrong_tex[w] = crop_texture(g->renderer, src, wr_rect,
                                     CHOICE_W, CHOICE_H);
    }

    /* Place into choices[], shuffled */
    g->correct_choice = rand() % 3;
    int wi = 0;
    for (int i = 0; i < NUM_CHOICES; i++) {
        if (i == g->correct_choice) {
            g->choices[i].tex        = correct_tex;
            g->choices[i].is_correct = 1;
        } else {
            g->choices[i].tex        = wrong_tex[wi++];
            g->choices[i].is_correct = 0;
        }
        /* Resting screen position */
        g->choices[i].sx       = PANEL_X;
        g->choices[i].sy       = PANEL_Y + i * (CHOICE_H + CHOICE_SPACING);
        g->choices[i].dragging = 0;
    }

    g->round_start = SDL_GetTicks();
    g->elapsed     = 0.f;
    g->state       = STATE_PLAYING;
    g->anim_timer  = 0.f;
}

/* ══════════════════════════════════════════════════════════════
   UPDATE
══════════════════════════════════════════════════════════════ */

void game_update(Game *g, float dt) {
    switch (g->state) {

    case STATE_LOADING:
        g->load_angle += 200.f * dt;
        if (g->load_angle > 360.f) g->load_angle -= 360.f;
        break;

    case STATE_PLAYING:
        g->elapsed = (SDL_GetTicks() - g->round_start) / 1000.f;

        /* Time up */
        if (!g->piece_placed && g->elapsed >= TIME_LIMIT) {
            g->state     = STATE_FAILURE;
            g->anim_timer = 0.f;
            break;
        }

        /* After placement, wait a moment then advance */
        if (g->piece_placed) {
            g->anim_timer += dt;
            g->rotozoom_angle += 90.f * dt;
            g->rotozoom_scale  = 1.f + 0.25f * (float)fabs(sin(g->anim_timer * 3.0));

            if (g->anim_timer >= 2.5f) {
                if (g->placed_correct) {
                    g->round++;
                    if (g->round >= TOTAL_ROUNDS) {
                        g->state      = STATE_SUCCESS;
                        g->anim_timer = 0.f;
                    } else {
                        game_start_round(g);
                    }
                } else {
                    g->state      = STATE_FAILURE;
                    g->anim_timer = 0.f;
                }
            }
        }
        break;

    case STATE_SUCCESS:
    case STATE_FAILURE:
        g->anim_timer     += dt;
        g->rotozoom_angle += 100.f * dt;
        g->rotozoom_scale  = 0.8f + 0.4f * (float)fabs(sin(g->anim_timer * 2.0));
        break;

    default:
        break;
    }
}

/* ══════════════════════════════════════════════════════════════
   DRAW HELPERS
══════════════════════════════════════════════════════════════ */

static void draw_bg(SDL_Renderer *r) {
    /* Dark gradient */
    for (int y = 0; y < WINDOW_H; y++) {
        float t = (float)y / WINDOW_H;
        SDL_SetRenderDrawColor(r, (Uint8)(8 + 6*t), (Uint8)(2*t), (Uint8)(22 + 18*t), 255);
        SDL_RenderDrawLine(r, 0, y, WINDOW_W, y);
    }
    /* Scanlines */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 28);
    for (int y = 0; y < WINDOW_H; y += 3)
        SDL_RenderDrawLine(r, 0, y, WINDOW_W, y);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

static void draw_timer_bar(Game *g) {
    SDL_Renderer *r = g->renderer;
    float ratio = 1.f - (g->elapsed / TIME_LIMIT);
    if (ratio < 0.f) ratio = 0.f;

    int bx = BOARD_X, by = BOARD_Y + BOARD_H + 18;
    int bw = BOARD_W, bh = 16;

    /* Track */
    SDL_Rect track = {bx, by, bw, bh};
    SDL_SetRenderDrawColor(r, 15, 5, 35, 220);
    SDL_RenderFillRect(r, &track);

    /* Fill */
    int fw = (int)(bw * ratio);
    if (fw > 0) {
        Uint8 rr = (Uint8)(255 * (1.f - ratio));
        Uint8 gg = (Uint8)(60  * ratio);
        Uint8 bb = (Uint8)(255 * ratio);
        SDL_Rect fill = {bx, by, fw, bh};
        SDL_SetRenderDrawColor(r, rr, gg, bb, 230);
        SDL_RenderFillRect(r, &fill);
        /* Pulse leading edge */
        float pulse = 0.5f + 0.5f * sinf(SDL_GetTicks() * 0.01f);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 255, 255, 255, (Uint8)(160 * pulse));
        SDL_Rect edge = {bx + fw - 4, by, 4, bh};
        SDL_RenderFillRect(r, &edge);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    /* Tick marks */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 80);
    for (int seg = 1; seg < 10; seg++)
        SDL_RenderDrawLine(r, bx + seg * bw/10, by, bx + seg * bw/10, by + bh);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* Outer glow border */
    draw_glow_rect(r, track, 80, 0, 120, 3);

    /* Hourglass symbol – animated */
    int hx = bx - 35, hy = by;
    SDL_SetRenderDrawColor(r, 200, 200, 0, 255);
    SDL_Rect htop = {hx, hy,     18, 6};
    SDL_Rect hbot = {hx, hy+10, 18, 6};
    SDL_RenderFillRect(r, &htop);
    SDL_RenderFillRect(r, &hbot);
    /* Falling sand dot */
    float sand = hy + 6 + ratio * 4.f;
    SDL_SetRenderDrawColor(r, 255, 220, 0, 255);
    SDL_Rect grain = {hx + 7, (int)sand, 4, 4};
    SDL_RenderFillRect(r, &grain);
}

static void draw_score_dots(Game *g) {
    SDL_Renderer *r = g->renderer;
    int cx = PANEL_X + 85;
    int cy = WINDOW_H - 50;

    /* Label blocks */
    for (int i = 0; i < TOTAL_ROUNDS; i++) {
        int dx = cx - (TOTAL_ROUNDS * 30)/2 + i * 30;
        SDL_Rect dot = {dx, cy - 12, 22, 22};
        if (i < g->round) {
            SDL_SetRenderDrawColor(r, 0, 255, 150, 255);
        } else if (i == g->round && g->state == STATE_PLAYING) {
            float pulse = 0.5f + 0.5f * sinf(SDL_GetTicks() * 0.005f);
            SDL_SetRenderDrawColor(r, 0, (Uint8)(200 * pulse), (Uint8)(255 * pulse), 255);
        } else {
            SDL_SetRenderDrawColor(r, 40, 0, 60, 255);
        }
        SDL_RenderFillRect(r, &dot);
        draw_glow_rect(r, dot, 0, 180, 220, 2);
    }
}

/* ══════════════════════════════════════════════════════════════
   DRAW – LOADING
══════════════════════════════════════════════════════════════ */

static void draw_loading(Game *g) {
    SDL_Renderer *r = g->renderer;
    draw_bg(r);

    int cx = WINDOW_W / 2, cy = WINDOW_H / 2;
    float ang = g->load_angle * (float)M_PI / 180.f;

    /* Outer ring */
    int ring_r = 80;
    for (int i = 0; i < 48; i++) {
        float a = ang + i * (float)M_PI * 2.f / 48.f;
        float brightness = 0.3f + 0.7f * ((i % 3) == 0 ? 1.f : 0.f);
        SDL_SetRenderDrawColor(r, 0, (Uint8)(220 * brightness), (Uint8)(255 * brightness), 255);
        SDL_RenderDrawLine(r,
            cx + (int)((ring_r - 6) * cosf(a)), cy + (int)((ring_r - 6) * sinf(a)),
            cx + (int)( ring_r      * cosf(a)), cy + (int)( ring_r      * sinf(a)));
    }
    /* Inner spinning cross */
    for (int i = 0; i < 4; i++) {
        float a = ang * 2.f + i * (float)M_PI / 2.f;
        SDL_SetRenderDrawColor(r, 255, 0, 180, 255);
        SDL_RenderDrawLine(r, cx, cy,
            cx + (int)(50 * cosf(a)), cy + (int)(50 * sinf(a)));
    }

    /* "CHARGEMENT" dots */
    int num_dots = 8;
    for (int i = 0; i < num_dots; i++) {
        float a2 = -ang * 1.5f + i * (float)M_PI * 2.f / num_dots;
        float dist = 110.f;
        int px = cx + (int)(dist * cosf(a2));
        int py = cy + (int)(dist * sinf(a2));
        float bright2 = 0.4f + 0.6f * (float)((i + (int)(g->load_angle / 45)) % num_dots == 0);
        SDL_SetRenderDrawColor(r, (Uint8)(255 * bright2), (Uint8)(100 * bright2), 0, 255);
        SDL_Rect dot = {px - 5, py - 5, 10, 10};
        SDL_RenderFillRect(r, &dot);
    }
}

/* ══════════════════════════════════════════════════════════════
   DRAW – PLAYING
══════════════════════════════════════════════════════════════ */

static void draw_playing(Game *g) {
    SDL_Renderer *r = g->renderer;
    draw_bg(r);

    /* ── Board ── */
    /* Outer glow */
    SDL_Rect board = {BOARD_X, BOARD_Y, BOARD_W, BOARD_H};
    draw_glow_rect(r, board, 0, 200, 255, 6);

    /* Draw cells */
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLS; col++) {
            int idx = row * GRID_COLS + col;
            SDL_Rect cr = cell_rect(col, row);

            if (col == g->missing_col && row == g->missing_row) {
                /* THE HOLE */
                if (g->piece_placed) {
                    /* Show the placed piece */
                    if (g->placed_tex) {
                        SDL_RenderCopy(r, g->placed_tex, NULL, &cr);
                    }
                    /* Color tint feedback */
                    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                    if (g->placed_correct) {
                        float pulse = 0.5f + 0.5f * sinf(g->anim_timer * 6.f);
                        SDL_SetRenderDrawColor(r, 0, 255, 120, (Uint8)(80 * pulse));
                    } else {
                        float pulse = 0.5f + 0.5f * sinf(g->anim_timer * 8.f);
                        SDL_SetRenderDrawColor(r, 255, 0, 60, (Uint8)(100 * pulse));
                    }
                    SDL_RenderFillRect(r, &cr);
                    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
                    /* Border */
                    if (g->placed_correct)
                        draw_glow_rect(r, cr, 0, 255, 120, 4);
                    else
                        draw_glow_rect(r, cr, 255, 0, 60, 4);
                } else {
                    /* Empty hole – pulsing outline */
                    float pulse = 0.5f + 0.5f * sinf(SDL_GetTicks() * 0.004f);
                    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(r, 255, 0, 180, (Uint8)(40 + 30 * pulse));
                    SDL_RenderFillRect(r, &cr);
                    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
                    draw_glow_rect(r, cr, 255, 0, 180, 5);

                    /* Question mark blocks */
                    int qx = cr.x + CELL_W/2 - 8;
                    int qy = cr.y + CELL_H/2 - 10;
                    SDL_SetRenderDrawColor(r, 255, 0, 180, (Uint8)(200 * pulse));
                    SDL_Rect qb = {qx, qy, 16, 20};
                    SDL_RenderFillRect(r, &qb);
                }
            } else {
                /* Normal piece */
                if (g->cells[idx]) {
                    SDL_RenderCopy(r, g->cells[idx], NULL, &cr);
                }
                /* Subtle grid line */
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, 0, 180, 220, 50);
                SDL_RenderDrawRect(r, &cr);
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
            }
        }
    }

    /* ── Right panel: choices ── */
    /* Panel bg */
    SDL_Rect panel_bg = {PANEL_X - 20, PANEL_Y - 20,
                          CHOICE_W + 40,
                          NUM_CHOICES * (CHOICE_H + CHOICE_SPACING) - CHOICE_SPACING + 40};
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 10, 0, 25, 180);
    SDL_RenderFillRect(r, &panel_bg);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    draw_glow_rect(r, panel_bg, 80, 0, 140, 4);

    /* Draw each choice */
    for (int i = 0; i < NUM_CHOICES; i++) {
        Choice *ch = &g->choices[i];
        if (!ch->tex) continue;

        int dx = ch->dragging ? ch->drag_x - ch->drag_off_x : ch->sx;
        int dy = ch->dragging ? ch->drag_y - ch->drag_off_y : ch->sy;
        SDL_Rect dst = {dx, dy, CHOICE_W, CHOICE_H};

        /* Glow when dragging */
        if (ch->dragging) {
            draw_glow_rect(r, dst, 255, 0, 180, 6);
            /* Drop shadow */
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(r, 0, 0, 0, 100);
            SDL_Rect sh = {dx + 6, dy + 6, CHOICE_W, CHOICE_H};
            SDL_RenderFillRect(r, &sh);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        } else {
            draw_glow_rect(r, dst, 0, 160, 200, 3);
        }

        SDL_RenderCopy(r, ch->tex, NULL, &dst);

        /* Number label */
        SDL_Rect num = {ch->sx + CHOICE_W - 24, ch->sy, 22, 22};
        SDL_SetRenderDrawColor(r, 180, 0, 255, 200);
        SDL_RenderFillRect(r, &num);
    }

    /* ── Arrow pointing from panel to hole ── */
    if (!g->piece_placed) {
        SDL_Rect hole_rect = cell_rect(g->missing_col, g->missing_row);
        int ax1 = PANEL_X - 25;
        int ay1 = PANEL_Y + (NUM_CHOICES * (CHOICE_H + CHOICE_SPACING)) / 2 - 20;
        int ax2 = hole_rect.x + CELL_W + 10;
        int ay2 = hole_rect.y + CELL_H / 2;

        float pulse = 0.5f + 0.5f * sinf(SDL_GetTicks() * 0.005f);
        SDL_SetRenderDrawColor(r, 0, (Uint8)(200 * pulse), (Uint8)(255 * pulse), 255);
        SDL_RenderDrawLine(r, ax1, ay1, ax2, ay2);
        /* Arrowhead */
        SDL_RenderDrawLine(r, ax2, ay2, ax2 + 10, ay2 - 8);
        SDL_RenderDrawLine(r, ax2, ay2, ax2 + 10, ay2 + 8);
    }

    draw_timer_bar(g);
    draw_score_dots(g);

    /* Round indicator top-right */
    for (int i = 0; i < 5; i++) {
        SDL_Rect ri = {WINDOW_W - 40, 20 + i * 18, 12, 12};
        if (i < g->round)
            SDL_SetRenderDrawColor(r, 0, 255, 150, 255);
        else if (i == g->round)
            SDL_SetRenderDrawColor(r, 255, 200, 0, 255);
        else
            SDL_SetRenderDrawColor(r, 30, 0, 50, 255);
        SDL_RenderFillRect(r, &ri);
    }
}

/* ══════════════════════════════════════════════════════════════
   DRAW – SUCCESS
══════════════════════════════════════════════════════════════ */

static void draw_success(Game *g) {
    SDL_Renderer *r = g->renderer;
    float t = g->anim_timer;

    /* Flash BG */
    Uint8 flash = (Uint8)(20 + 15 * fabs(sin(t * 3.0)));
    SDL_SetRenderDrawColor(r, 0, flash, (Uint8)(flash * 2), 255);
    SDL_RenderClear(r);

    /* Rotozoom the completed image */
    int img_idx = (g->round - 1 + 5) % 5;
    if (img_idx < 0) img_idx = 0;
    SDL_Texture *src = g->full_images[img_idx];
    if (src) {
        SDL_Texture *rz = rotozoom_tex(r, src, g->rotozoom_angle,
                                        g->rotozoom_scale * 0.65);
        if (rz) {
            int rw, rh;
            SDL_QueryTexture(rz, NULL, NULL, &rw, &rh);
            SDL_Rect dst = {WINDOW_W/2 - rw/2, WINDOW_H/2 - rh/2, rw, rh};
            SDL_SetTextureAlphaMod(rz, 210);
            SDL_RenderCopy(r, rz, NULL, &dst);
            SDL_DestroyTexture(rz);
        }
    }

    /* Firework bursts */
    for (int i = 0; i < 48; i++) {
        float a = i * (float)M_PI * 2.f / 48.f + t * 0.7f;
        float dist = 160.f + 80.f * (float)fabs(sin(t * 1.5f + i * 0.4f));
        int px = WINDOW_W/2 + (int)(dist * cosf(a));
        int py = WINDOW_H/2 + (int)(dist * sinf(a));
        Uint8 pr = (Uint8)(128 + 127 * sinf(t * 3.f + i * 0.5f));
        Uint8 pg = (Uint8)(128 + 127 * sinf(t * 2.f + i * 0.9f));
        Uint8 pb = (Uint8)(128 + 127 * sinf(t * 4.f + i * 0.3f));
        SDL_SetRenderDrawColor(r, pr, pg, pb, 255);
        SDL_Rect star = {px - 5, py - 5, 10, 10};
        SDL_RenderFillRect(r, &star);
    }

    /* ── Build SUCCES! banner texture then rotozoom it ── */
    int ban_w = 420, ban_h = 90;
    SDL_Texture *ban_tex = SDL_CreateTexture(r,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, ban_w, ban_h);
    if (ban_tex) {
        SDL_SetRenderTarget(r, ban_tex);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(r, 0, 28, 18, 220);
        SDL_RenderClear(r);
        /* Glow border inside the texture */
        for (int gw = 7; gw >= 0; gw--) {
            Uint8 ga = (Uint8)(255 * (1.f - gw / 7.f));
            SDL_SetRenderDrawColor(r, 0, 255, 160, ga);
            SDL_Rect gb = {gw, gw, ban_w - 2*gw, ban_h - 2*gw};
            SDL_RenderDrawRect(r, &gb);
        }
        /* Letter blocks */
        const char *w = "SUCCES!";
        int wl = (int)strlen(w);
        int bkw = 38, bkh = 50;
        int wx = ban_w/2 - wl * (bkw + 6) / 2;
        for (int ci = 0; ci < wl; ci++) {
            float br = 0.6f + 0.4f * sinf(t * 5.f + ci * 0.9f);
            SDL_SetRenderDrawColor(r, 0, (Uint8)(255 * br), (Uint8)(160 * br), 255);
            SDL_Rect blk = {wx + ci * (bkw + 6), ban_h/2 - bkh/2, bkw, bkh};
            SDL_RenderFillRect(r, &blk);
            SDL_SetRenderDrawColor(r, 200, 255, 220, 180);
            SDL_Rect hi = {blk.x + 2, blk.y + 2, blk.w - 4, 6};
            SDL_RenderFillRect(r, &hi);
        }
        SDL_SetRenderTarget(r, NULL);
        /* Rotozoom the banner – gentle wobble + pulse scale */
        double ban_angle = 8.0 * sin(t * 2.5);
        double ban_scale = 1.0 + 0.12 * fabs(sin(t * 3.5));
        SDL_Texture *rz_ban = rotozoom_tex(r, ban_tex, ban_angle, ban_scale);
        SDL_DestroyTexture(ban_tex);
        if (rz_ban) {
            int rw2, rh2;
            SDL_QueryTexture(rz_ban, NULL, NULL, &rw2, &rh2);
            SDL_Rect bdst = {WINDOW_W/2 - rw2/2, WINDOW_H - rh2 - 20, rw2, rh2};
            SDL_SetTextureBlendMode(rz_ban, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(r, rz_ban, NULL, &bdst);
            SDL_DestroyTexture(rz_ban);
        }
    }
}

/* ══════════════════════════════════════════════════════════════
   DRAW – FAILURE
══════════════════════════════════════════════════════════════ */

static void draw_failure(Game *g) {
    SDL_Renderer *r = g->renderer;
    float t = g->anim_timer;

    Uint8 flash = (Uint8)(15 + 12 * fabs(sin(t * 4.5)));
    SDL_SetRenderDrawColor(r, flash, 0, (Uint8)(flash / 3), 255);
    SDL_RenderClear(r);

    /* Rotozoom spinning */
    SDL_Texture *src = g->full_images[g->img_idx];
    if (src) {
        SDL_Texture *rz = rotozoom_tex(r, src,
                                        g->rotozoom_angle * 2.5,
                                        0.25 + 0.35 * fabs(sin(t * 1.2)));
        if (rz) {
            int rw, rh;
            SDL_QueryTexture(rz, NULL, NULL, &rw, &rh);
            SDL_Rect dst = {WINDOW_W/2 - rw/2, WINDOW_H/2 - rh/2, rw, rh};
            SDL_SetTextureAlphaMod(rz, 130);
            SDL_RenderCopy(r, rz, NULL, &dst);
            SDL_DestroyTexture(rz);
        }
    }

    /* Shatter lines */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 255, 0, 60, 180);
    for (int i = 0; i < 14; i++) {
        float a = i * (float)M_PI / 7.f + t * 0.4f;
        float len = 200.f + 150.f * (float)((i % 3) / 2.f);
        SDL_RenderDrawLine(r, WINDOW_W/2, WINDOW_H/2,
            WINDOW_W/2 + (int)(len * cosf(a)),
            WINDOW_H/2 + (int)(len * sinf(a)));
    }
    /* Static noise */
    srand((unsigned)(t * 1000));
    SDL_SetRenderDrawColor(r, 255, 0, 60, 50);
    for (int i = 0; i < 600; i++)
        SDL_RenderDrawPoint(r, rand() % WINDOW_W, rand() % WINDOW_H);
    srand((unsigned)time(NULL));
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* ── Build ECHEC! banner texture then rotozoom it ── */
    int ban_w = 360, ban_h = 90;
    SDL_Texture *ban_tex = SDL_CreateTexture(r,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, ban_w, ban_h);
    if (ban_tex) {
        SDL_SetRenderTarget(r, ban_tex);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(r, 28, 0, 12, 220);
        SDL_RenderClear(r);
        /* Glow border */
        for (int gw = 7; gw >= 0; gw--) {
            Uint8 ga = (Uint8)(255 * (1.f - gw / 7.f));
            SDL_SetRenderDrawColor(r, 255, 0, 60, ga);
            SDL_Rect gb = {gw, gw, ban_w - 2*gw, ban_h - 2*gw};
            SDL_RenderDrawRect(r, &gb);
        }
        /* Letter blocks */
        const char *w = "ECHEC!";
        int wl = (int)strlen(w);
        int bkw = 38, bkh = 50;
        int wx = ban_w/2 - wl * (bkw + 6) / 2;
        for (int ci = 0; ci < wl; ci++) {
            float br = 0.6f + 0.4f * sinf(t * 6.f + ci);
            SDL_SetRenderDrawColor(r, (Uint8)(255 * br), 0, (Uint8)(60 * br), 255);
            SDL_Rect blk = {wx + ci * (bkw + 6), ban_h/2 - bkh/2, bkw, bkh};
            SDL_RenderFillRect(r, &blk);
            /* Dark crack line across each block */
            SDL_SetRenderDrawColor(r, 0, 0, 0, 120);
            SDL_RenderDrawLine(r, blk.x, blk.y + bkh/2,
                               blk.x + bkw, blk.y + bkh/2);
        }
        SDL_SetRenderTarget(r, NULL);
        /* Rotozoom: aggressive shake + pulse */
        double ban_angle = 12.0 * sin(t * 4.0);
        double ban_scale = 1.0 + 0.18 * fabs(sin(t * 5.0));
        SDL_Texture *rz_ban = rotozoom_tex(r, ban_tex, ban_angle, ban_scale);
        SDL_DestroyTexture(ban_tex);
        if (rz_ban) {
            int rw2, rh2;
            SDL_QueryTexture(rz_ban, NULL, NULL, &rw2, &rh2);
            SDL_Rect bdst = {WINDOW_W/2 - rw2/2, WINDOW_H - rh2 - 20, rw2, rh2};
            SDL_SetTextureBlendMode(rz_ban, SDL_BLENDMODE_BLEND);
            SDL_RenderCopy(r, rz_ban, NULL, &bdst);
            SDL_DestroyTexture(rz_ban);
        }
    }
}

/* ══════════════════════════════════════════════════════════════
   MASTER DRAW
══════════════════════════════════════════════════════════════ */

void game_draw(Game *g) {
    SDL_RenderClear(g->renderer);
    switch (g->state) {
    case STATE_LOADING: draw_loading(g);  break;
    case STATE_PLAYING: draw_playing(g);  break;
    case STATE_SUCCESS: draw_success(g);  break;
    case STATE_FAILURE: draw_failure(g);  break;
    }
    SDL_RenderPresent(g->renderer);
}

/* ══════════════════════════════════════════════════════════════
   MOUSE
══════════════════════════════════════════════════════════════ */

void game_on_mouse_down(Game *g, int x, int y) {
    if (g->state != STATE_PLAYING) return;
    if (g->piece_placed) return;

    for (int i = 0; i < NUM_CHOICES; i++) {
        Choice *ch = &g->choices[i];
        if (!ch->tex) continue;
        if (x >= ch->sx && x <= ch->sx + CHOICE_W &&
            y >= ch->sy && y <= ch->sy + CHOICE_H) {
            ch->dragging    = 1;
            ch->drag_x      = x;
            ch->drag_y      = y;
            ch->drag_off_x  = x - ch->sx;
            ch->drag_off_y  = y - ch->sy;
        }
    }
}

void game_on_mouse_move(Game *g, int x, int y) {
    for (int i = 0; i < NUM_CHOICES; i++) {
        if (g->choices[i].dragging) {
            g->choices[i].drag_x = x;
            g->choices[i].drag_y = y;
        }
    }
}

void game_on_mouse_up(Game *g, int x, int y) {
    if (g->state != STATE_PLAYING) return;

    SDL_Rect hole = cell_rect(g->missing_col, g->missing_row);

    for (int i = 0; i < NUM_CHOICES; i++) {
        Choice *ch = &g->choices[i];
        if (!ch->dragging) continue;
        ch->dragging = 0;

        /* Check if dropped on the hole */
        int center_x = x;
        int center_y = y;
        if (center_x >= hole.x && center_x <= hole.x + CELL_W &&
            center_y >= hole.y && center_y <= hole.y + CELL_H) {

            /* Snap: store placed tex (scaled to CELL_W x CELL_H) */
            g->placed_tex     = crop_texture(g->renderer, ch->tex,
                                              (SDL_Rect){0,0,CHOICE_W,CHOICE_H},
                                              CELL_W, CELL_H);
            g->piece_placed   = 1;
            g->placed_correct = ch->is_correct;
            g->anim_timer     = 0.f;

            /* Reveal correct piece position (show it faintly) */
            return;
        }
        /* else snap back – dragging already 0, sx/sy unchanged */
    }
}
