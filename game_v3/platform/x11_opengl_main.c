//bin/true; GAMEROOT="`dirname $0`/.."; mkdir -p "$GAMEROOT/bin" && ${CC:-clang} -o "$GAMEROOT/bin/game" -std=c99 -Wall -Wextra -pedantic -O1 -ggdb -pthread $0 "$GAMEROOT/game/game.c" -I"$GAMEROOT" -lm -lGL -lX11; exit

#define INCLUDE_SRC
#include "game/game.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#ifndef __USE_POSIX199309
#define __USE_POSIX199309
#endif
#include <time.h>

#include <GL/glx.h>

#define GL_FUNCTIONS \
    /*  ret, name, params */ \
    GL_EXPAND(void, CreateBuffers, GLsizei n, GLuint *buffers) \
    GL_EXPAND(void, NamedBufferData, GLuint buffer, GLsizeiptr size, const GLvoid *data, GLenum usage) \
    GL_EXPAND(void, CreateVertexArrays, GLsizei n, GLuint *arrays) \
    GL_EXPAND(void, BindVertexArray, GLuint array) \
    GL_EXPAND(void, EnableVertexArrayAttrib, GLuint vaobj, GLuint index) \
    GL_EXPAND(void, DisableVertexArrayAttrib, GLuint vaobj, GLuint index) \
    GL_EXPAND(void, VertexArrayElementBuffer, GLuint vaobj, GLuint buffer) \
    GL_EXPAND(void, VertexArrayVertexBuffer, GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) \
    GL_EXPAND(void, VertexArrayAttribFormat, GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) \
    GL_EXPAND(void, VertexArrayAttribBinding, GLuint vaobj, GLuint attribindex, GLuint bindingindex) \
    GL_EXPAND(GLuint, CreateShader, GLenum shaderType) \
    GL_EXPAND(void, DeleteShader, GLuint shader) \
    GL_EXPAND(void, ShaderSource, GLuint shader, GLsizei count, const GLchar **string, const GLint *length) \
    GL_EXPAND(void, CompileShader, GLuint shader) \
    GL_EXPAND(GLuint, CreateProgram, void) \
    GL_EXPAND(void, AttachShader, GLuint program, GLuint shader) \
    GL_EXPAND(void, DetachShader, GLuint program, GLuint shader) \
    GL_EXPAND(void, LinkProgram, GLuint program) \
    GL_EXPAND(void, UseProgram, GLuint program) \
    GL_EXPAND(void, GetShaderiv, GLuint shader, GLenum pname, GLint *params) \
    GL_EXPAND(void, GetShaderInfoLog, GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog) \
    GL_EXPAND(void, GetProgramiv, GLuint program, GLenum pname, GLint *params) \
    GL_EXPAND(void, GetProgramInfoLog, GLuint program, GLsizei maxLength, GLsizei *length, GLchar *info) \
    GL_EXPAND(GLint, GetUniformLocation, GLuint program, const GLchar *name) \
    GL_EXPAND(void, Uniform1i, GLint location, GLint v0) \
    GL_EXPAND(void, Uniform1f, GLint location, GLfloat v0) \
    GL_EXPAND(void, Uniform2f, GLint location, GLfloat v0, GLfloat v1) \
    GL_EXPAND(void, Uniform4f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) \
    GL_EXPAND(void, UniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
    GL_EXPAND(void, CreateTextures, GLenum target, GLsizei n, GLuint *textures) \
    GL_EXPAND(void, TextureStorage2D, GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) \
    GL_EXPAND(void, TextureParameteri, GLuint texture, GLenum pname, GLint param) \
    GL_EXPAND(void, TextureSubImage2D, GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) \
    GL_EXPAND(void, BindTextureUnit, GLuint unit, GLuint texture)
#define GL_EXPAND(RET, NAME, ...) static RET (*gl##NAME)(__VA_ARGS__);
GL_FUNCTIONS
#undef GL_EXPAND

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);

