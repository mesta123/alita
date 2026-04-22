#ifndef MINIMAP_H
#define MINIMAP_H

#include <SDL2/SDL.h>
#include "player.h"
#include "level.h"
#include "platform.h"

#define MINIMAP_WIDTH 120
#define MINIMAP_HEIGHT 70
#define MINIMAP_MARGIN 10

typedef struct {
    SDL_Rect position;
    float scaleX, scaleY;
    int visible;
    SDL_Texture *levelPreview;
} Minimap;

void minimapInit(Minimap *mm, SDL_Renderer *r, LevelManager *lm);
void minimapUpdate(Minimap *mm, SDL_Renderer *r, LevelManager *lm, 
                   Player *player, PlatformManager *pm);
void minimapRender(Minimap *mm, SDL_Renderer *r, LevelManager *lm,
                   Player *player, PlatformManager *pm);
void minimapToggle(Minimap *mm);
void minimapDestroy(Minimap *mm);
void minimapSetLevel(Minimap *mm, LevelManager *lm);

#endif
