//usr/bin/clang -o libgame.so -fPIC -shared $0 && touch libgame.so.done; exit

#define INCLUDE_SRC
#include "game.h"

static void draw_pixel(struct Colorbuffer *cb, uint32_t X, uint32_t Y, uint32_t color)
{
    if (X >= 0 && X < cb->width && Y >= 0 && Y <= cb->height) {
        cb->data[Y * cb->pitch + X] = color;
    }
}

static void draw_rect(struct Colorbuffer *cb, int64_t X, int64_t Y, uint32_t W, uint32_t H, uint32_t color)
{
    for (int64_t y = Y; y < Y + H; ++y) {
        for (int64_t x = X; x < X + W; ++x) {
            draw_pixel(cb, x, y, color);
        }
    }
}

static void draw_caseys_pattern(struct Colorbuffer *cb, float xoff)
{
    for (uint32_t y = 0; y < cb->height; ++y) {
        for (uint32_t x = 0; x < cb->width; ++x) {
            draw_pixel(cb, x, y, RGB(0, y, (uint32_t)(x+xoff)));
        }
    }
}

static void draw_line(struct Colorbuffer *cb, V2 a, V2 b, uint32_t color)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float step = fabsf(dx) >= fabsf(dy) ? fabsf(dx) : fabsf(dy);
    dx /= step;
    dy /= step;
    float x = a.x;
    float y = a.y;
    for (uint32_t i = 0; i < step; ++i) {
        draw_pixel(cb, x, y, color);
        x += dx;
        y += dy;
    }
}

static void draw_triangle(struct Colorbuffer *cb, V2 a, V2 b, V2 c, uint32_t color)
{
    draw_line(cb, a, b, color);
    draw_line(cb, b, c, color);
    draw_line(cb, c, a, color);
}

static void draw_circle(struct Colorbuffer *cb, int xc, int yc, int x, int y, uint32_t color)
{
    draw_pixel(cb, xc+x, yc+y, color);
    draw_pixel(cb, xc-x, yc+y, color);
    draw_pixel(cb, xc+x, yc-y, color);
    draw_pixel(cb, xc-x, yc-y, color);
    draw_pixel(cb, xc+y, yc+x, color);
    draw_pixel(cb, xc-y, yc+x, color);
    draw_pixel(cb, xc+y, yc-x, color);
    draw_pixel(cb, xc-y, yc-x, color);
}

static void circle_bresenham(struct Colorbuffer *cb, int xc, int yc, int r, uint32_t color)
{
    int x = 0, y = r;
    int d = 3 - 2 * r;
    draw_circle(cb, xc, yc, x, y, color);
    while (y >= x++)
    {
        if (d > 0)
            d = d + 4 * (x - (--y)) + 10;
        else
            d = d + 4 * x + 6;
        draw_circle(cb, xc, yc, x, y, color);
    }
}

struct GameData {
    V2 playerpos;
};

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    const float playerradius = 50.0f;
    struct GameData *gamedata = (struct GameData *) mem->permanant_storage;
    if (!mem->initialized) {
        gamedata->playerpos = v2(playerradius, playerradius);
        mem->initialized = true;
    }
    if (keys[K_Q] == KS_PRESSED) {
        *running = false;
    }
    V2 direction = {0, 0};
    if (keys[K_W]) {
        direction.y -= 1;
    }
    if (keys[K_A]) {
        direction.x -= 1;
    }
    if (keys[K_S]) {
        direction.y += 1;
    }
    if (keys[K_D]) {
        direction.x += 1;
    }
    gamedata->playerpos = v2add(gamedata->playerpos, v2mul(v2nrm(direction), 500.0f * delta));

    draw_rect(cb, 0, 0, cb->width, cb->height, RGB(0, 0, 0)); // background
    // draw_caseys_pattern(cb, gamedata->xoffset);
    circle_bresenham(cb, gamedata->playerpos.x, gamedata->playerpos.y, playerradius, RGB(255, 0, 255));
}
