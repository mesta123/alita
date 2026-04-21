#ifndef IMG_BUTTON_H
#define IMG_BUTTON_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

typedef enum {
    BTN_STATE_NORMAL = 0,
    BTN_STATE_HOVER,
    BTN_STATE_PRESSED
} ButtonState;

typedef struct {
    SDL_Rect rect;
    SDL_Texture *tex_normal;
    SDL_Texture *tex_hover;
    SDL_Texture *tex_pressed;
    ButtonState state;
    int is_down;
    int was_hover;
} ImgButton;

SDL_Texture* loadTex(SDL_Renderer *r, const char *filename);
void initButton(SDL_Renderer *r, ImgButton *b, SDL_Rect rect,
                const char *normal, const char *hover, const char *pressed);
void destroyButton(ImgButton *b);
void renderButton(SDL_Renderer *r, ImgButton *b);
int mouseInside(SDL_Rect rect, int x, int y);
void updateButtonState(ImgButton *b, int mx, int my, int mouseDown,
                       Mix_Chunk *sfx_hover);

#endif
