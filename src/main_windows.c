#define X(RET, NAME, ...) RET NAME(__VA_ARGS__);
KERNEL32_FUNCTIONS
#undef X
#define X(RET, NAME, ...) RET (*NAME)(__VA_ARGS__);
// required
USER32_FUNCTIONS
#if RENDERER_OPENGL
GDI32_FUNCTIONS
OPENGL32_FUNCTIONS
GL10_FUNCTIONS
#endif
// optional
DWMAPI_FUNCTIONS
WINMM_FUNCTIONS
#undef X

void (*windows_renderer_init)(void);
void (*windows_renderer_deinit)(void);
void (*windows_renderer_resize)(void);
void (*windows_renderer_present)(void);

void windows_select_renderer(void) {
#if RENDERER_OPENGL
    windows_renderer_init = opengl_init;
    windows_renderer_deinit = opengl_deinit;
    windows_renderer_resize = opengl_resize;
    windows_renderer_present = opengl_present;
    return;
#endif
}

void windows_update_cursor_clip(void) {

}

void windows_toggle_fullscreen(void) {
    static WINDOWPLACEMENT save_placement = {size_of(WINDOWPLACEMENT)};

    u32 style = cast(u32) GetWindowLongPtrW(windows_hwnd, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO mi = {size_of(MONITORINFO)};
        GetMonitorInfoW(MonitorFromWindow(windows_hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

        GetWindowPlacement(windows_hwnd, &save_placement);
        SetWindowLongPtrW(windows_hwnd, GWL_STYLE, style & ~cast(u32) WS_OVERLAPPEDWINDOW);
        SetWindowPos(windows_hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED);
    } else {
        SetWindowLongPtrW(windows_hwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(windows_hwnd, &save_placement);
        SetWindowPos(windows_hwnd, null, 0, 0, 0, 0, SWP_NOSIZE |
            SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

s64 windows_window_proc(HWND hwnd, u32 message, u64 wParam, s64 lParam) {
    switch (message) {
        case WM_PAINT: {
            ValidateRect(hwnd, null);
            return 0;
        }
        case WM_ERASEBKGND: {
            return 1;
        }
        case WM_ACTIVATEAPP: {
            if (wParam != 0) windows_update_cursor_clip();
            return 0;
        }
        case WM_SIZE: {
            platform_screen_width = cast(u16) cast(u64) lParam;
            platform_screen_height = cast(u16) (cast(u64) lParam >> 16);

            windows_renderer_resize();
            return 0;
        }
        case WM_CREATE: {
            windows_hwnd = hwnd;
            windows_hdc = GetDC(hwnd);

            if (DwmSetWindowAttribute) {
                s32 dark_mode = true;
                DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark_mode, size_of(dark_mode));
                s32 round_mode = DWMWCP_DONOTROUND;
                DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &round_mode, size_of(round_mode));
            }

            windows_renderer_init();
            return 0;
        }
        case WM_DESTROY: {
            windows_renderer_deinit();

            PostQuitMessage(0);
            return 0;
        }
        case WM_SYSCOMMAND: {
            if (wParam == SC_KEYMENU) return 0;
            // fallthrough
        }
        default: {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }
}

void WinMainCRTStartup(void) {
    {
        HMODULE lib;
#define X(RET, NAME, ...) NAME = (RET (*)(__VA_ARGS__)) GetProcAddress(lib, #NAME);
        lib = LoadLibraryA("USER32");
        USER32_FUNCTIONS
#if RENDERER_OPENGL
        lib = LoadLibraryA("GDI32");
        GDI32_FUNCTIONS
        lib = LoadLibraryA("OPENGL32");
        OPENGL32_FUNCTIONS
        GL10_FUNCTIONS
#endif
        lib = LoadLibraryA("DWMAPI");
        DWMAPI_FUNCTIONS
        lib = LoadLibraryA("WINMM");
        WINMM_FUNCTIONS
#undef X
    }

    windows_hinstance = GetModuleHandleW(null);

    bool sleep_is_granular = timeBeginPeriod && timeBeginPeriod(1) == 0;

    windows_select_renderer();

    SetProcessDPIAware();
    WNDCLASSEXW wndclass = {0};
    wndclass.cbSize = size_of(WNDCLASSEXW);
    wndclass.style = CS_OWNDC;
    wndclass.lpfnWndProc = windows_window_proc;
    wndclass.hInstance = windows_hinstance;
    wndclass.hIcon = LoadIconW(null, IDI_WARNING);
    wndclass.hCursor = LoadCursorW(null, IDC_CROSS);
    wndclass.lpszClassName = L"A";
    RegisterClassExW(&wndclass);
    CreateWindowExW(0, wndclass.lpszClassName, L"game",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        null, null, windows_hinstance, null);

    for (;;) {
        MSG msg;
        while (PeekMessageW(&msg, null, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            switch (msg.message) {
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP: {
                    bool pressed = (msg.lParam & 1u << 31) == 0;
                    bool repeat = pressed && (msg.lParam & 1 << 30) != 0;
                    bool sys = msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP;
                    bool alt = sys && (msg.lParam & 1 << 29) != 0;

                    if (!repeat && (!sys || alt || msg.wParam == VK_F10)) {
                        if (pressed) {
                            if (msg.wParam == VK_F4 && alt) DestroyWindow(windows_hwnd);
                            if (msg.wParam == VK_F11 || (msg.wParam == VK_RETURN && alt)) windows_toggle_fullscreen();
                            if (DEVELOPER && msg.wParam == VK_ESCAPE) DestroyWindow(windows_hwnd);
                        }
                    }
                    break;
                }
                case WM_QUIT: {
                    goto game_loop_end;
                }
                default: {
                    DispatchMessageW(&msg);
                }
            }
        }

        windows_renderer_present();

        if (sleep_is_granular) {
            Sleep(1);
        }
    }
game_loop_end:

    ExitProcess(0);
}