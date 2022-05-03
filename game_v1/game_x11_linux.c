//usr/bin/clang -o game -std=c99 -Wall -Wextra -pedantic -Wno-unused-parameter -Og -ggdb $0 -lm -ldl -lX11 -lXext; exit

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 199309L

#define INCLUDE_SRC
#include "game.h"
#include "fmath.h"
#include "keys.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include <linux/joystick.h>

#define MAX_WIDTH 3840
#define MAX_HEIGHT 2160

#define KB(B) (B * 1024LL)
#define MB(B) (KB(B) * 1024LL)
#define GB(B) (MB(B) * 1024LL)

#define ASSERT(EXPR, ...) if (!(EXPR)) die(__VA_ARGS__)

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputs("ERROR: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    exit(EXIT_FAILURE);
}

static bool trans(enum KeyState *keys, bool check, enum Keys key, enum KeyState state)
{
    if (check) {
        keys[key] = state;
    }
    return check;
}

static bool check_keys(enum KeyState *keys, KeySym sym, enum KeyState state)
{
    return
        trans(keys, sym == XK_Escape, K_ESCAPE, state) ||
        trans(keys, sym == XK_space, K_SPACE, state) ||
        trans(keys, sym == XK_Tab, K_TAB, state) ||
        trans(keys, sym == XK_grave, K_BACKTICK, state) ||
        trans(keys, sym == XK_BackSpace, K_BACKSPACE, state) ||
        trans(keys, sym == XK_Return, K_ENTER, state) ||
        trans(keys, sym >= XK_a && sym <= XK_z, K_A + (sym - XK_a), state) ||
        trans(keys, sym >= XK_0 && sym <= XK_9, K_0 + (sym - XK_0), state) ||
        trans(keys, sym >= XK_F1 && sym <= XK_F12, K_F1 + (sym - XK_F1), state);
}

static clockid_t CLOCKTYPE = CLOCK_REALTIME;
static uint64_t gettime(void)
{
    struct timespec ts;
    clock_gettime(CLOCKTYPE, &ts);
    return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

static double getelapsedtime(uint64_t since)
{
    return (double) (gettime() - since) / 1000000000ULL;
}

static time_t get_file_modified_time(const char *path)
{
    struct stat attr;
    stat(path, &attr);
    return attr.st_mtim.tv_sec;
}

struct LiveGameCode {
    void *DLL;
    game_update_and_render *guar;
    time_t last_write_time;
};

static void unload_game_code(struct LiveGameCode *game)
{
    if (game->DLL) {
        dlclose(game->DLL);
    }
}

static struct LiveGameCode load_game_code(void)
{
    struct LiveGameCode lgc;
    lgc.guar = GameUpdateAndRenderStub;
    if ((lgc.DLL = dlopen("./libgame.so", RTLD_LAZY|RTLD_GLOBAL))) {
        lgc.guar = (game_update_and_render *) dlsym(lgc.DLL, "GameUpdateAndRender");
    }
    lgc.last_write_time = get_file_modified_time("./libgame.so.done");
    return lgc;
}

int main(void)
{
    struct LiveGameCode game = load_game_code();

    int js0 = open("/dev/input/js0", O_RDONLY|O_NONBLOCK);
    if (js0 != -1) {
        char name[128];
        if (ioctl(js0, JSIOCGNAME(sizeof(name)), name) < 0)
            strncpy(name, "Unknown", sizeof(name));
        printf("Controller 0: %s\n", name);
    }

    struct GameMemory mem = {0};
    mem.initialized = false;
    mem.permanent_storage_size = MB(64);
    mem.permanant_storage = calloc(mem.permanent_storage_size, 1);
    ASSERT(mem.permanant_storage != NULL, "Failed to allocate game memory.");

    enum KeyState keys[K_LAST] = {KS_RELEASED};
    enum KeyState keys_previous[K_LAST] = {KS_RELEASED};

    Display *dpy = XOpenDisplay(NULL);
    ASSERT(dpy, "cannot open display");

    XKeyboardState kbstate;
    XGetKeyboardControl(dpy, &kbstate);
    XKeyboardControl kbctrl;
    kbctrl.auto_repeat_mode = AutoRepeatModeOff;
    XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kbctrl);

    ASSERT(XShmQueryExtension(dpy) == True, "XShm extension missing");

    int scr = DefaultScreen(dpy);

    int WIDTH = 600, HEIGHT = 400;
    Window window = XCreateSimpleWindow(
        dpy,
        RootWindow(dpy, scr),
        0, 0,
        WIDTH, HEIGHT,
        0, WhitePixel(dpy, scr),
        BlackPixel(dpy, scr)
    );
    Atom WMDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, window, &WMDeleteWindow, 1);
    XSelectInput(dpy, window, KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|FocusChangeMask|ExposureMask);
    XStoreName(dpy, window, "Title");
    XMapRaised(dpy, window);
    XSync(dpy, False);

    GC gc = XCreateGC(dpy, window, 0, NULL);

    XShmSegmentInfo shminfo;
    XImage *image = XShmCreateImage(dpy, DefaultVisual(dpy, scr), DefaultDepth(dpy, scr), ZPixmap, NULL, &shminfo, MAX_WIDTH, MAX_HEIGHT);
    shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * MAX_HEIGHT, IPC_CREAT|0777);
    shminfo.shmaddr = image->data = shmat(shminfo.shmid, 0, 0);
    shminfo.readOnly = False;
    XShmAttach(dpy, &shminfo);

    XShmPutImage(dpy, window, gc, image, 0, 0, 0, 0, WIDTH, HEIGHT, True);

    int ShmCompletionType = XShmGetEventBase(dpy) + ShmCompletion;

