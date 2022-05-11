#include "game.h"

GAME_UPDATE(game_update)
{
    if (input->keys[K_ESCAPE]) {
        window->running = false;
    }
}
