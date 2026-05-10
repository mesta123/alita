/*
 * menu.c  –  Sous-menu Sauvegarde SDL2
 *
 * Écran 1 : background + "Voulez-vous sauvegarder votre jeu ?" + [Oui] [Non]
 *   -> Oui  : Écran 2  background + [Charger le jeu] [Nouvelle partie]
 *   -> Non  : ferme le programme
 *
 * Compilation :
 *   gcc menu.c -o menu $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_mixer SDL2_ttf)
 */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

/* ═══════════════════════════════════════════════════════════════════════════
 *  Asset path resolution  (finds files next to the executable)
 * ═════════════════════════════════════════════════════════════════════════ */
static char g_asset_dir[4096] = ".";

void resolve_asset_dir(void)
{
    char tmp[4096] = {0};
    ssize_t n = readlink("/proc/self/exe", tmp, sizeof(tmp) - 1);
    if (n > 0) {
        tmp[n] = '\0';
        strncpy(g_asset_dir, dirname(tmp), sizeof(g_asset_dir) - 1);
    }
}

const char *asset(const char *filename)
{
    static char buf[8192];
    snprintf(buf, sizeof(buf) - 1, "%s/%s", g_asset_dir, filename);
    return buf;
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Constants
 * ═════════════════════════════════════════════════════════════════════════ */
#define WIN_W  1280
#define WIN_H   720

#define BTN_W   320
#define BTN_H    80

/* ═══════════════════════════════════════════════════════════════════════════
 *  Enums & structs
 * ═════════════════════════════════════════════════════════════════════════ */
typedef enum {
    SCR_SAVE_PROMPT,   /* background + question + Oui / Non             */
    SCR_CHOICE,        /* background + Charger le jeu / Nouvelle partie  */
    SCR_QUIT
} Screen;

typedef struct {
    SDL_Texture *normal;
    SDL_Texture *hover;
    SDL_Rect     dst;
    int          is_hovered;
} Button;

typedef struct {
    SDL_Window   *win;
    SDL_Renderer *ren;

    SDL_Texture  *bg;
    TTF_Font     *font_title;
    TTF_Font     *font_hint;

    Button btn_oui;
    Button btn_non;
    Button btn_charger;
    Button btn_nouvelle;

    Mix_Music *music;
    Mix_Chunk *sfx_hover;
    Mix_Chunk *sfx_click;

    Screen screen;
    int    running;
} App;

/* ═══════════════════════════════════════════════════════════════════════════
 *  Utility
 * ═════════════════════════════════════════════════════════════════════════ */
SDL_Texture *tex_load(SDL_Renderer *ren, const char *path)
{
    SDL_Surface *s = IMG_Load(path);
    if (!s) { fprintf(stderr, "IMG_Load %s : %s\n", path, IMG_GetError()); return NULL; }
    SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
    SDL_FreeSurface(s);
    return t;
}

void draw_text(SDL_Renderer *ren, TTF_Font *font, const char *txt,
               SDL_Color col, int cx, int cy)
{
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, txt, col);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
    SDL_FreeSurface(s);
    if (!t) return;
    int w, h;
    SDL_QueryTexture(t, NULL, NULL, &w, &h);
    SDL_Rect dst = { cx - w/2, cy - h/2, w, h };
    SDL_RenderCopy(ren, t, NULL, &dst);
    SDL_DestroyTexture(t);
}

