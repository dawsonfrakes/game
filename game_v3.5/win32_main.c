#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "types.h"

bool renderer_init(struct RendererWindowData *window);
void renderer_update(void);
void renderer_deinit(void);

internal LRESULT CALLBACK win32_window_callback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            // const u16 repeat_count = LOWORD(lparam);
            const b32 isdown = !(lparam >> 31);
            // const b32 wasdown = ((lparam >> 30) & 1);
            const b32 isaltdown = (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) && (lparam & (1 << 29));
            switch (wparam) {
                case VK_ESCAPE: {
                    if (isdown)
                        PostQuitMessage(0);
                } break;
                case VK_F4: {
                    if (isdown && isaltdown)
                        PostQuitMessage(0);
                } break;
            }
        } break;
        case WM_SIZE: {
            // int w = LOWORD(lparam);
            // int h = HIWORD(lparam);
        } break;
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        default: return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
    return 0;
}

internal HWND win32_create_window(HINSTANCE inst)
{
    const char *class = "GameWindowClass";
    RegisterClassA(&(WNDCLASSA) {
        .style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW,
        .lpfnWndProc = win32_window_callback,
        .hInstance = inst,
        .lpszClassName = "GameWindowClass"
    });
    return CreateWindowExA(
        0,
        class,
        "Window Title",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        inst,
        NULL
    );
}

internal bool win32_process_messages(void)
{
    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return true;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, PSTR cmdline, INT show)
#pragma GCC diagnostic pop
{
    HWND hwnd = win32_create_window(inst);
    renderer_init(&(struct RendererWindowData) {
        .inst = inst,
        .hwnd = hwnd
    });
    while (win32_process_messages()) {
        renderer_update();
    }

    renderer_deinit();
    return 0;
}
