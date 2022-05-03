#define INCLUDE_SRC
#include "opengl.h"
#include "keys.h"
#include "fmath.h"

#include <stdio.h> // remove this eventually
#include <stdbool.h>

#define LENGTH(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))

float vertices[] = {
    -0.5f, 0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    0.5f, 0.5f, 0.0f,
    0.5f, -0.5f, 0.0f
};

unsigned int elements[] = {
    0, 1, 2,
    2, 1, 3
};

struct RawModel {
    unsigned int VAO, vertex_count, program_id;
};

struct Program {
    unsigned int program_id, vertex_id, fragment_id;
    const char *vertex_path, *fragment_path;
    char *vertex_src, *fragment_src;
    bool committed, loaded, freed;
};

enum ObjectType {
    OT_RAWMODEL,
    OT_PROGRAM
};

struct ObjectMegastruct {
    size_t id;
    bool valid;
    enum ObjectType type;
    union {
        struct RawModel raw_model;
        struct Program program;
    } data;
};

struct Memory {
    struct ObjectMegastruct *object_memory;
    size_t objects_in_memory;
    size_t object_memory_max_size;
    bool initialized;
};

void store_data_in_vbo(unsigned int attrib_number, float *vertices, size_t sizeof_vertices, unsigned int *indices, size_t sizeof_indices)
{
    unsigned int buffers[2];
    GL.GenBuffers(2, buffers);
    GL.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[1]);
    GL.BufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof_indices, indices, GL_STATIC_DRAW);

    GL.BindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    GL.BufferData(GL_ARRAY_BUFFER, sizeof_vertices, vertices, GL_STATIC_DRAW);
    GL.VertexAttribPointer(attrib_number, 3, GL_FLOAT, GL_FALSE, 0, 0);
    GL.BindBuffer(GL_ARRAY_BUFFER, 0);
}

struct RawModel load_data_to_vao(float *positions, size_t num_positions, unsigned int *indices, size_t num_indices)
{
    struct RawModel result;

    GL.GenVertexArrays(1, &result.VAO);
    GL.BindVertexArray(result.VAO);

    store_data_in_vbo(0, positions, num_positions * sizeof(float), indices, num_indices * sizeof(unsigned int));

    GL.BindVertexArray(0);

    result.program_id = GL.CreateProgram();
    result.vertex_count = num_indices;
    return result;
}

unsigned int draw_mode = 2;
void update(struct Memory *memory, float delta, enum KeyState *keys, enum KeyState *keys_last_frame, bool *running)
{
    if (!memory->initialized) {
        glEnable(GL_DEPTH_TEST);
        memory->object_memory[memory->objects_in_memory].id = memory->objects_in_memory;
        memory->object_memory[memory->objects_in_memory].valid = true;
        memory->object_memory[memory->objects_in_memory].type = OT_RAWMODEL;
        memory->object_memory[memory->objects_in_memory].data.raw_model = load_data_to_vao(vertices, LENGTH(vertices), elements, LENGTH(elements));
        ++memory->objects_in_memory;

        memory->object_memory[memory->objects_in_memory].id = memory->objects_in_memory;
        memory->object_memory[memory->objects_in_memory].valid = true;
        memory->object_memory[memory->objects_in_memory].type = OT_PROGRAM;
        memory->object_memory[memory->objects_in_memory].data.program = (struct Program) {
            .program_id = memory->object_memory[0].data.raw_model.program_id,
            .vertex_path = "shader.vs",
            .fragment_path = "shader.fs",
            .vertex_id = GL.CreateShader(GL_VERTEX_SHADER),
            .fragment_id = GL.CreateShader(GL_FRAGMENT_SHADER),
        };
        ++memory->objects_in_memory;

        memory->initialized = true;
    }
    if (JUST_PRESSED(K_Q)) {
        *running = false;
    }

    if (JUST_PRESSED(K_W)) {
        draw_mode = (draw_mode + 1) % 3;
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT + draw_mode);
    }

    glClearColor(0.0f, 0.0f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    for (size_t i = 0; i < memory->objects_in_memory; ++i) {
        if (memory->object_memory[i].valid) {
            switch (memory->object_memory[i].type) {
                case OT_RAWMODEL: {
                    GL.UseProgram(memory->object_memory[i].data.raw_model.program_id);
                    GL.BindVertexArray(memory->object_memory[i].data.raw_model.VAO);
                    GL.EnableVertexAttribArray(0);
                    glDrawElements(GL_TRIANGLES, memory->object_memory[i].data.raw_model.vertex_count, GL_UNSIGNED_INT, 0);
                    GL.DisableVertexAttribArray(0);
                    GL.BindVertexArray(0);
                } break;
                case OT_PROGRAM: {
                    if (!memory->object_memory[i].data.program.committed && memory->object_memory[i].data.program.loaded) {
                        GL.ShaderSource(memory->object_memory[i].data.program.vertex_id, 1, (const char **) &memory->object_memory[i].data.program.vertex_src, 0);
                        GL.CompileShader(memory->object_memory[i].data.program.vertex_id);
                        int success;
                        char infolog[512];
                        GL.GetShaderiv(memory->object_memory[i].data.program.vertex_id, GL_COMPILE_STATUS, &success);
                        if (!success) {
                            GL.GetShaderInfoLog(memory->object_memory[i].data.program.vertex_id, 512, 0, infolog);
                            printf("error: %s\n", infolog);
                        }

                        GL.ShaderSource(memory->object_memory[i].data.program.fragment_id, 1, (const char **) &memory->object_memory[i].data.program.fragment_src, 0);
                        GL.CompileShader(memory->object_memory[i].data.program.fragment_id);
                        GL.GetShaderiv(memory->object_memory[i].data.program.fragment_id, GL_COMPILE_STATUS, &success);
                        if (!success) {
                            GL.GetShaderInfoLog(memory->object_memory[i].data.program.fragment_id, 512, 0, infolog);
                            printf("error: %s\n", infolog);
                        }

                        GL.AttachShader(memory->object_memory[i].data.program.program_id, memory->object_memory[i].data.program.vertex_id);
                        GL.AttachShader(memory->object_memory[i].data.program.program_id, memory->object_memory[i].data.program.fragment_id);
                        GL.LinkProgram(memory->object_memory[i].data.program.program_id);
                        GL.ValidateProgram(memory->object_memory[i].data.program.program_id);
                        GL.GetProgramiv(memory->object_memory[i].data.program.program_id, GL_LINK_STATUS, &success);
                        if (!success) {
                            GL.GetProgramInfoLog(memory->object_memory[i].data.program.program_id, 512, NULL, infolog);
                            printf("error: %s\n", infolog);
                        }

                        GL.DeleteShader(memory->object_memory[i].data.program.vertex_id);
                        GL.DeleteShader(memory->object_memory[i].data.program.fragment_id);

                        memory->object_memory[i].data.program.committed = true;
                    }
                } break;
            }
        }
    }
}