void draw_text_shadow(SDL_Renderer *ren, TTF_Font *font, const char *txt,
                      SDL_Color col, int cx, int cy)
{
    SDL_Color shadow = {0, 0, 0, 200};
    draw_text(ren, font, txt, shadow, cx + 3, cy + 3);
    draw_text(ren, font, txt, col,    cx,     cy);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Button helpers
 * ═════════════════════════════════════════════════════════════════════════ */
void btn_init(Button *b, SDL_Renderer *ren,
              const char *path_normal, const char *path_hover,
              int x, int y, int w, int h)
{
    b->normal     = tex_load(ren, path_normal);
    b->hover      = tex_load(ren, path_hover);
    b->dst        = (SDL_Rect){ x, y, w, h };
    b->is_hovered = 0;
}

void btn_free(Button *b)
{
    if (b->normal) { SDL_DestroyTexture(b->normal); b->normal = NULL; }
    if (b->hover)  { SDL_DestroyTexture(b->hover);  b->hover  = NULL; }
}

void btn_draw(SDL_Renderer *ren, Button *b)
{
    SDL_Texture *tex = b->is_hovered ? b->hover : b->normal;
    if (tex) SDL_RenderCopy(ren, tex, NULL, &b->dst);
}

int btn_contains(Button *b, int x, int y)
{
    return x >= b->dst.x && x < b->dst.x + b->dst.w &&
           y >= b->dst.y && y < b->dst.y + b->dst.h;
}

int btn_update_hover(Button *b, int mx, int my)
{
    int prev      = b->is_hovered;
    b->is_hovered = btn_contains(b, mx, my);
    return b->is_hovered != prev;
}

/* Returns the x-start of button index i in a centred row of n buttons */
int row_x(int i, int n, int bw, int gap)
{
    int total = n * bw + (n - 1) * gap;
    return WIN_W / 2 - total / 2 + i * (bw + gap);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Init / Cleanup
 * ═════════════════════════════════════════════════════════════════════════ */
int app_init(App *app)
{
    memset(app, 0, sizeof(*app));
    resolve_asset_dir();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return -1;
    }
    if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)) {
        fprintf(stderr, "IMG_Init failed\n"); return -1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError()); return -1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError()); return -1;
    }

    app->win = SDL_CreateWindow("Menu Sauvegarde",
                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                 WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (!app->win) { fprintf(stderr, "Window: %s\n", SDL_GetError()); return -1; }

    app->ren = SDL_CreateRenderer(app->win, -1,
                 SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app->ren) { fprintf(stderr, "Renderer: %s\n", SDL_GetError()); return -1; }

    SDL_SetRenderDrawBlendMode(app->ren, SDL_BLENDMODE_BLEND);

    /* Textures */
    app->bg = tex_load(app->ren, asset("background_.png"));

    /* Fonts */
    app->font_title = TTF_OpenFont(asset("font.ttf"), 50);
    app->font_hint  = TTF_OpenFont(asset("font.ttf"), 26);
    if (!app->font_title || !app->font_hint) {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError()); return -1;
    }

    /* Audio */
    app->music     = Mix_LoadMUS(asset("music.mp3"));
    app->sfx_hover = Mix_LoadWAV(asset("hover.ogg"));
    app->sfx_click = Mix_LoadWAV(asset("click.mp3"));
    if (app->music) { Mix_VolumeMusic(55); Mix_PlayMusic(app->music, -1); }

    /* ── Screen 1: Oui / Non side by side, centred, 62 % down ── */
    int gap1  = 50;
    int btn_y = WIN_H * 62 / 100 - BTN_H / 2;

    btn_init(&app->btn_oui,
             app->ren,
             asset("ouihover.jpeg"), asset("oui.jpeg"),
             row_x(0, 2, BTN_W, gap1), btn_y, BTN_W, BTN_H);

    btn_init(&app->btn_non,
             app->ren,
             asset("non.png"), asset("non_hover.png"),
             row_x(1, 2, BTN_W, gap1), btn_y, BTN_W, BTN_H);

    /* ── Screen 2: Charger / Nouvelle stacked, centred, mid-screen ── */
    int b2w = 380, b2h = 85;
    int mid = WIN_H / 2;

    btn_init(&app->btn_charger,
             app->ren,
             asset("charger_le_jeu_.png"), asset("charger_le_jeu_hover.png"),
             WIN_W / 2 - b2w / 2, mid - b2h - 25, b2w, b2h);

    btn_init(&app->btn_nouvelle,
             app->ren,
             asset("nouvelle_partie.png"), asset("nouvelle_partie_hover.png"),
             WIN_W / 2 - b2w / 2, mid + 25, b2w, b2h);

    app->screen  = SCR_SAVE_PROMPT;
    app->running = 1;
    return 0;
}

void app_cleanup(App *app)
{
    btn_free(&app->btn_oui);
    btn_free(&app->btn_non);
    btn_free(&app->btn_charger);
    btn_free(&app->btn_nouvelle);

    if (app->bg)         SDL_DestroyTexture(app->bg);
    if (app->font_title) TTF_CloseFont(app->font_title);
    if (app->font_hint)  TTF_CloseFont(app->font_hint);
    if (app->sfx_hover)  Mix_FreeChunk(app->sfx_hover);
    if (app->sfx_click)  Mix_FreeChunk(app->sfx_click);
    if (app->music)      Mix_FreeMusic(app->music);

    if (app->ren) SDL_DestroyRenderer(app->ren);
    if (app->win) SDL_DestroyWindow(app->win);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Drawing
 * ═════════════════════════════════════════════════════════════════════════ */
void draw_bg(App *app)
{
    if (app->bg) {
        SDL_Rect dst = {0, 0, WIN_W, WIN_H};
        SDL_RenderCopy(app->ren, app->bg, NULL, &dst);
    } else {
        SDL_SetRenderDrawColor(app->ren, 5, 0, 15, 255);
        SDL_RenderClear(app->ren);
    }
}

/* Thin neon glow line centred horizontally */
void draw_glow_line(SDL_Renderer *ren, int y)
{
    int x0 = WIN_W/2 - 420, x1 = WIN_W/2 + 420;
    SDL_SetRenderDrawColor(ren, 110, 0, 180, 55);
    SDL_RenderDrawLine(ren, x0, y-1, x1, y-1);
    SDL_SetRenderDrawColor(ren, 160, 40, 255, 130);
    SDL_RenderDrawLine(ren, x0, y,   x1, y);
    SDL_SetRenderDrawColor(ren, 200, 100, 255, 55);
    SDL_RenderDrawLine(ren, x0, y+1, x1, y+1);
}

/* Screen 1 */
void render_save_prompt(App *app)
{
    draw_bg(app);

    /* Dark translucent band so text is readable on any background */
    SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 150);
    SDL_Rect band = { 0, WIN_H * 36 / 100, WIN_W, WIN_H * 36 / 100 };
    SDL_RenderFillRect(app->ren, &band);

    int text_y = WIN_H * 48 / 100;

    draw_glow_line(app->ren, text_y - 42);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color hint  = {170, 130, 210, 210};

    draw_text_shadow(app->ren, app->font_title,
                     "Voulez-vous sauvegarder votre jeu ?",
                     white, WIN_W / 2, text_y);

    draw_glow_line(app->ren, text_y + 42);

    btn_draw(app->ren, &app->btn_oui);
    btn_draw(app->ren, &app->btn_non);

    draw_text(app->ren, app->font_hint,
              "Choisissez une option", hint,
              WIN_W / 2, WIN_H * 80 / 100);
}

