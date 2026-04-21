#include "game_state.h"

void initStateManager(StateManager *sm) {
    sm->current = STATE_MAIN_MENU;
    sm->previous = STATE_MAIN_MENU;
}

void changeState(StateManager *sm, GameState newState) {
    sm->previous = sm->current;
    sm->current = newState;
}

void goBack(StateManager *sm) {
    GameState temp = sm->current;
    sm->current = sm->previous;
    sm->previous = temp;
}
