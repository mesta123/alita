#include "level.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

static void levelInternalRender(LevelManager *lm, SDL_Renderer *renderer);
static void levelInternalDestroy(LevelManager *lm);

void levelManagerInit(LevelManager *lm, SDL_Renderer *r) {
    (void)r;
    
    lm->currentLevel = -1;
    lm->cameraX = 0;
    lm->cameraY = 0;
    lm->shakeTimer = 0;
    lm->shakeIntensity = 0;
    lm->showLevelUp = 0;
    lm->levelUpTimer = 0;
    lm->transitioning = 0;
    
    // === NIVEAU 1 ===
    strcpy(lm->levels[0].bgFile, "lvl1.jpeg");
    lm->levels[0].id = 0;
    lm->levels[0].bgTexture = NULL;
    lm->levels[0].collisionMap = NULL;
    lm->levels[0].width = GAME_WIDTH;
    lm->levels[0].height = GAME_HEIGHT;
    lm->levels[0].ladderCount = 0;
    lm->levels[0].hasSafetyNet = 1;
    
    lm->levels[0].portal.x = 600;
    lm->levels[0].portal.y = 150;
    lm->levels[0].portal.w = 40;
    lm->levels[0].portal.h = 60;
    lm->levels[0].portal.active = 1;
    lm->levels[0].portal.triggered = 0;
    
    // === NIVEAU 2 (JEU) ===
    strcpy(lm->levels[1].bgFile, "lvl2.jpeg");
    lm->levels[1].id = 1;
    lm->levels[1].bgTexture = NULL;
    lm->levels[1].collisionMap = NULL;
    lm->levels[1].width = 1600;
    lm->levels[1].height = 800;
    lm->levels[1].ladderCount = 2;
    lm->levels[1].hasSafetyNet = 0;
    
    // Échelle gauche - position ajustée pour être accessible
    lm->levels[1].ladders[0].x = 180;
    lm->levels[1].ladders[0].y = 250;
    lm->levels[1].ladders[0].w = 50;  // Plus large pour faciliter la détection
    lm->levels[1].ladders[0].h = 200;
    lm->levels[1].ladders[0].active = 1;
    
    // Échelle droite - position ajustée
    lm->levels[1].ladders[1].x = 1300;
    lm->levels[1].ladders[1].y = 80;
    lm->levels[1].ladders[1].w = 50;
    lm->levels[1].ladders[1].h = 250;
    lm->levels[1].ladders[1].active = 1;
    
    lm->levels[1].portal.x = 1500;
    lm->levels[1].portal.y = 100;
    lm->levels[1].portal.w = 50;
    lm->levels[1].portal.h = 70;
    lm->levels[1].portal.active = 1;
    lm->levels[1].portal.triggered = 0;
    
    // === ÉCRAN LEVEL UP ===
    strcpy(lm->levels[2].bgFile, "lvlup.jpeg");
    lm->levels[2].id = 2;
    lm->levels[2].bgTexture = NULL;
    lm->levels[2].collisionMap = NULL;
    lm->levels[2].width = GAME_WIDTH;
    lm->levels[2].height = GAME_HEIGHT;
    lm->levels[2].ladderCount = 0;
    lm->levels[2].hasSafetyNet = 0;
    lm->levels[2].portal.active = 0;
}

