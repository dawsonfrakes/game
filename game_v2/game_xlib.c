//usr/bin/clang -o game -std=c99 -Wall -Wextra -pedantic -Og -ggdb $0 -lm -lGL -lX11; exit

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 199309L

#include "game.c"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>

#include <GL/glx.h>

#define GL_FUNCTIONS \
    /*  ret, name, params */ \
    GL_EXPAND(GLXContext, XCreateContextAttribsARB, Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list)

#define GL_EXPAND(RET, NAME, ...) static RET (*gl##NAME)(__VA_ARGS__);
GL_FUNCTIONS
#undef GL_EXPAND

static void load_extensions(void)
{
#define GL_EXPAND(RET, NAME, ...) gl##NAME = (RET (*)(__VA_ARGS__)) glXGetProcAddressARB((const GLubyte *) "gl"#NAME);
    GL_FUNCTIONS
#undef GL_EXPAND
#define GL_GAME_EXPAND(RET, NAME, ...) GL.NAME = (RET (*)(__VA_ARGS__)) glXGetProcAddressARB((const GLubyte *) "gl"#NAME);
    GL_GAME_FUNCTIONS
#undef GL_GAME_EXPAND
}

static bool is_ext_supported(const char *ext_list, const char *extension) {
    const char *where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return false;
    const char *start = ext_list;
    for (;;) {
        where = strstr(start, extension);

        if (!where) {
            break;
        }

        const char *terminator = where + strlen(extension);

        if ( where == start || *(where - 1) == ' ' ) {
            if ( *terminator == ' ' || *terminator == '\0' ) {
                return true;
            }
        }

        start = terminator;
    }
    return false;
}

static bool load_opengl(Display *dpy, int screen, GLXContext *out_ctx, XVisualInfo **out_visual)
{
    const GLint glx_attributes[] = {
        GLX_X_RENDERABLE, True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_DOUBLEBUFFER, True,
        None
    };
    const int context_attribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
    };

    int maj = 0, min = 0;
    glXQueryVersion(dpy, &maj, &min);
    if (maj <= 1 && min < 2) {
        return false;
    }
    printf("GLX Version %d.%d\n", maj, min);

    int fbcount;
    GLXFBConfig *configs = glXChooseFBConfig(dpy, screen, glx_attributes, &fbcount);
    if (!configs) {
        return false;
    }
    printf("Found %d possible framebuffers\n", fbcount);

    int best_fbc = -1, best_num_samp = -1;
    for (int i = 0; i < fbcount; ++i) {
        XVisualInfo *vi = glXGetVisualFromFBConfig(dpy, configs[i]);
        if (vi) {
            int samp_buf, samples;
            glXGetFBConfigAttrib(dpy, configs[i], GLX_SAMPLE_BUFFERS, &samp_buf);
            glXGetFBConfigAttrib(dpy, configs[i], GLX_SAMPLES, &samples);
            if (best_fbc < 0 || (samp_buf && samples > best_num_samp)) {
                best_fbc = i;
                best_num_samp = samples;
            }
            XFree(vi);
        }
    }
    printf("Selecting framebuffer config %d\n", best_fbc);

    *out_visual = glXGetVisualFromFBConfig(dpy, configs[best_fbc]);
    if (*out_visual == 0) {
        return false;
    }

    if (screen != (*out_visual)->screen) {
        return false;
    }

    const char *glx_extensions = glXQueryExtensionsString(dpy, screen);
    if (is_ext_supported(glx_extensions, "GLX_ARB_create_context")) {
        *out_ctx = glXCreateNewContext(dpy, configs[best_fbc], GLX_RGBA_TYPE, 0, True);
    } else {
        *out_ctx = glXCreateContextAttribsARB(dpy, configs[best_fbc], 0, True, context_attribs);
    }

    XFree(configs);

    if (!glXIsDirect(dpy, *out_ctx)) {
        return false;
    }

    return true;
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

static long read_file(const char *path, char **output)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("ERROR %s\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    *output = malloc(sizeof(char) * len);
    if (!*output) {
        return -1;
    }
    len = fread(*output, sizeof(char), len, f);
    fclose(f);
    return len;
}

