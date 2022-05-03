#ifndef GAME_H
#define GAME_H

#include "darray.h"
#include "fmath.h"
#include "input.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct Memory {
    void *permenant_storage;
    size_t permenant_storage_size;

    // void *transient_storage;
    // size_t transient_storage_size;

    bool initialized;
};

enum FrameWhence {
    FW_CURRENT,
    FW_PREVIOUS,
    FW_LAST
};

struct WindowState {
    unsigned int width, height;
    bool running;
    bool fullscreen[FW_LAST];
    bool just_resized;
};

enum ClearValues {
    CV_Color = 1,
    CV_Depth = 2,
    CV_Stencil = 4
};

enum AttributeType {
    AT_Integer,
    AT_Float,
    AT_Float2,
    AT_Float3,
    AT_Float4,
    AT_Mat3,
    AT_Mat4,
    AT_LAST
};

#define ATTRIBN(TYPE, NAME, NORM) {TYPE, NAME, NORM, 0, 0}
#define ATTRIB(TYPE, NAME) ATTRIBN(TYPE, NAME, false)
struct BufferAttribute {
    enum AttributeType type;
    const char *name;
    bool normalized;
    size_t size;
    size_t offset;
};

struct BufferLayout {
    struct BufferAttribute attributes[16];
    size_t num_attributes;
    size_t stride;
};

struct VertexBuffer {
    uint32_t id;
    size_t buffer_size;
    struct BufferLayout layout;
};

struct IndexBuffer {
    uint32_t id;
    size_t num_indices;
};

struct VertexArray {
    uint32_t id;
    struct VertexBuffer vertex_buffers[3];
    size_t num_vertex_buffers;
    struct IndexBuffer index_buffer;
};

struct ShaderProgram {
    uint32_t id, vertexid, fragmentid;
    const char *name;
};

enum TextureType {
    TT_2D,
    TT_3D,
    TT_Cubemap,
    TT_LAST
};

struct Texture {
    uint32_t id;
    uint32_t width, height, depth;
    const char *path;
};

struct Graphics {
    struct ShaderProgram (*shader)(const char *name);
    void (*shader_bind)(const struct ShaderProgram *program);
    void (*shader_unbind)(void);
    void (*shader_set_mat4)(const struct ShaderProgram *program, const char *name, const float *value);
    void (*shader_set_vec2)(const struct ShaderProgram *program, const char *name, V2 value);
    void (*shader_set_vec4)(const struct ShaderProgram *program, const char *name, V4 value);
    void (*shader_set_int)(const struct ShaderProgram *program, const char *name, int value);
    void (*shader_set_float)(const struct ShaderProgram *program, const char *name, float value);

    struct VertexBuffer (*vertex_buffer)(const float *vertices, size_t vertices_size, const struct BufferLayout *layout);
    struct IndexBuffer (*index_buffer)(const uint32_t *indices, size_t num_indices);

    struct BufferLayout (*buffer_layout)(const struct BufferAttribute *attributes, size_t num_attributes);

    struct VertexArray (*vertex_array)(void);
    void (*vertex_array_bind)(const struct VertexArray *va);
    void (*vertex_array_unbind)(void);
    void (*vertex_array_add_vertex_buffer)(struct VertexArray *va, const struct VertexBuffer *vb);
    void (*vertex_array_set_index_buffer)(struct VertexArray *va, const struct IndexBuffer *ib);

    struct Texture (*texture)(const char *texture_path, enum TextureType type);
    void (*texture_bind)(const struct Texture *t, uint32_t slot);
    void (*texture_unbind)(uint32_t slot);

    struct RenderCommand {
        void (*clear_color)(float r, float g, float b, float a);
        void (*clear)(enum ClearValues c);
        void (*draw_indexed)(const struct VertexArray *va);
    } render_command;
};

enum InputMode {
    IM_RELEASED,
    IM_CONFINED,
    IM_CAPTURED,
    IM_LAST
};

struct Input {
    enum KeyState keys[K_LAST][FW_LAST];
    enum ButtonState buttons[MB_LAST][FW_LAST];
    int mx, my, dx, dy, dz;

    enum InputMode mode[FW_LAST];
};

struct Time {
    double origin, elapsed[FW_LAST], delta;
};

#define GAME_UPDATE(NAME) void NAME(struct Memory *memory, struct WindowState *window, const struct Graphics *gfx, struct Input *input, const struct Time *time)
GAME_UPDATE(game_update);

#endif /* GAME_H */
