#ifndef GAME_STATE_H
#define GAME_STATE_H

typedef enum {
    STATE_MAIN_MENU,
    STATE_OPTIONS,
    STATE_GAMEPLAY,
    STATE_EXIT
} GameState;

typedef struct {
    GameState current;
    GameState previous;
} StateManager;

void initStateManager(StateManager *sm);
void changeState(StateManager *sm, GameState newState);
void goBack(StateManager *sm);

#endif
