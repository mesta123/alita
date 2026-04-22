#include "gameplay_state.h"
#include <stdio.h>

void gpInit(GameplayState *gp, SDL_Renderer *r) {
    gp->isPaused = 0;
    gp->showGameOver = 0;
    gp->gameOverTimer = 0;
    gp->sfx_jump = NULL;
    gp->renderer = r;
    
    levelManagerInit(&gp->levelManager, r);
    platformManagerInit(&gp->platformManager, r);
    
    gpSetupLevel(gp, 0);
    
    playerInit(&gp->player, r, 30, 268);
    playerForceGround(&gp->player, 300);
    
    minimapInit(&gp->minimap, r, &gp->levelManager);
    smInit(&gp->saveMenu, r);
}

void gpHandleEvent(GameplayState *gp, SDL_Window *w, SDL_Event *e, StateManager *sm) {
    if (gp->levelManager.showLevelUp) {
        if (e->type == SDL_KEYDOWN || e->type == SDL_MOUSEBUTTONDOWN) {
            gp->levelManager.showLevelUp = 0;
            gp->levelManager.transitioning = 0;
            levelSwitchTo(&gp->levelManager, gp->renderer, 1);
            gpSetupLevel(gp, 1);
        }
        return;
    }
    
    if (gp->showGameOver) {
        if (e->type == SDL_KEYDOWN || e->type == SDL_MOUSEBUTTONDOWN) {
            gp->showGameOver = 0;
            if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_ESCAPE) {
                changeState(sm, STATE_MAIN_MENU);
            }
        }
        return;
    }
    
    if (gp->isPaused) {
        smHandleEvent(&gp->saveMenu, w, e, &gp->player, &gp->levelManager, &gp->platformManager);
        return;
    }
    
    if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym == SDLK_ESCAPE) gp->isPaused = 1;
        else if (e->key.keysym.sym == SDLK_m) minimapToggle(&gp->minimap);
    }
}

void gpUpdate(GameplayState *gp) {
    if (gp->levelManager.showLevelUp) {
        levelManagerUpdate(&gp->levelManager, &gp->player, gp->renderer);
        return;
    }
    
    if (gp->isPaused || gp->showGameOver) return;
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    Level *lvl = &gp->levelManager.levels[gp->levelManager.currentLevel];
    
    int onLadder = levelCheckLadder(&gp->levelManager, &gp->player);
    
    playerUpdate(&gp->player, keys, lvl->width, lvl->height, onLadder);
    
    if (playerIsDead(&gp->player)) {
        gp->showGameOver = 1;
        gp->gameOverTimer = 120;
        return;
    }
    
    platformManagerUpdate(&gp->platformManager);
    platformManagerCheckCollisions(&gp->platformManager, &gp->player, &gp->levelManager);
    levelManagerUpdate(&gp->levelManager, &gp->player, gp->renderer);
    minimapUpdate(&gp->minimap, gp->renderer, &gp->levelManager, &gp->player, &gp->platformManager);
    
    if (gp->levelManager.currentLevel == 1 && gp->platformManager.count == 0) {
        gpSetupLevel(gp, 1);
    }
}

void gpRender(GameplayState *gp, SDL_Renderer *r, SDL_Window *w) {
    if (!r) return;
    
    int camX = gp->levelManager.cameraX;
    int camY = gp->levelManager.cameraY;
    
    if (gp->levelManager.showLevelUp || gp->levelManager.currentLevel == 2) {
        levelManagerRender(&gp->levelManager, r);
        
        Uint32 ticks = SDL_GetTicks();
        if ((ticks / 200) % 2 == 0) {
            SDL_Rect textBg = {GAME_WIDTH/2 - 80, GAME_HEIGHT - 60, 160, 30};
            SDL_SetRenderDrawColor(r, 0, 0, 0, 200);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
            SDL_RenderFillRect(r, &textBg);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }
        return;
    }
    
    levelManagerRender(&gp->levelManager, r);
    
    platformManagerRender(&gp->platformManager, r, camX, camY);
    
    playerDraw(&gp->player, r, camX, camY);
    
    if (!gp->isPaused) {
        minimapRender(&gp->minimap, r, &gp->levelManager, &gp->player, &gp->platformManager);
    }
    
    if (gp->isPaused) {
        smRender(&gp->saveMenu, r, w);
    }
    
    if (gp->showGameOver) {
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 200);
        SDL_Rect overlay = {0, 0, GAME_WIDTH, GAME_HEIGHT};
        SDL_RenderFillRect(r, &overlay);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        
        SDL_Rect box = {GAME_WIDTH/2 - 150, GAME_HEIGHT/2 - 80, 300, 160};
        SDL_SetRenderDrawColor(r, 50, 0, 0, 255);
        SDL_RenderFillRect(r, &box);
        SDL_SetRenderDrawColor(r, 255, 0, 0, 255);
        SDL_RenderDrawRect(r, &box);
        
        for (int i = 1; i <= 3; i++) {
            SDL_Rect border = {box.x - i, box.y - i, box.w + 2*i, box.h + 2*i};
            SDL_SetRenderDrawColor(r, 255, 50, 50, 255);
            SDL_RenderDrawRect(r, &border);
        }
        
        SDL_Rect g1 = {box.x + 50, box.y + 30, 20, 40};
        SDL_Rect g2 = {box.x + 50, box.y + 30, 30, 10};
        SDL_Rect g3 = {box.x + 50, box.y + 60, 30, 10};
        SDL_Rect g4 = {box.x + 70, box.y + 50, 10, 10};
        
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, &g1);
        SDL_RenderFillRect(r, &g2);
        SDL_RenderFillRect(r, &g3);
        SDL_RenderFillRect(r, &g4);
        
        SDL_Rect msgBg = {box.x + 20, box.y + 100, 260, 30};
        SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
        SDL_RenderFillRect(r, &msgBg);
    }
}