/* Screen 2 */
void render_choice(App *app)
{
    draw_bg(app);

    SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 150);
    SDL_Rect band = { 0, WIN_H * 28 / 100, WIN_W, WIN_H * 48 / 100 };
    SDL_RenderFillRect(app->ren, &band);

    int text_y = WIN_H * 38 / 100;

    draw_glow_line(app->ren, text_y - 42);

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {130, 255, 195, 255};
    SDL_Color hint  = {170, 130, 210, 210};

    draw_text_shadow(app->ren, app->font_title,
                     "Que souhaitez-vous faire ?",
                     white, WIN_W / 2, text_y);

    draw_glow_line(app->ren, text_y + 42);

    btn_draw(app->ren, &app->btn_charger);
    btn_draw(app->ren, &app->btn_nouvelle);

    draw_text(app->ren, app->font_hint,
              "Jeu sauvegarde avec succes !", green,
              WIN_W / 2, WIN_H * 80 / 100);

    draw_text(app->ren, app->font_hint,
              "Echap = retour", hint,
              WIN_W / 2, WIN_H * 87 / 100);
}

void render_frame(App *app)
{
    SDL_RenderClear(app->ren);
    switch (app->screen) {
        case SCR_SAVE_PROMPT: render_save_prompt(app); break;
        case SCR_CHOICE:      render_choice(app);      break;
        default:              draw_bg(app);             break;
    }
    SDL_RenderPresent(app->ren);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Events
 * ═════════════════════════════════════════════════════════════════════════ */
void handle_events(App *app)
{
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    /* Hover updates – play sfx on change */
    int changed = 0;
    if (app->screen == SCR_SAVE_PROMPT) {
        changed |= btn_update_hover(&app->btn_oui, mx, my);
        changed |= btn_update_hover(&app->btn_non, mx, my);
    } else if (app->screen == SCR_CHOICE) {
        changed |= btn_update_hover(&app->btn_charger,  mx, my);
        changed |= btn_update_hover(&app->btn_nouvelle, mx, my);
    }
    if (changed && app->sfx_hover)
        Mix_PlayChannel(-1, app->sfx_hover, 0);

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {

        if (ev.type == SDL_QUIT)
            app->running = 0;

        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) {
            if (app->screen == SCR_CHOICE)
                app->screen = SCR_SAVE_PROMPT;
            else
                app->running = 0;
        }

        if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
            int cx = ev.button.x, cy = ev.button.y;

            if (app->screen == SCR_SAVE_PROMPT) {

                if (btn_contains(&app->btn_oui, cx, cy)) {
                    if (app->sfx_click) Mix_PlayChannel(-1, app->sfx_click, 0);
                    SDL_Delay(100);
                    app->screen = SCR_CHOICE;   /* -> affiche Charger / Nouvelle */
                }

                if (btn_contains(&app->btn_non, cx, cy)) {
                    if (app->sfx_click) Mix_PlayChannel(-1, app->sfx_click, 0);
                    SDL_Delay(100);
                    app->running = 0;           /* -> ferme le programme */
                }

            } else if (app->screen == SCR_CHOICE) {

                if (btn_contains(&app->btn_charger, cx, cy)) {
                    if (app->sfx_click) Mix_PlayChannel(-1, app->sfx_click, 0);
                    printf("[ACTION] Charger le jeu\n");
                    SDL_Delay(100);
                    /* TODO: charger la sauvegarde */
                }

                if (btn_contains(&app->btn_nouvelle, cx, cy)) {
                    if (app->sfx_click) Mix_PlayChannel(-1, app->sfx_click, 0);
                    printf("[ACTION] Nouvelle partie\n");
                    SDL_Delay(100);
                    /* TODO: lancer le sous-menu joueur */
                }
            }
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  Main
 * ═════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    App app;

    if (app_init(&app) < 0) {
        fprintf(stderr, "Initialisation echouee.\n");
        app_cleanup(&app);
        return EXIT_FAILURE;
    }

    while (app.running) {
        handle_events(&app);
        render_frame(&app);
        SDL_Delay(16);
    }

    app_cleanup(&app);
    return EXIT_SUCCESS;
}