#ifdef _POSIX_MONOTONIC_CLOCK
        struct timespec monotonic_test;
        if (clock_gettime(CLOCK_MONOTONIC, &monotonic_test) == 0) {
            CLOCKTYPE = CLOCK_MONOTONIC;
        }
#endif

    double start_time = gettime();
    double last_update_time = 0.0;

    struct Mouse mouse = {
        .x = 300,
        .y = 200
    };

    bool RUNNING = true;
    while (RUNNING) {
        time_t new_write_time = get_file_modified_time("./libgame.so.done");
        if (new_write_time != game.last_write_time) {
            unload_game_code(&game);
            game = load_game_code();
        }

        while (XPending(dpy) > 0) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            switch (ev.type) {
                case KeyPress: {
                    KeySym sym = XLookupKeysym(&ev.xkey, 0);
                    check_keys(keys, sym, KS_PRESSED);
                } break;
                case KeyRelease: {
                    KeySym sym = XLookupKeysym(&ev.xkey, 0);
                    check_keys(keys, sym, KS_RELEASED);
                } break;
                case ButtonPress: {
                } break;
                case ButtonRelease: {
                } break;
                case MotionNotify: {
                    mouse.x = ev.xmotion.x;
                    mouse.y = ev.xmotion.y;
                } break;
                case FocusOut: {
                    for (size_t i = 0; i < K_LAST; ++i) {
                        keys[i] = KS_RELEASED;
                    }
                } break;
                case Expose: {
                    WIDTH = ev.xexpose.width;
                    HEIGHT = ev.xexpose.height;
                } break;
                case MappingNotify: {
                    XRefreshKeyboardMapping(&ev.xmapping);
                } break;
                case ClientMessage: {
                    if (ev.xclient.data.l[0] == (long) WMDeleteWindow) {
                        RUNNING = false;
                    }
                } break;
                case DestroyNotify: {
                    RUNNING = false;
                } break;
                default: {
                    if (ev.type == ShmCompletionType) {
                        double time = getelapsedtime(start_time);
                        float delta = time - last_update_time;
                        last_update_time = time;

                        ASSERT(WIDTH <= MAX_WIDTH && HEIGHT <= MAX_HEIGHT, "Window too large");
                        struct Colorbuffer cb = {
                            .width = WIDTH,
                            .height = HEIGHT,
                            .pitch = MAX_WIDTH,
                            .data = (uint32_t *) image->data,
                            .rgba_offsets = {16, 8, 0, 24}
                        };
                        cb.width = cb.width < MAX_WIDTH ? cb.width : MAX_WIDTH;
                        cb.height = cb.height < MAX_HEIGHT ? cb.height : MAX_HEIGHT;
                        game.guar(delta, &cb, &mem, keys, keys_previous, &mouse, &RUNNING);
                        for (size_t i = 0; i < K_LAST; ++i) {
                            keys_previous[i] = keys[i];
                        }
                        XShmPutImage(dpy, window, gc, image, 0, 0, 0, 0, WIDTH, HEIGHT, True);
                    }
                } break;
            }
        }

        if (js0 != -1) {
            struct js_event e;
            while (read(js0, &e, sizeof(e)) > 0) {
                if (e.type & JS_EVENT_INIT)
                    continue;
                printf("%d - %d\n", e.number, e.value);
            }
        }
    }

    kbctrl.auto_repeat_mode = kbstate.global_auto_repeat;
    XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kbctrl);
    XSync(dpy, True);
    return EXIT_SUCCESS;
}
