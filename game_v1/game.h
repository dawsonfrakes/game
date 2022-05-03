#ifndef GAME_H
#define GAME_H

#include "keys.h"
#include "fmath.h"

#include <stdint.h>
#include <stdbool.h>

#define JUST_PRESSED(KEY) (keys[KEY] == KS_PRESSED && keys_previous[KEY] == KS_RELEASED)
#define JUST_RELEASED(KEY) (keys[KEY] == KS_RELEASED && keys_previous[KEY] == KS_PRESSED)
#define RGB(R, G, B) ((R << cb->rgba_offsets[0]) | (G << cb->rgba_offsets[1]) | (B << cb->rgba_offsets[2]) | (0xFF << cb->rgba_offsets[3]))

struct Colorbuffer {
    uint32_t width, height, pitch;
    uint32_t *data;
    uint32_t rgba_offsets[4];
};

struct GameMemory {
    bool initialized;
    uint64_t permanent_storage_size;
    void *permanant_storage;
};

struct Mouse {
    int x, y;
};

#define GAME_UPDATE_AND_RENDER(NAME) void NAME(float delta, struct Colorbuffer *cb, struct GameMemory *mem, enum KeyState *keys, enum KeyState *keys_previous, struct Mouse *mouse, bool *running)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub){}

#endif /* GAME_H */