void levelManagerLoad(LevelManager *lm, SDL_Renderer *r, int levelId) {
    if (levelId < 0 || levelId >= MAX_LEVELS) return;
    
    if (lm->currentLevel >= 0 && lm->currentLevel != 2) {
        Level *old = &lm->levels[lm->currentLevel];
        if (old->bgTexture) {
            SDL_DestroyTexture(old->bgTexture);
            old->bgTexture = NULL;
        }
        if (old->collisionMap) {
            SDL_FreeSurface(old->collisionMap);
            old->collisionMap = NULL;
        }
    }
    
    Level *lvl = &lm->levels[levelId];
    
    const char* paths[20];
    int pathCount = 0;
    
    if (strcmp(lvl->bgFile, "lvl1.jpeg") == 0) {
        paths[pathCount++] = "lvl1.jpeg";
        paths[pathCount++] = "./lvl1.jpeg";
        paths[pathCount++] = "img/lvl1.jpeg";
        paths[pathCount++] = "../lvl1.jpeg";
    } else if (strcmp(lvl->bgFile, "lvl2.jpeg") == 0) {
        paths[pathCount++] = "lvl2.jpeg";
        paths[pathCount++] = "./lvl2.jpeg";
        paths[pathCount++] = "img/lvl2.jpeg";
        paths[pathCount++] = "../lvl2.jpeg";
    } else if (strcmp(lvl->bgFile, "lvlup.jpeg") == 0) {
        paths[pathCount++] = "lvlup.jpeg";
        paths[pathCount++] = "./lvlup.jpeg";
        paths[pathCount++] = "img/lvlup.jpeg";
        paths[pathCount++] = "../lvlup.jpeg";
    }
    paths[pathCount] = NULL;
    
    lvl->bgTexture = NULL;
    
    printf("\n=== Chargement niveau %d (fichier: %s) ===\n", levelId, lvl->bgFile);
    
    for (int i = 0; paths[i] != NULL; i++) {
        printf("Essai %d: '%s' -> ", i+1, paths[i]);
        lvl->bgTexture = IMG_LoadTexture(r, paths[i]);
        if (lvl->bgTexture) {
            printf("SUCCES!\n");
            break;
        } else {
            printf("ECHEC\n");
        }
    }
    
    if (!lvl->bgTexture) {
        printf("!!! AUCUNE IMAGE TROUVEE - Creation fond de secours !!!\n");
        
        int bgW = (levelId == 1) ? 1600 : GAME_WIDTH;
        int bgH = (levelId == 1) ? 800 : GAME_HEIGHT;
        
        lvl->bgTexture = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_TARGET, bgW, bgH);
        if (lvl->bgTexture) {
            SDL_SetTextureBlendMode(lvl->bgTexture, SDL_BLENDMODE_NONE);
            SDL_SetRenderTarget(r, lvl->bgTexture);
            
            if (levelId == 2) {
                SDL_SetRenderDrawColor(r, 255, 215, 0, 255);
                SDL_RenderClear(r);
            } else if (levelId == 1) {
                for (int y = 0; y < bgH; y++) {
                    Uint8 r_val = 50 + (y * 100 / bgH);
                    Uint8 g_val = 20 + (y * 50 / bgH);
                    Uint8 b_val = 100 + (y * 155 / bgH);
                    SDL_SetRenderDrawColor(r, r_val, g_val, b_val, 255);
                    SDL_RenderDrawLine(r, 0, y, bgW, y);
                }
            } else {
                SDL_SetRenderDrawColor(r, 135, 206, 235, 255);
                SDL_RenderClear(r);
            }
            SDL_SetRenderTarget(r, NULL);
        }
    } else {
        int imgWidth, imgHeight;
        SDL_QueryTexture(lvl->bgTexture, NULL, NULL, &imgWidth, &imgHeight);
        printf("Image chargee: %dx%d\n", imgWidth, imgHeight);
        
        if (levelId == 1) {
            lvl->width = imgWidth;
            lvl->height = imgHeight;
        } else {
            lvl->width = GAME_WIDTH;
            lvl->height = GAME_HEIGHT;
        }
        
        printf("Niveau defini: %dx%d\n", lvl->width, lvl->height);
    }
    
    lm->cameraX = 0;
    lm->cameraY = 0;
    lm->currentLevel = levelId;
    
    printf("=== Niveau %d actif ===\n\n", levelId);
}

