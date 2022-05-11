// 2> /dev/null; GAMEROOT="`dirname $0`"; mkdir -p $GAMEROOT/build && for FILE in $GAMEROOT/shaders/*; do glslc $FILE -o $GAMEROOT/build/${FILE##*.}.spv; done && clang -o $GAMEROOT/build/game.exe -std=c99 -Wall -Wextra -Werror=vla -pedantic -Og -ggdb -march=native -ffast-math $0 game.c steam_api64.lib -lvulkan; exit

#define MAX(A, B) (A) > (B) ? (A) : (B)
#define MIN(A, B) (A) < (B) ? (A) : (B)
#define CLAMP(V, LOW, HIGH) MAX(LOW, MIN(HIGH, V))

#define INCLUDE_SRC
#include "game.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "fmath.h"
#include "steamworks.h"

#include "vk_renderer.h"

static struct Input input = {
    .keys = {KS_RELEASED},
    .keys_previous = {KS_RELEASED}
};
static struct Window window = {
    .width = 800,
    .height = 600,
    .running = true
};

internal LRESULT CALLBACK win32_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            // const u16 repeat_count = LOWORD(lparam);
            const b32 isdown = !(lparam >> 31);
            const enum KeyState state = isdown ? KS_PRESSED : KS_RELEASED;
            // const b32 wasdown = ((lparam >> 30) & 1);
            const b32 isaltdown = (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) && (lparam & (1 << 29));
            if (wparam >= '0' && wparam <= '9') {
                input.keys[K_0 + (wparam - '0')] = state;
            } else if (wparam >= VK_F1 && wparam <= VK_F12) {
                input.keys[K_F1 + (wparam - VK_F1)] = state;
            } else if (wparam >= 'A' && wparam <= 'Z') {
                input.keys[K_A + (wparam - 'A')] = state;
            } else if (wparam >= VK_LEFT && wparam <= VK_DOWN) {
                input.keys[K_LEFTARROW + (wparam - VK_LEFT)] = state;
            } else {
                switch (wparam) {
                    case VK_ESCAPE: input.keys[K_ESCAPE] = state; break;
                    case VK_OEM_3: input.keys[K_BACKTICK] = state; break;
                    case VK_BACK: input.keys[K_BACKSPACE] = state; break;
                    case VK_SPACE: input.keys[K_SPACE] = state; break;
                    case VK_TAB: input.keys[K_TAB] = state; break;
                    case VK_CAPITAL: input.keys[K_CAPSLOCK] = state; break;
                    case VK_RETURN: input.keys[K_ENTER] = state; break;
                    case VK_CONTROL: input.keys[K_CONTROL] = state; break;
                    case VK_SHIFT: input.keys[K_SHIFT] = state; break;
                    case VK_MENU: input.keys[K_ALT] = state; break;
                }
            }

            // handle alt-f4 outside of game code (for now?)
            if (wparam == VK_F4 && isdown && isaltdown) {
                PostQuitMessage(0);
            }
        } break;
        case WM_SIZE: {
            const int w = LOWORD(lparam);
            const int h = HIWORD(lparam);
            window.width = MAX(w, 0);
            window.height = MAX(h, 0);
        } break;
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        default: return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, PSTR cmdline, INT show)
#pragma GCC diagnostic pop
{
    RegisterClassA(&(WNDCLASSA) {
        .style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW,
        .lpfnWndProc = win32_window_callback,
        .hInstance = inst,
        .lpszClassName = "GameWindowClass"
    });
    const HWND hwnd = CreateWindowExA(
        0,
        "GameWindowClass",
        "Window Title",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, window.width, window.height,
        NULL,
        NULL,
        inst,
        NULL
    );

    if (!SteamAPI_Init()) {
        LOGWARN("Steam could not be loaded");
    }

    if (!renderer_init(&(struct WindowToRendererInfo) {
        .inst = inst,
        .hwnd = hwnd
    })) {
        PostQuitMessage(1);
    }

    while (true) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                window.running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        // have the running check before drawing a frame instead of at the beginning of the loop
        if (!window.running) {
            break;
        }

        game_update(&input, &window);
        renderer_update();

        for (usize i = 0; i < K_LAST; ++i) {
            input.keys_previous[i] = input.keys[i];
        }
    }

    SteamAPI_Shutdown();
    renderer_deinit();
    return EXIT_SUCCESS;
}