static void load_opengl(Display *dpy, int screen, GLXContext *out_ctx, XVisualInfo **out_visual)
{
    int fbcount;
    GLXFBConfig *configs = glXChooseFBConfig(dpy, screen, (const GLint []) {
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
    }, &fbcount);

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

    *out_visual = glXGetVisualFromFBConfig(dpy, configs[best_fbc]);
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB((const GLubyte *) "glXCreateContextAttribsARB");
    *out_ctx = glXCreateContextAttribsARB(dpy, configs[best_fbc], 0, True, (const int []) {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 5,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB|GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        None
    });

    XFree(configs);
}

#define ASSERT(CHECK, ...) if (!(CHECK)) {fprintf(stderr, __VA_ARGS__);fputc('\n', stderr);exit(EXIT_FAILURE);}

static uint64_t gettime(clockid_t clock_type)
{
    struct timespec ts;
    clock_gettime(clock_type, &ts);
    return (uint64_t) ts.tv_sec * 1000000000ULL + (uint64_t) ts.tv_nsec;
}

static double getelapsedtime(clockid_t clock_type, uint64_t since)
{
    return (double) (gettime(clock_type) - since) / 1000000000ULL;
}

static void clear(enum ClearValues c)
{
    glClear(
        GL_COLOR_BUFFER_BIT * ((c & CV_Color) == CV_Color) +
        GL_DEPTH_BUFFER_BIT * ((c & CV_Depth) == CV_Depth) +
        GL_STENCIL_BUFFER_BIT * ((c & CV_Stencil) == CV_Stencil)
    );
}

static long read_file(const char *path, void **output)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("File %s not found.\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    *output = malloc(len + 1);
    if (!*output) {
        return -1;
    }
    len = fread(*output, 1, len, f);
    ((uint8_t *)(*output))[len] = 0;
    fclose(f);
    return len;
}

struct BMPFile {
    uint8_t *data;
    uint32_t width, height;
    void *header_ptr;
};

struct BMPHeader {
    uint16_t FileType;
    uint32_t FileSize;
    uint16_t Reserved1;
    uint16_t Reserved2;
    uint32_t BitmapOffset;
    uint32_t Size;
    int32_t Width;
    int32_t Height;
    uint16_t Planes;
    uint16_t BitsPerPixel;
} __attribute__((__packed__));

static struct BMPFile read_bmp_file(const char *path)
{
    void *file_data;
    read_file(path, &file_data);
    ASSERT(file_data, "failed to read file %s", path);
    struct BMPHeader *bmp = (struct BMPHeader *) file_data;
    struct BMPFile result;
    result.data = ((uint8_t *) file_data) + bmp->BitmapOffset;
    result.width = bmp->Width;
    result.height = bmp->Height;
    result.header_ptr = file_data;
    return result;
}

static DArray loaded_programs = STATIC_INIT_DARRAY(struct ShaderProgram);
static struct ShaderProgram shader(const char *name)
{
    struct ShaderProgram result;
    char infolog[1024];
    int success;

    for (size_t i = 0; i < loaded_programs.num_elements; ++i) {
        struct ShaderProgram *s = ((struct ShaderProgram *) loaded_programs.start) + i;
        if (s->name == name) {
            return *s;
        }
    }

    result.id = glCreateProgram();
    result.name = name;

    char path[128];
    snprintf(path, 128, "shaders/%s.glsl", name);

    void *shader_src;
    read_file(path, &shader_src);

