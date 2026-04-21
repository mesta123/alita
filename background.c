#include "background.h"
#include <stdio.h>

void loadLevel(Background *background, SDL_Renderer *renderer, int niveau)
{
    background->level = niveau;

    if (background->image)
    {
        SDL_DestroyTexture(background->image);
        background->image = NULL;
    }

    const char *chemin;

    if (niveau == 1)
        chemin = "images/background.png";
    else
        chemin = "images/background2.png";

    SDL_Surface *surface = IMG_Load(chemin);
    if (!surface)
        return;

    background->image = SDL_CreateTextureFromSurface(renderer, surface);
    background->largeurNiveau = surface->w;
    background->hauteurNiveau = surface->h;

    SDL_FreeSurface(surface);
}

void initBackground(Background *background, SDL_Renderer *renderer, int largeurEcran, int hauteurEcran)
{
    background->image = NULL;
    background->scrollX = 0;
    background->scrollAbsoluX = 0;
    background->direction = 0;
    background->finJeu = 0;
    background->score = 0;

    loadLevel(background, renderer, 1);

    background->camera.x = 0;
    background->camera.y = 0;
    background->camera.w = largeurEcran;
    background->camera.h = hauteurEcran;
}

void afficherBackground(Background *background, SDL_Renderer *renderer, int largeurEcran, int hauteurEcran)
{
    int decalage = background->scrollX % background->largeurNiveau;
    if (decalage < 0)
        decalage += background->largeurNiveau;

    SDL_Rect src1 = {decalage, 0, background->largeurNiveau - decalage, background->hauteurNiveau};
    SDL_Rect dst1 = {0, 0, src1.w * largeurEcran / background->largeurNiveau, hauteurEcran};

    SDL_RenderCopy(renderer, background->image, &src1, &dst1);

    SDL_Rect src2 = {0, 0, decalage, background->hauteurNiveau};
    SDL_Rect dst2 = {dst1.w, 0, largeurEcran - dst1.w, hauteurEcran};

    SDL_RenderCopy(renderer, background->image, &src2, &dst2);
}

void scrolling(Background *background, int deplacementX)
{
    if (background->finJeu)
        return;

    background->scrollAbsoluX += deplacementX;

    if (background->scrollAbsoluX < 0)
    {
        deplacementX -= background->scrollAbsoluX;
        background->scrollAbsoluX = 0;
    }

    background->scrollX = (background->scrollX + deplacementX) % background->largeurNiveau;

    if (background->scrollX < 0)
        background->scrollX += background->largeurNiveau;

    background->camera.x = background->scrollX;

    if (background->camera.x > background->largeurNiveau - background->camera.w)
        background->camera.x = background->largeurNiveau - background->camera.w;
}

void initTemps(Background *background)
{
    TTF_Init();
    background->startTime = SDL_GetTicks();
    background->font = TTF_OpenFont("font/arial.ttf", 24);

    background->color.r = 255;
    background->color.g = 255;
    background->color.b = 0;
    background->color.a = 255;
}

int getElapsedSeconds(Background *background)
{
    return (SDL_GetTicks() - background->startTime) / 1000;
}

void afficherTemps(Background *background, SDL_Renderer *renderer, int decalageX)
{
    char texte[50];
    sprintf(texte, "Time: %d s", getElapsedSeconds(background));

    SDL_Surface *surface = TTF_RenderText_Solid(background->font, texte, background->color);
    if (!surface)
        return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect position = {decalageX + 10, 10, surface->w, surface->h};

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &position);
    SDL_DestroyTexture(texture);
}

void afficherScore(Background *background, SDL_Renderer *renderer, int decalageX)
{
    char texte[50];
    sprintf(texte, "Score: %d", background->score);

    SDL_Surface *surface = TTF_RenderText_Solid(background->font, texte, background->color);
    if (!surface)
        return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect position = {decalageX + 10, 40, surface->w, surface->h};

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &position);
    SDL_DestroyTexture(texture);
}

void afficherWinMessage(Background *background, SDL_Renderer *renderer, int largeurEcran, int hauteurEcran)
{
    SDL_Surface *surface = TTF_RenderText_Solid(background->font, "YOU WIN!", background->color);
    if (!surface)
        return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect position =
    {
        largeurEcran / 2 - surface->w / 2,
        hauteurEcran / 2 - surface->h / 2,
        surface->w,
        surface->h
    };

    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, NULL, &position);
    SDL_DestroyTexture(texture);
}

void saveScore(int score)
{
    FILE *fichier = fopen("score.txt", "a");
    if (fichier)
    {
        fprintf(fichier, "Score: %d\n", score);
        fclose(fichier);
    }
}
