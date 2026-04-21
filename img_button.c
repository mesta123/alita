#include "img_button.h"
#include <stdio.h>
#include <string.h>

SDL_Texture* loadTex(SDL_Renderer *r, const char *filename) {
    char path[256];
    snprintf(path, sizeof(path), "img/%s", filename);
    
    SDL_Texture *t = IMG_LoadTexture(r, path);
    if (!t) {
        printf("IMG_LoadTexture error (%s): %s\n", path, IMG_GetError());
    }
    return t;
}

void initButton(SDL_Renderer *r, ImgButton *b, SDL_Rect rect,
                const char *normal, const char *hover, const char *pressed) {
    b->rect = rect;
    b->tex_normal = loadTex(r, normal);
    b->tex_hover = hover ? loadTex(r, hover) : NULL;
    b->tex_pressed = pressed ? loadTex(r, pressed) : NULL;
    b->state = BTN_STATE_NORMAL;
    b->is_down = 0;
    b->was_hover = 0;
}

void destroyButton(ImgButton *b) {
    if (b->tex_normal) SDL_DestroyTexture(b->tex_normal);
    if (b->tex_hover) SDL_DestroyTexture(b->tex_hover);
    if (b->tex_pressed) SDL_DestroyTexture(b->tex_pressed);
    b->tex_normal = b->tex_hover = b->tex_pressed = NULL;
}

void renderButton(SDL_Renderer *r, ImgButton *b) {
    SDL_Texture *t = b->tex_normal;
    if (b->state == BTN_STATE_HOVER && b->tex_hover)   
        t = b->tex_hover;
    else if (b->state == BTN_STATE_PRESSED && b->tex_pressed) 
        t = b->tex_pressed;
    
    if (t) SDL_RenderCopy(r, t, NULL, &b->rect);
}

int mouseInside(SDL_Rect rect, int x, int y) {
    return (x >= rect.x && x <= rect.x + rect.w &&
            y >= rect.y && y <= rect.y + rect.h);
}

void updateButtonState(ImgButton *b, int mx, int my, int mouseDown,
                       Mix_Chunk *sfx_hover) {
    int inside = mouseInside(b->rect, mx, my);

    if (inside && !b->was_hover && sfx_hover) {
        Mix_PlayChannel(-1, sfx_hover, 0);
        b->was_hover = 1;
    }
    if (!inside) b->was_hover = 0;

    if (inside) {
        b->state = mouseDown ? BTN_STATE_PRESSED : BTN_STATE_HOVER;
    } else {
        b->state = BTN_STATE_NORMAL;
    }
}