    char *new_shader = strstr(shader_src, "#type");
    while (new_shader) {
        char *shader_type = strchr(new_shader, ' ') + 1;
        if (shader_type) {
            GLenum gl_shader_type = 0;
            if (strncmp(shader_type, "vertex", 6) == 0)
                gl_shader_type = GL_VERTEX_SHADER;
            else if (strncmp(shader_type, "fragment", 8) == 0)
                gl_shader_type = GL_FRAGMENT_SHADER;

            char *shader_start = strchr(shader_type, '\n') + 1;
            new_shader = strstr(shader_start, "#type");
            if (new_shader) *new_shader++ = '\0';
            uint32_t id = glCreateShader(gl_shader_type);
            if (gl_shader_type == GL_VERTEX_SHADER)
                result.vertexid = id;
            else if (gl_shader_type == GL_FRAGMENT_SHADER)
                result.fragmentid = id;
            glShaderSource(id, 1, (const char **) &shader_start, NULL);
            glCompileShader(id);
            glGetShaderiv(id, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(result.vertexid, 1024, NULL, infolog);
                fprintf(stderr, "%s\n", infolog);
            }
            glAttachShader(result.id, id);
        }
    }

    free(shader_src);

    glLinkProgram(result.id);
    glGetProgramiv(result.id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(result.id, 1024, NULL, infolog);
        fprintf(stderr, "%s\n", infolog);
    }
    glDetachShader(result.id, result.vertexid);
    glDetachShader(result.id, result.fragmentid);
    glDeleteShader(result.vertexid);
    glDeleteShader(result.fragmentid);

    darray_append(&loaded_programs, &result);

    return result;
}

static void shader_bind(const struct ShaderProgram *program)
{
    glUseProgram(program->id);
}

static void shader_unbind(void)
{
    glUseProgram(0);
}

static void shader_set_mat4(const struct ShaderProgram *program, const char *name, const float *value)
{
    glUniformMatrix4fv(glGetUniformLocation(program->id, name), 1, GL_TRUE, value);
}

static void shader_set_vec2(const struct ShaderProgram *program, const char *name, V2 value)
{
    glUniform2f(glGetUniformLocation(program->id, name), value.x, value.y);
}

static void shader_set_vec4(const struct ShaderProgram *program, const char *name, V4 value)
{
    glUniform4f(glGetUniformLocation(program->id, name), value.x, value.y, value.z, value.w);
}

static void shader_set_int(const struct ShaderProgram *program, const char *name, int value)
{
    glUniform1i(glGetUniformLocation(program->id, name), value);
}

static void shader_set_float(const struct ShaderProgram *program, const char *name, float value)
{
    glUniform1f(glGetUniformLocation(program->id, name), value);
}

static const size_t ShaderAttributeSizeof[AT_LAST] = {
    [AT_Integer] = sizeof(int),
    [AT_Float] = sizeof(float),
    [AT_Float2] = 2 * sizeof(float),
    [AT_Float3] = 3 * sizeof(float),
    [AT_Float4] = 4 * sizeof(float),
    [AT_Mat3] = 3 * 3 * sizeof(float),
    [AT_Mat4] = 4 * 4 * sizeof(float)
};

static struct BufferLayout buffer_layout(const struct BufferAttribute *attributes, size_t num_attributes)
{
    struct BufferLayout result;
    memcpy(result.attributes, attributes, sizeof(struct BufferAttribute) * num_attributes);
    size_t offset = 0;
    for (size_t i = 0; i < num_attributes; ++i) {
        result.attributes[i].size = ShaderAttributeSizeof[result.attributes[i].type];
        result.attributes[i].offset = offset;
        offset += result.attributes[i].size;
    }
    result.num_attributes = num_attributes;
    result.stride = offset;
    return result;
}

static struct VertexArray vertex_array(void)
{
    struct VertexArray result = {0};
    glCreateVertexArrays(1, &result.id);
    result.index_buffer.id = UINT32_MAX;
    return result;
}

static void vertex_array_bind(const struct VertexArray *va)
{
    glBindVertexArray(va->id);
}

static void vertex_array_unbind(void)
{
    glBindVertexArray(0);
}

static struct VertexBuffer vertex_buffer(const float *vertices, size_t vertices_size, const struct BufferLayout *layout)
{
    struct VertexBuffer result = {0};
    glCreateBuffers(1, &result.id);
    glNamedBufferData(result.id, vertices_size, vertices, GL_STATIC_DRAW);
    result.buffer_size = vertices_size;
    if (layout)
        result.layout = *layout;
    return result;
}