static void levelInternalRender(LevelManager *lm, SDL_Renderer *renderer) {
    if (lm->currentLevel < 0) return;
    Level *lvl = &lm->levels[lm->currentLevel];
    
    if (lvl->bgTexture) {
        if (lm->currentLevel == 1) {
            SDL_Rect src = {lm->cameraX, lm->cameraY, GAME_WIDTH, GAME_HEIGHT};
            SDL_Rect dst = {0, 0, GAME_WIDTH, GAME_HEIGHT};
            SDL_RenderCopy(renderer, lvl->bgTexture, &src, &dst);
        } else {
            SDL_Rect dst = {0, 0, GAME_WIDTH, GAME_HEIGHT};
            SDL_RenderCopy(renderer, lvl->bgTexture, NULL, &dst);
        }
    } else {
        SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
        SDL_RenderClear(renderer);
    }
    
    // Échelles - dessinées avec offset caméra pour le niveau 2
    for (int i = 0; i < lvl->ladderCount; i++) {
        if (lvl->ladders[i].active) {
            int drawX = lvl->ladders[i].x;
            int drawY = lvl->ladders[i].y;
            
            // Appliquer offset caméra uniquement pour niveau 2
            if (lm->currentLevel == 1) {
                drawX -= lm->cameraX;
                drawY -= lm->cameraY;
            }
            
            SDL_Rect ladderRect = {drawX, drawY, lvl->ladders[i].w, lvl->ladders[i].h};
            
            // Couleur orange/jaune pour échelle
            SDL_SetRenderDrawColor(renderer, 255, 200, 100, 200);
            SDL_RenderFillRect(renderer, &ladderRect);
            
            // Barreaux
            SDL_SetRenderDrawColor(renderer, 200, 150, 50, 255);
            for (int j = 0; j < ladderRect.h; j += 15) {
                SDL_RenderDrawLine(renderer, ladderRect.x, ladderRect.y + j, 
                                 ladderRect.x + ladderRect.w, ladderRect.y + j);
            }
            
            // Bordure
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(renderer, &ladderRect);
        }
    }
    
    // Portail (pas sur écran level up)
    if (lm->currentLevel != 2 && lvl->portal.active && !lvl->portal.triggered) {
        int portalX = lvl->portal.x;
        int portalY = lvl->portal.y;
        
        if (lm->currentLevel == 1) {
            portalX -= lm->cameraX;
            portalY -= lm->cameraY;
        }
        
        SDL_Rect portalRect = {portalX, portalY, lvl->portal.w, lvl->portal.h};
        
        Uint32 ticks = SDL_GetTicks();
        int colorShift = (ticks / 100) % 360;
        
        Uint8 red = (Uint8)(128 + 127 * sin(colorShift * 3.14159 / 180));
        Uint8 green = (Uint8)(128 + 127 * sin((colorShift + 120) * 3.14159 / 180));
        Uint8 blue = (Uint8)(128 + 127 * sin((colorShift + 240) * 3.14159 / 180));
        
        SDL_SetRenderDrawColor(renderer, red, green, blue, 200);
        SDL_RenderFillRect(renderer, &portalRect);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &portalRect);
        
        for (int i = 0; i < 5; i++) {
            int px = portalX + rand() % lvl->portal.w;
            int py = portalY + rand() % lvl->portal.h;
            SDL_Rect sparkle = {px, py, 3, 3};
            SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255);
            SDL_RenderFillRect(renderer, &sparkle);
        }
    }
    
    // Filet de sécurité niveau 1
    if (lvl->hasSafetyNet) {
        SDL_Rect net = {0 - lm->cameraX, lvl->height - 30 - lm->cameraY, lvl->width, 30};
        SDL_SetRenderDrawColor(renderer, 50, 200, 50, 150);
        SDL_RenderFillRect(renderer, &net);
        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 200);
        for (int x = 0; x < lvl->width; x += 20) {
            SDL_RenderDrawLine(renderer, x - lm->cameraX, net.y, x - lm->cameraX, net.y + 30);
        }
        for (int y = 0; y < 30; y += 10) {
            SDL_RenderDrawLine(renderer, 0 - lm->cameraX, net.y + y, lvl->width - lm->cameraX, net.y + y);
        }
    }
}

void levelManagerRender(LevelManager *lm, SDL_Renderer *r) {
    levelInternalRender(lm, r);
}

static void levelInternalDestroy(LevelManager *lm) {
    for (int i = 0; i < MAX_LEVELS; i++) {
        if (lm->levels[i].bgTexture) {
            SDL_DestroyTexture(lm->levels[i].bgTexture);
            lm->levels[i].bgTexture = NULL;
        }
        if (lm->levels[i].collisionMap) {
            SDL_FreeSurface(lm->levels[i].collisionMap);
            lm->levels[i].collisionMap = NULL;
        }
    }
}

void levelManagerDestroy(LevelManager *lm) {
    levelInternalDestroy(lm);
}

void levelSwitchTo(LevelManager *lm, SDL_Renderer *r, int levelId) {
    levelManagerLoad(lm, r, levelId);
}

int levelGetCurrentId(LevelManager *lm) {
    return lm->currentLevel;
}

int levelCheckPixelCollision(LevelManager *lm, int x, int y) { 
    (void)lm; (void)x; (void)y;
    return 0; 
}

void levelResolvePlayerCollision(LevelManager *lm, Player *player) {
    (void)lm; (void)player;
    player->onGround = 0;
}