void gpDestroy(GameplayState *gp) {
    playerDestroy(&gp->player);
    levelManagerDestroy(&gp->levelManager);
    platformManagerDestroy(&gp->platformManager);
    minimapDestroy(&gp->minimap);
    smDestroy(&gp->saveMenu);
    if (gp->sfx_jump) Mix_FreeChunk(gp->sfx_jump);
}

void gpSetupLevel(GameplayState *gp, int levelId) {
    if (gp->levelManager.showLevelUp) return;
    
    levelManagerLoad(&gp->levelManager, gp->renderer, levelId);
    
    gp->platformManager.count = 0;
    gp->showGameOver = 0;
    gp->gameOverTimer = 0;
    
    if (levelId == 0) {
        // NIVEAU 1 - Collisions horizontales activées
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 0, 300, 650, 50, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 80, 240, 40, 60, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 85, 180, 30, 30, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 180, 200, 80, 20, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 320, 220, 50, 80, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_H, 280, 150, 40, 10, 280, 400, 0.02f);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 420, 180, 20, 20, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 450, 160, 20, 20, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 480, 140, 20, 20, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 550, 240, 40, 60, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 600, 200, 50, 20, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 140, 260, 40, 10, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 520, 120, 40, 10, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 500, 80, 60, 20, 0, 0, 0);
        
        playerSetPosition(&gp->player, 30, 268);
        playerForceGround(&gp->player, 300);
        
    } else if (levelId == 1) {
        // NIVEAU 2 - Pas de collisions horizontales (comportement original)
        
        // === PLATEFORMES VERTES (STATIQUES) - Points de repère sûrs ===
        
        // Zone de départ verte
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 0, 550, 250, 50, 0, 0, 0);
        
        // Plateforme verte intermédiaire gauche
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 300, 480, 100, 25, 0, 0, 0);
        
        // Plateforme verte centrale - point central important
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 500, 400, 150, 30, 0, 0, 0);
        
        // Plateforme verte escalier vers le haut
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 700, 350, 80, 25, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 850, 300, 80, 25, 0, 0, 0);
        
        // Plateforme verte haute centrale
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 600, 250, 120, 30, 0, 0, 0);
        
        // Section échelle gauche - tout en VERT
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 100, 450, 120, 40, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 80, 350, 100, 30, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 120, 250, 90, 30, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 100, 150, 110, 40, 0, 0, 0);
        
        // Section échelle droite - tout en VERT
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 1200, 450, 150, 40, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 1250, 350, 130, 30, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 1300, 250, 120, 30, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 1350, 150, 140, 40, 0, 0, 0);
        
        // Plateformes vertes de secours en bas
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 450, 600, 80, 25, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 650, 650, 80, 25, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 900, 600, 100, 30, 0, 0, 0);
        
        // Plateforme verte finale avant portail
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 1450, 200, 100, 40, 0, 0, 0);
        platformManagerAdd(&gp->platformManager, PLATFORM_STATIC, 1500, 120, 80, 30, 0, 0, 0);
        
        // === OBSTACLES BLEUS (MOBILES LENTS) - Difficulté ===
        
        // Obstacles bleus horizontaux lents - section début
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_H, 250, 520, 60, 15, 220, 320, 0.008f);
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_H, 400, 450, 50, 15, 350, 480, 0.006f);
        
        // Obstacles bleus verticaux lents - section milieu
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_V, 550, 380, 50, 15, 320, 450, 0.005f);
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_V, 750, 320, 50, 15, 270, 400, 0.007f);
        
        // Obstacles bleus oscillants - section difficile
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_H, 950, 280, 60, 15, 900, 1050, 0.004f);
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_V, 1050, 220, 50, 15, 180, 320, 0.006f);
        
        // Obstacles bleus finaux lents
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_H, 1150, 180, 60, 15, 1100, 1250, 0.005f);
        platformManagerAdd(&gp->platformManager, PLATFORM_MOVING_V, 1400, 350, 50, 15, 300, 450, 0.008f);
        
        playerSetPosition(&gp->player, 50, 520);
        playerForceGround(&gp->player, 550);
    }
    
    gp->levelManager.cameraX = 0;
    gp->levelManager.cameraY = 0;
    
    minimapSetLevel(&gp->minimap, &gp->levelManager);
    
    printf("Niveau %d charge: %d plateformes\n", levelId, gp->platformManager.count);
}