static const GLenum ShaderAttributeToGLType[AT_LAST] = {
    [AT_Integer] = GL_INT,
    [AT_Float] = GL_FLOAT,
    [AT_Float2] = GL_FLOAT,
    [AT_Float3] = GL_FLOAT,
    [AT_Float4] = GL_FLOAT,
    [AT_Mat3] = GL_FLOAT,
    [AT_Mat4] = GL_FLOAT
};

static const size_t ShaderAttributeComponentCount[AT_LAST] = {
    [AT_Integer] = 1,
    [AT_Float] = 1,
    [AT_Float2] = 2,
    [AT_Float3] = 3,
    [AT_Float4] = 4,
    [AT_Mat3] = 9,
    [AT_Mat4] = 16
};

static void vertex_array_add_vertex_buffer(struct VertexArray *va, const struct VertexBuffer *vb)
{
    ASSERT(vb->layout.num_attributes != 0, "VBO(%d): layout not set", vb->id);
    for (size_t i = 0; i < vb->layout.num_attributes; ++i) {
        glEnableVertexArrayAttrib(va->id, i);
        glVertexArrayAttribFormat(va->id, i, ShaderAttributeComponentCount[vb->layout.attributes[i].type], ShaderAttributeToGLType[vb->layout.attributes[i].type], vb->layout.attributes[i].normalized, vb->layout.attributes[i].offset);
        glVertexArrayAttribBinding(va->id, i, va->num_vertex_buffers);
    }
    glVertexArrayVertexBuffer(va->id, va->num_vertex_buffers, vb->id, 0, vb->layout.stride);
    va->vertex_buffers[va->num_vertex_buffers++] = *vb;
}

static struct IndexBuffer index_buffer(const uint32_t *indices, size_t num_indices)
{
    struct IndexBuffer result;
    glCreateBuffers(1, &result.id);
    glNamedBufferData(result.id, num_indices * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    result.num_indices = num_indices;
    return result;
}

static void vertex_array_set_index_buffer(struct VertexArray *va, const struct IndexBuffer *ib)
{
    va->index_buffer = *ib;
    glVertexArrayElementBuffer(va->id, va->index_buffer.id);
}

static void draw_indexed(const struct VertexArray *va)
{
    glDrawElements(GL_TRIANGLES, va->index_buffer.num_indices, GL_UNSIGNED_INT, 0);
}

static const GLenum TextureTypeToGLType[TT_LAST] = {
    [TT_2D] = GL_TEXTURE_2D,
    [TT_3D] = GL_TEXTURE_3D,
    [TT_Cubemap] = GL_TEXTURE_CUBE_MAP
};

static DArray loaded_textures = STATIC_INIT_DARRAY(struct Texture);
static struct Texture texture(const char *texture_path, enum TextureType type)
{
    ASSERT(type != TT_3D, "3D textures aren't supported yet");
    struct Texture result;
    for (size_t i = 0; i < loaded_textures.num_elements; ++i) {
        struct Texture *t = ((struct Texture *) loaded_textures.start) + i;
        if (t->path == texture_path) {
            return *t;
        }
    }

