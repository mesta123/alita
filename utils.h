#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>

int checkAABBCollision(SDL_Rect a, SDL_Rect b);
int pointInRect(int x, int y, SDL_Rect rect);
float lerp(float a, float b, float t);
int clamp(int value, int min, int max);

#endif