void levelTriggerShake(LevelManager *lm, int intensity, int duration) {
    lm->shakeIntensity = intensity;
    lm->shakeTimer = duration;
}

int levelCheckLadder(LevelManager *lm, Player *player) {
    if (lm->currentLevel < 0) return 0;
    Level *lvl = &lm->levels[lm->currentLevel];
    
    // Debug
    static int lastDebug = 0;
    int now = SDL_GetTicks();
    
    SDL_Rect playerRect = {(int)player->x, (int)player->y, PLAYER_WIDTH, PLAYER_HEIGHT};
    
    for (int i = 0; i < lvl->ladderCount; i++) {
        if (!lvl->ladders[i].active) continue;
        
        SDL_Rect ladderRect = {lvl->ladders[i].x, lvl->ladders[i].y, 
                              lvl->ladders[i].w, lvl->ladders[i].h};
        
        // Debug affichage des positions
        if (now - lastDebug > 1000) {
            printf("Joueur: %d,%d Echelle%d: %d,%d %dx%d\n", 
                   (int)player->x, (int)player->y, i,
                   lvl->ladders[i].x, lvl->ladders[i].y,
                   lvl->ladders[i].w, lvl->ladders[i].h);
            lastDebug = now;
        }
        
        if (checkAABBCollision(playerRect, ladderRect)) {
            printf("ECHELLE %d TOUCHEE!\n", i);
            return 1;
        }
    }
    return 0;
}

int levelCheckPortal(LevelManager *lm, Player *player) {
    if (lm->currentLevel < 0 || lm->currentLevel == 2) return 0;
    Level *lvl = &lm->levels[lm->currentLevel];
    
    if (!lvl->portal.active || lvl->portal.triggered) return 0;
    
    SDL_Rect playerRect = {(int)player->x, (int)player->y, PLAYER_WIDTH, PLAYER_HEIGHT};
    SDL_Rect portalRect = {lvl->portal.x, lvl->portal.y, lvl->portal.w, lvl->portal.h};
    
    return checkAABBCollision(playerRect, portalRect);
}

void levelStartLevelUp(LevelManager *lm, SDL_Renderer *r) {
    if (lm->currentLevel < 0) return;
    lm->levels[lm->currentLevel].portal.triggered = 1;
    lm->showLevelUp = 1;
    lm->levelUpTimer = 300;
    lm->transitioning = 1;
    
    printf("=== LEVEL UP DEMARRE ===\n");
    
    levelManagerLoad(lm, r, 2);
}

void levelManagerUpdate(LevelManager *lm, Player *player, SDL_Renderer *r) {
    if (lm->currentLevel < 0) return;
    Level *lvl = &lm->levels[lm->currentLevel];
    
    // Gestion écran level up
    if (lm->showLevelUp) {
        lm->levelUpTimer--;
        
        if (lm->levelUpTimer <= 0) {
            printf("=== FIN LEVEL UP -> NIVEAU 2 ===\n");
            lm->showLevelUp = 0;
            lm->transitioning = 0;
            levelSwitchTo(lm, r, 1);
        }
        return;
    }
    
    // Camera follow niveau 2
    if (lm->currentLevel == 1) {
        int targetCamX = (int)player->x - GAME_WIDTH / 2;
        int targetCamY = (int)player->y - GAME_HEIGHT / 2;
        
        if (targetCamX < 0) targetCamX = 0;
        if (targetCamX > lvl->width - GAME_WIDTH) targetCamX = lvl->width - GAME_WIDTH;
        if (targetCamY < 0) targetCamY = 0;
        if (targetCamY > lvl->height - GAME_HEIGHT) targetCamY = lvl->height - GAME_HEIGHT;
        
        lm->cameraX += (targetCamX - lm->cameraX) * 0.1f;
        lm->cameraY += (targetCamY - lm->cameraY) * 0.1f;
    } else {
        lm->cameraX = 0;
        lm->cameraY = 0;
    }
    
    // Collision portail
    if (!lm->transitioning && lm->currentLevel != 2 && levelCheckPortal(lm, player)) {
        printf("PORTAIL TOUCHE! Lancement level up...\n");
        levelStartLevelUp(lm, r);
    }
    
    // Filet de sécurité niveau 1
    if (lvl->hasSafetyNet && player->y > lvl->height - 60) {
        player->y = lvl->height - 60 - PLAYER_HEIGHT;
        player->vy = -5;
        player->onGround = 1;
        printf("Filet de securite!\n");
    }
}