    struct BMPFile bmp = read_bmp_file(texture_path);
    glCreateTextures(TextureTypeToGLType[type], 1, &result.id);
    glTextureStorage2D(result.id, 1, GL_RGBA8, bmp.width, bmp.height);
    glTextureParameteri(result.id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(result.id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureSubImage2D(result.id, 0, 0, 0, bmp.width, bmp.height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, bmp.data);
    free(bmp.header_ptr);
    result.width = bmp.width;
    result.height = bmp.height;
    result.depth = 0;
    result.path = texture_path;
    darray_append(&loaded_textures, &result);
    return result;
}

static void texture_bind(const struct Texture *t, uint32_t slot)
{
    glBindTextureUnit(slot, t->id);
}

static void texture_unbind(uint32_t slot)
{
    glBindTextureUnit(slot, 0);
}

static const uint32_t XLIB_KEYMAP[K_LAST] = {
    [K_SPACE] = XK_space, [K_ENTER] = XK_Return, [K_TAB] = XK_Tab, [K_BACKTICK] = XK_grave, [K_ESCAPE] = XK_Escape, [K_BACKSPACE] = XK_BackSpace, [K_SEMICOLON] = XK_semicolon,
    [K_LEFT_ARROW] = XK_Left, XK_Up, XK_Right, XK_Down,
    [K_A] = XK_a, XK_b, XK_c, XK_d, XK_e, XK_f, XK_g, XK_h, XK_i, XK_j, XK_k, XK_l, XK_m, XK_n, XK_o, XK_p, XK_q, XK_r, XK_s, XK_t, XK_u, XK_v, XK_w, XK_x, XK_y, XK_z,
    [K_0] = XK_0, XK_1, XK_2, XK_3, XK_4, XK_5, XK_6, XK_7, XK_8, XK_9,
    [K_F1] = XK_F1, XK_F2, XK_F3, XK_F4, XK_F5, XK_F6, XK_F7, XK_F8, XK_F9, XK_F10, XK_F11, XK_F12,
    [K_LEFT_CONTROL] = XK_Control_L, [K_LEFT_SHIFT] = XK_Shift_L, [K_LEFT_ALT] = XK_Alt_L
};

#define KILOBYTES(B) ((B) * 1000ULL)
#define MEGABYTES(B) (KILOBYTES(B) * 1000ULL)
#define GIGABYTES(B) (MEGABYTES(B) * 1000ULL)

int main(void)
{
    struct Memory memory = {0};
    memory.permenant_storage_size = GIGABYTES(4);
    memory.permenant_storage = malloc(memory.permenant_storage_size);
    ASSERT(memory.permenant_storage, "failed to malloc required memory");

    clockid_t clock_type = CLOCK_REALTIME;
    #ifdef CLOCK_MONOTONIC
    {
        struct timespec monotonic_test;
        if (clock_gettime(CLOCK_MONOTONIC, &monotonic_test) == 0) {
            clock_type = CLOCK_MONOTONIC;
        }
    }
    #endif

    Display *dpy = XOpenDisplay(NULL);
    int scr = DefaultScreen(dpy);

    XKeyboardControl kbctrl;
    kbctrl.auto_repeat_mode = AutoRepeatModeOff;
    XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kbctrl);

    GLXContext context;
    XVisualInfo *visual;
    load_opengl(dpy, scr, &context, &visual);
    #define GL_EXPAND(RET, NAME, ...) gl##NAME = (RET (*)(__VA_ARGS__)) glXGetProcAddressARB((const GLubyte *) "gl"#NAME);
    GL_FUNCTIONS
    #undef GL_EXPAND

    struct WindowState window_state = {
        .width = 600,
        .height = 400,
        .running = true,
        .just_resized = true
    };
    Window window = XCreateWindow(
        dpy,
        RootWindow(dpy, scr),
        0, 0,
        window_state.width, window_state.height,
        0,
        visual->depth,
        InputOutput,
        visual->visual,
        CWEventMask|CWColormap,
        &(XSetWindowAttributes) {
            .event_mask = KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|FocusChangeMask|ExposureMask|StructureNotifyMask,
            .colormap = XCreateColormap(dpy, RootWindow(dpy, scr), visual->visual, AllocNone)
        }
    );
    Cursor clear_cursor;
    {
        XColor xcolor;
        XAllocColor(dpy, DefaultColormap(dpy, scr), &xcolor);
        Pixmap empty = XCreatePixmap(dpy, window, 16, 16, 1);
        clear_cursor = XCreatePixmapCursor(dpy, empty, empty, &xcolor, &xcolor, 0, 0);
        XFreePixmap(dpy, empty);
        XFreeColors(dpy, DefaultColormap(dpy, scr), &xcolor.pixel, 1, 0);
    }
    // XStoreName(dpy, window, "Game");
    Atom WMClose = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    Atom WMState = XInternAtom(dpy, "_NET_WM_STATE", True);
    Atom WMFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", True);

    XSetWMProtocols(dpy, window, &WMClose, 1);
    XMapWindow(dpy, window);
    XSync(dpy, False);

    glXMakeCurrent(dpy, window, context);
    {
        int maj, min;
        glGetIntegerv(GL_MAJOR_VERSION, &maj);
        glGetIntegerv(GL_MINOR_VERSION, &min);
        ASSERT(maj > 4 || (maj == 4 && min >= 5), "Engine does not support OpenGL below 4.5");
    }
    printf("GL Vendor %s\n", glGetString(GL_VENDOR));
    printf("GL Renderer %s\n", glGetString(GL_RENDERER));
    printf("GL Version %s\n", glGetString(GL_VERSION));
    printf("GL Shading Language %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    {
        PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC) glXGetProcAddressARB((const GLubyte *) "glXSwapIntervalEXT");
        if (glXSwapIntervalEXT) {
            glXSwapIntervalEXT(dpy, window, 0);
            printf("VSync disabled.\n");
        }
    }

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    struct Graphics gfx = {
        .shader = shader,
        .shader_bind = shader_bind,
        .shader_unbind = shader_unbind,
        .shader_set_mat4 = shader_set_mat4,
        .shader_set_vec2 = shader_set_vec2,
        .shader_set_vec4 = shader_set_vec4,
        .shader_set_int = shader_set_int,
        .shader_set_float = shader_set_float,
        .vertex_buffer = vertex_buffer,
        .index_buffer = index_buffer,
        .buffer_layout = buffer_layout,
        .vertex_array = vertex_array,
        .vertex_array_bind = vertex_array_bind,
        .vertex_array_unbind = vertex_array_unbind,
        .vertex_array_add_vertex_buffer = vertex_array_add_vertex_buffer,
        .vertex_array_set_index_buffer = vertex_array_set_index_buffer,
        .texture = texture,
        .texture_bind = texture_bind,
        .texture_unbind = texture_unbind,
        .render_command = {
            .clear_color = glClearColor,
            .clear = clear,
            .draw_indexed = draw_indexed
        }
    };

