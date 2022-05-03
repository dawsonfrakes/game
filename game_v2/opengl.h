#ifndef OPENGL_H
#define OPENGL_H

#include <GL/gl.h>

#define GL_GAME_FUNCTIONS \
    /*  ret, name, params */ \
    GL_GAME_EXPAND(void, GenBuffers, GLsizei n, GLuint *buffers) \
    GL_GAME_EXPAND(void, BindBuffer, GLenum target, GLuint buffer) \
    GL_GAME_EXPAND(void, BufferData, GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) \
    GL_GAME_EXPAND(void, GenVertexArrays, GLsizei n, GLuint *arrays) \
    GL_GAME_EXPAND(void, BindVertexArray, GLuint array) \
    /* (void, BindAttribLocation, GLuint program, GLuint index, const GLchar *name) */ \
    GL_GAME_EXPAND(GLuint, CreateShader, GLenum shaderType) \
    GL_GAME_EXPAND(void, DeleteShader, GLuint shader) \
    GL_GAME_EXPAND(void, ShaderSource, GLuint shader, GLsizei count, const GLchar **string, const GLint *length) \
    GL_GAME_EXPAND(void, CompileShader, GLuint shader) \
    GL_GAME_EXPAND(GLuint, CreateProgram, void) \
    GL_GAME_EXPAND(void, AttachShader, GLuint program, GLuint shader) \
    GL_GAME_EXPAND(void, LinkProgram, GLuint program) \
    GL_GAME_EXPAND(void, UseProgram, GLuint program) \
    GL_GAME_EXPAND(void, VertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) \
    GL_GAME_EXPAND(void, EnableVertexAttribArray, GLuint index) \
    GL_GAME_EXPAND(void, DisableVertexAttribArray, GLuint index) \
    GL_GAME_EXPAND(void, GetShaderiv, GLuint shader, GLenum pname, GLint *params) \
    GL_GAME_EXPAND(void, GetShaderInfoLog, GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog) \
    GL_GAME_EXPAND(void, GetProgramiv, GLuint program, GLenum pname, GLint *params) \
    GL_GAME_EXPAND(void, GetProgramInfoLog, GLuint program, GLsizei maxLength, GLsizei *length, GLchar *info) \
    GL_GAME_EXPAND(void, ValidateProgram, GLuint program) \
    GL_GAME_EXPAND(GLint, GetUniformLocation, GLuint program, const GLchar *name) \
    GL_GAME_EXPAND(void, Uniform4f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) \
    GL_GAME_EXPAND(void, UniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)

struct OpenGL {
    #define GL_GAME_EXPAND(RET, NAME, ...) RET (*NAME)(__VA_ARGS__);
    GL_GAME_FUNCTIONS
    #undef GL_GAME_EXPAND
};

extern struct OpenGL GL;

#ifdef INCLUDE_SRC

struct OpenGL GL = {0};

#endif /* INCLUDE_SRC */

#endif /* OPENGL_H */