int main(void)
{
    struct Memory memory;
    memory.object_memory_max_size = 4 * 1024 * 1024 * 1024ULL;
    memory.object_memory = (struct ObjectMegastruct *) calloc(memory.object_memory_max_size, 1);
    memory.objects_in_memory = 0;
    memory.initialized = false;

    Display *dpy = XOpenDisplay(NULL);

    XKeyboardControl kbctrl;
    kbctrl.auto_repeat_mode = AutoRepeatModeOff;
    XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kbctrl);

    int scr = DefaultScreen(dpy);

    GLXContext context;
    XVisualInfo *visual;
    if (!load_opengl(dpy, scr, &context, &visual)) {
        return 1;
    }

    load_extensions();

    uint16_t width = 600, height = 400;
    Window window = XCreateWindow(
        dpy,
        RootWindow(dpy, scr),
        0, 0,
        width, height,
        0,
        visual->depth,
        InputOutput,
        visual->visual,
        CWBackPixel|CWBorderPixel|CWEventMask|CWColormap,
        &(XSetWindowAttributes) {
            .background_pixel = BlackPixel(dpy, scr),
            .border_pixel = BlackPixel(dpy, scr),
            .override_redirect = True,
            .event_mask = KeyPressMask|KeyReleaseMask|FocusChangeMask|ExposureMask|StructureNotifyMask,
            .colormap = XCreateColormap(dpy, RootWindow(dpy, scr), visual->visual, AllocNone)
        }
    );
    XStoreName(dpy, window, "GL GAME");
    Atom WMCloseAtom = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, window, &WMCloseAtom, 1);
    XMapRaised(dpy, window);
    XSync(dpy, False);

    glXMakeCurrent(dpy, window, context);
    printf("GL Vendor %s\n", glGetString(GL_VENDOR));
    printf("GL Renderer %s\n", glGetString(GL_RENDERER));
    printf("GL Version %s\n", glGetString(GL_VERSION));
    printf("GL Shading Language %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

#ifdef _POSIX_MONOTONIC_CLOCK
        {
            struct timespec monotonic_test;
            if (clock_gettime(CLOCK_MONOTONIC, &monotonic_test) == 0) {
                CLOCKTYPE = CLOCK_MONOTONIC;
            }
        }
#endif

    double start_time = gettime();
    double last_update_time = 0.0;

    enum KeyState keys[K_LAST] = {KS_RELEASED};
    enum KeyState keys_last_frame[K_LAST] = {KS_RELEASED};

    bool running = true;
    while (running) {
        while (XPending(dpy) > 0) {
            XEvent e;
            XNextEvent(dpy, &e);
            switch (e.type) {
                case KeyPress: {
                    KeySym sym = XLookupKeysym(&e.xkey, 0);
                    check_keys(keys, sym, KS_PRESSED);
                } break;
                case KeyRelease: {
                    KeySym sym = XLookupKeysym(&e.xkey, 0);
                    check_keys(keys, sym, KS_RELEASED);
                } break;
                case FocusOut: {
                    for (size_t i = 0; i < K_LAST; ++i) {
                        keys[i] = KS_RELEASED;
                    }
                } break;
                case Expose: {
                    width = e.xexpose.width;
                    height = e.xexpose.height;
                    glViewport(0, 0, width, height);
                } break;
                case MappingNotify: {
                    XRefreshKeyboardMapping(&e.xmapping);
                } break;
                case ClientMessage: {
                    if (e.xclient.data.l[0] == (long) WMCloseAtom) {
                        running = false;
                    }
                } break;
                case DestroyNotify: {
                    running = false;
                } break;
            }
        }

        double time = getelapsedtime(start_time);
        float delta = time - last_update_time;
        last_update_time = time;
        update(&memory, delta, keys, keys_last_frame, &running);

        for (size_t i = 0; i < memory.objects_in_memory; ++i) {
            if (memory.object_memory[i].valid) {
                switch (memory.object_memory[i].type) {
                    case OT_PROGRAM: {
                        if (!memory.object_memory[i].data.program.loaded) {
                            long vlen = read_file(memory.object_memory[i].data.program.vertex_path, &memory.object_memory[i].data.program.vertex_src);
                            long flen = read_file(memory.object_memory[i].data.program.fragment_path, &memory.object_memory[i].data.program.fragment_src);
                            if (vlen == -1 || flen == -1) {
                                fputs("failed to read file\n", stderr);
                            }
                            memory.object_memory[i].data.program.loaded = true;
                        }
                        if (!memory.object_memory[i].data.program.freed && memory.object_memory[i].data.program.committed) {
                            free(memory.object_memory[i].data.program.vertex_src);
                            free(memory.object_memory[i].data.program.fragment_src);
                            memory.object_memory[i].data.program.freed = true;
                        }
                    } break;
                    default: break;
                }
            }
        }

        glXSwapBuffers(dpy, window);

        for (size_t i = 0; i < K_LAST; ++i) {
            keys_last_frame[i] = keys[i];
        }
    }

    kbctrl.auto_repeat_mode = AutoRepeatModeDefault;
    XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kbctrl);
    XSync(dpy, True);
    return EXIT_SUCCESS;
}