    struct Input input = {0};

    struct Time time = {
        .origin = gettime(clock_type)
    };

    double last_fps_time = 0.0;
    uint32_t fps = 0;

    int warpx = 0, warpy = 0;
    while (window_state.running) {
        while (XPending(dpy) > 0) {
            XEvent e;
            XNextEvent(dpy, &e);
            switch (e.type) {
                case KeyPress:
                case KeyRelease: {
                    const enum KeyState state = e.type == KeyPress ? KS_PRESSED : KS_RELEASED;
                    const KeySym sym = XLookupKeysym(&e.xkey, 0);
                    for (size_t i = 0; i < K_LAST; ++i) {
                        if (XLIB_KEYMAP[i] == sym) {
                            input.keys[i][FW_CURRENT] = state;
                            break;
                        }
                    }
                } break;
                case ButtonPress:
                case ButtonRelease: {
                    const enum ButtonState state = e.type == ButtonPress ? MBS_DOWN : MBS_UP;
                    if (e.xbutton.button > 5 || e.xbutton.button < 1) break;
                    switch (e.xbutton.button) {
                        case Button4: input.dz -= 1; break; // scroll up
                        case Button5: input.dz += 1; break; // scroll down
                        default: input.buttons[e.xbutton.button - 1][FW_CURRENT] = state; break;
                    }
                } break;
                case MotionNotify: {
                    input.mx = e.xmotion.x;
                    input.my = e.xmotion.y;
                    if (input.mode[FW_CURRENT] == IM_CAPTURED) {
                        input.dx += e.xmotion.x - warpx;
                        input.dy += e.xmotion.y - warpy;
                    }
                } break;
                case FocusOut: {
                    for (size_t i = 0; i < K_LAST; ++i) {
                        input.keys[i][FW_CURRENT] = KS_RELEASED;
                    }
                    for (size_t i = 0; i < MB_LAST; ++i) {
                        input.buttons[i][FW_CURRENT] = MBS_UP;
                    }
                } break;
                case Expose: {
                    window_state.width = e.xexpose.width;
                    window_state.height = e.xexpose.height;
                    warpx = window_state.width / 2.0f;
                    warpy = window_state.height / 2.0f;
                    glViewport(0, 0, window_state.width, window_state.height);
                    window_state.just_resized = true;
                } break;
                case MappingNotify: {
                    XRefreshKeyboardMapping(&e.xmapping);
                } break;
                case ClientMessage: {
                    if (e.xclient.data.l[0] == (long) WMClose) {
                        window_state.running = false;
                    }
                } break;
                case DestroyNotify: {
                    window_state.running = false;
                } break;
                default: break;
            }
        }

        time.elapsed[FW_CURRENT] = getelapsedtime(clock_type, time.origin);
        time.delta = time.elapsed[FW_CURRENT] - time.elapsed[FW_PREVIOUS];

        last_fps_time += time.delta;
        ++fps;

        if (last_fps_time >= 1) {
            char name[64];
            snprintf(name, 64, "%f ms/f - %d fps", 1000.0f / fps, fps);
            XStoreName(dpy, window, name);
            last_fps_time = 0.0;
            fps = 0;
        }

        game_update(&memory, &window_state, &gfx, &input, &time);
        glXSwapBuffers(dpy, window);

        if (input.mode[FW_CURRENT] != input.mode[FW_PREVIOUS]) {
            switch (input.mode[FW_CURRENT]) {
                case IM_RELEASED: {
                    XUngrabPointer(dpy, CurrentTime);
                } break;
                case IM_CONFINED: {
                    XUngrabPointer(dpy, CurrentTime);
                    XGrabPointer(dpy, window, true, 0, GrabModeAsync, GrabModeAsync, window, None, CurrentTime);
                } break;
                case IM_CAPTURED: {
                    XUngrabPointer(dpy, CurrentTime);
                    XGrabPointer(dpy, window, true, 0, GrabModeAsync, GrabModeAsync, window, clear_cursor, CurrentTime);
                } break;
                default: break;
            }
            XFlush(dpy);
        }

        if (input.mode[FW_CURRENT] == IM_CAPTURED) {
            XWarpPointer(dpy, None, window, 0, 0, 0, 0, warpx, warpy);
            XFlush(dpy);
            input.dx = 0;
            input.dy = 0;
        }

        if (window_state.fullscreen[FW_CURRENT] != window_state.fullscreen[FW_PREVIOUS]) {
            XSendEvent(dpy, RootWindow(dpy, scr), False, SubstructureNotifyMask, &(XEvent) {
                .xclient.type = ClientMessage,
                .xclient.display = dpy,
                .xclient.window = window,
                .xclient.message_type = WMState,
                .xclient.format = 32,
                .xclient.data.l = {
                    window_state.fullscreen[FW_CURRENT],
                    WMFullscreen,
                    None,
                    0,
                    1
                }
            });
        }

        input.dz = 0;
        time.elapsed[FW_PREVIOUS] = time.elapsed[FW_CURRENT];
        for (size_t i = 0; i < K_LAST; ++i) {
            input.keys[i][FW_PREVIOUS] = input.keys[i][FW_CURRENT];
        }
        for (size_t i = 0; i < MB_LAST; ++i) {
            input.buttons[i][FW_PREVIOUS] = input.buttons[i][FW_CURRENT];
        }
        input.mode[FW_PREVIOUS] = input.mode[FW_CURRENT];
        window_state.fullscreen[FW_PREVIOUS] = window_state.fullscreen[FW_CURRENT];
    }

    kbctrl.auto_repeat_mode = AutoRepeatModeDefault;
    XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kbctrl);
    XSync(dpy, True);
    return EXIT_SUCCESS;
}
