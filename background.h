#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

typedef struct
{
    SDL_Texture *image;

    SDL_Rect camera;

    int largeurNiveau;
    int hauteurNiveau;

    int scrollX;
    int scrollAbsoluX;
    int direction;

    int level;
    int finJeu;

    Uint32 startTime;
    TTF_Font *font;
    SDL_Color color;
    int score;

} Background;

void initBackground(Background *background, SDL_Renderer *renderer, int largeurEcran, int hauteurEcran);
void loadLevel(Background *background, SDL_Renderer *renderer, int niveau);
void afficherBackground(Background *background, SDL_Renderer *renderer, int largeurEcran, int hauteurEcran);
void scrolling(Background *background, int deplacementX);

void initTemps(Background *background);
int getElapsedSeconds(Background *background);
void afficherTemps(Background *background, SDL_Renderer *renderer, int decalageX);
void afficherScore(Background *background, SDL_Renderer *renderer, int decalageX);

void afficherWinMessage(Background *background, SDL_Renderer *renderer, int largeurEcran, int hauteurEcran);
void saveScore(int score);

#endif
