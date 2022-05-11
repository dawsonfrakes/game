#ifndef GAME_H
#define GAME_H

#include "gametypes.h"
#include "input.h"

struct Input {
    enum KeyState keys[K_LAST];
    enum KeyState keys_previous[K_LAST];
};

struct Window {
    u32 width, height;
    b32 running;
};

#define GAME_UPDATE(NAME) void NAME(const struct Input *input, struct Window *window)
GAME_UPDATE(game_update);

#endif /* GAME_H */
