#ifndef FMATH_H
#define FMATH_H

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define CLAMP(V, LOW, HIGH) MIN(MAX((V), (LOW)), (HIGH))
#define PI 3.14159265358979323846f
#define RAD(D) ((D)*(PI/180.0f))
#define DEG(R) ((R)*(180.0f/PI))
#define v2(X, Y) (V2) {X, Y}
#define v2f(F) (V2) {F, F}
#define V2_ZERO (V2) {0.0f, 0.0f}
#define V2_ONE (V2) {1.0f, 1.0f}
#define v3(X, Y, Z) (V3) {X, Y, Z}
#define v3f(F) (V3) {F, F, F}
#define v3v2(V) (V3) {(V).x, (V).y, 0.0f}
#define V3_ZERO (V3) {0.0f, 0.0f, 0.0f}
#define V3_ONE (V3) {1.0f, 1.0f, 1.0f}
#define v4(X, Y, Z, W) (V4) {X, Y, Z, W}
#define v4f(F) (V4) {F, F, F, F}
#define V4_ZERO (V4) {0.0f, 0.0f, 0.0f, 0.0f}
#define V4_ONE (V4) {1.0f, 1.0f, 1.0f, 1.0f}
#define m4p(M) (const float *) (M.m)
#define m4pp(M) (const float *) (M->m)
#define M4_ZERO (M4) {{{0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}}}
#define M4_IDENTITY (M4) {{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}}

typedef struct V2 {
    float x, y;
} V2;

typedef struct V3 {
    float x, y, z;
} V3;

typedef struct V4 {
    float x, y, z, w;
} V4;

typedef struct M4 {
    float m[4][4];
} M4;

V2 v2sub(V2 a, V2 b);
V2 v2mul(V2 a, float b);
V2 v2div(V2 a, float b);
V2 v2mulv2(V2 a, V2 b);

V3 v3add(V3 a, V3 b);
V3 v3sub(V3 a, V3 b);
V3 v3neg(V3 v);
V3 v3mul(V3 a, float b);
V3 v3div(V3 a, float b);
float v3dot(V3 a, V3 b);
float v3length(V3 v);
V3 v3normalized(V3 v);
V3 v3flatten(V3 v);
V3 v3cross(V3 a, V3 b);
M4 m4mul(M4 a, M4 b);
V4 m4mulv4(M4 a, V4 b);
M4 m4trans(M4 m, V3 translation);
M4 m4rotx(M4 m, float angle);
M4 m4roty(M4 m, float angle);
M4 m4rotz(M4 m, float angle);
M4 m4rot(M4 m, V3 rotation);
M4 m4scale(M4 m, V3 scale);
M4 m4world(V3 position, V3 rotation, V3 scale);
M4 m4perspective(float fovy, float aspect_ratio, float znear, float zfar);
M4 m4ortho(float left, float right, float bottom, float top, float znear, float zfar);
M4 m4lookat(V3 eye, V3 center, V3 up);

#ifdef INCLUDE_SRC

#include <math.h>

V2 v2sub(V2 a, V2 b)
{
    V2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

V2 v2mul(V2 a, float b)
{
    V2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

V2 v2div(V2 a, float b)
{
    if (b == 0.0f) {
        return V2_ZERO;
    }
    b = 1.0f / b;
    V2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

V2 v2mulv2(V2 a, V2 b)
{
    V2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

V3 v3add(V3 a, V3 b)
{
    V3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

V3 v3sub(V3 a, V3 b)
{
    V3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

V3 v3neg(V3 v)
{
    V3 result;
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

V3 v3mul(V3 a, float b)
{
    V3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

V3 v3div(V3 a, float b)
{
    if (b == 0.0f) {
        return V3_ZERO;
    }
    b = 1.0f / b;
    V3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

float v3dot(V3 a, V3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float v3length(V3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

V3 v3normalized(V3 v)
{
    return v3div(v, v3length(v));
}

V3 v3flatten(V3 v)
{
    V3 result;
    result.x = v.x;
    result.y = 0.0f;
    result.z = v.z;
    return result;
}

V3 v3cross(V3 a, V3 b)
{
    V3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

M4 m4mul(M4 a, M4 b)
{
    M4 result;
    result.m[0][0] = a.m[0][0] * b.m[0][0] + a.m[0][1] * b.m[1][0] + a.m[0][2] * b.m[2][0] + a.m[0][3] * b.m[3][0];
    result.m[0][1] = a.m[0][0] * b.m[0][1] + a.m[0][1] * b.m[1][1] + a.m[0][2] * b.m[2][1] + a.m[0][3] * b.m[3][1];
    result.m[0][2] = a.m[0][0] * b.m[0][2] + a.m[0][1] * b.m[1][2] + a.m[0][2] * b.m[2][2] + a.m[0][3] * b.m[3][2];
    result.m[0][3] = a.m[0][0] * b.m[0][3] + a.m[0][1] * b.m[1][3] + a.m[0][2] * b.m[2][3] + a.m[0][3] * b.m[3][3];
    result.m[1][0] = a.m[1][0] * b.m[0][0] + a.m[1][1] * b.m[1][0] + a.m[1][2] * b.m[2][0] + a.m[1][3] * b.m[3][0];
    result.m[1][1] = a.m[1][0] * b.m[0][1] + a.m[1][1] * b.m[1][1] + a.m[1][2] * b.m[2][1] + a.m[1][3] * b.m[3][1];
    result.m[1][2] = a.m[1][0] * b.m[0][2] + a.m[1][1] * b.m[1][2] + a.m[1][2] * b.m[2][2] + a.m[1][3] * b.m[3][2];
    result.m[1][3] = a.m[1][0] * b.m[0][3] + a.m[1][1] * b.m[1][3] + a.m[1][2] * b.m[2][3] + a.m[1][3] * b.m[3][3];
    result.m[2][0] = a.m[2][0] * b.m[0][0] + a.m[2][1] * b.m[1][0] + a.m[2][2] * b.m[2][0] + a.m[2][3] * b.m[3][0];
    result.m[2][1] = a.m[2][0] * b.m[0][1] + a.m[2][1] * b.m[1][1] + a.m[2][2] * b.m[2][1] + a.m[2][3] * b.m[3][1];
    result.m[2][2] = a.m[2][0] * b.m[0][2] + a.m[2][1] * b.m[1][2] + a.m[2][2] * b.m[2][2] + a.m[2][3] * b.m[3][2];
    result.m[2][3] = a.m[2][0] * b.m[0][3] + a.m[2][1] * b.m[1][3] + a.m[2][2] * b.m[2][3] + a.m[2][3] * b.m[3][3];
    result.m[3][0] = a.m[3][0] * b.m[0][0] + a.m[3][1] * b.m[1][0] + a.m[3][2] * b.m[2][0] + a.m[3][3] * b.m[3][0];
    result.m[3][1] = a.m[3][0] * b.m[0][1] + a.m[3][1] * b.m[1][1] + a.m[3][2] * b.m[2][1] + a.m[3][3] * b.m[3][1];
    result.m[3][2] = a.m[3][0] * b.m[0][2] + a.m[3][1] * b.m[1][2] + a.m[3][2] * b.m[2][2] + a.m[3][3] * b.m[3][2];
    result.m[3][3] = a.m[3][0] * b.m[0][3] + a.m[3][1] * b.m[1][3] + a.m[3][2] * b.m[2][3] + a.m[3][3] * b.m[3][3];
    return result;
}

V4 m4mulv4(M4 a, V4 b)
{
    V4 result;
    result.x = a.m[0][0] * b.x + a.m[0][1] * b.y + a.m[0][2] * b.z + a.m[0][3] * b.w;
    result.y = a.m[1][0] * b.x + a.m[1][1] * b.y + a.m[1][2] * b.z + a.m[1][3] * b.w;
    result.z = a.m[2][0] * b.x + a.m[2][1] * b.y + a.m[2][2] * b.z + a.m[2][3] * b.w;
    result.w = a.m[3][0] * b.x + a.m[3][1] * b.y + a.m[3][2] * b.z + a.m[3][3] * b.w;
    return result;
}

M4 m4trans(M4 m, V3 translation)
{
    M4 result = M4_IDENTITY;
    result.m[0][3] = translation.x;
    result.m[1][3] = translation.y;
    result.m[2][3] = translation.z;
    return m4mul(result, m);
}

M4 m4rotx(M4 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    M4 result = M4_IDENTITY;
    result.m[1][1] = c;
    result.m[1][2] = -s;
    result.m[2][1] = s;
    result.m[2][2] = c;
    return m4mul(result, m);
}

M4 m4roty(M4 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    M4 result = M4_IDENTITY;
    result.m[0][0] = c;
    result.m[0][2] = s;
    result.m[2][0] = -s;
    result.m[2][2] = c;
    return m4mul(result, m);
}

M4 m4rotz(M4 m, float angle)
{
    const float c = cosf(angle);
    const float s = sinf(angle);
    M4 result = M4_IDENTITY;
    result.m[0][0] = c;
    result.m[0][1] = -s;
    result.m[1][0] = s;
    result.m[1][1] = c;
    return m4mul(result, m);
}

M4 m4rot(M4 m, V3 rotation)
{
    M4 result = M4_IDENTITY;
    result = m4roty(result, rotation.y);
    result = m4rotx(result, rotation.x);
    result = m4rotz(result, rotation.z);
    return m4mul(result, m);
}

M4 m4scale(M4 m, V3 scale)
{
    M4 result = M4_IDENTITY;
    result.m[0][0] = scale.x;
    result.m[1][1] = scale.y;
    result.m[2][2] = scale.z;
    return m4mul(result, m);
}

M4 m4world(V3 position, V3 rotation, V3 scale)
{
    M4 result = m4trans(m4rot(m4scale(M4_IDENTITY, scale), rotation), position);
    return result;
}

M4 m4perspective(float fovy, float aspect_ratio, float znear, float zfar)
{
    M4 result = M4_ZERO;
    const float y_scale = 1.0f / tanf(fovy / 2.0f);
    const float x_scale = y_scale / aspect_ratio;
    const float frustum_length = zfar - znear;

    result.m[0][0] = x_scale;
    result.m[1][1] = y_scale;
    result.m[2][2] = -((zfar + znear) / frustum_length);
    result.m[2][3] = -((2 * znear * zfar) / frustum_length);
    result.m[3][2] = -1;
    return result;
}

M4 m4ortho(float left, float right, float bottom, float top, float znear, float zfar)
{
    const float depth = zfar - znear;
    const float width = right - left;
    const float height = top - bottom;

    M4 result = M4_IDENTITY;
    result.m[0][0] = 2.0f / width;
    result.m[1][1] = 2.0f / height;
    result.m[2][2] = -2.0f / depth;
    result.m[0][3] = -(right + left) / width;
    result.m[1][3] = -(top + bottom) / height;
    result.m[2][3] = -(zfar + znear) / depth;
    return result;
}

M4 m4lookat(V3 eye, V3 center, V3 up)
{
    V3 direction = v3normalized(v3sub(eye, center));
    V3 right = v3normalized(v3cross(up, direction));
    up = v3cross(direction, right);
    M4 result = M4_IDENTITY;
    result.m[0][0] = right.x;
    result.m[0][1] = right.y;
    result.m[0][2] = right.z;
    result.m[0][3] = -v3dot(right, eye);
    result.m[1][0] = up.x;
    result.m[1][1] = up.y;
    result.m[1][2] = up.z;
    result.m[1][3] = -v3dot(up, eye);
    result.m[2][0] = direction.x;
    result.m[2][1] = direction.y;
    result.m[2][2] = direction.z;
    result.m[2][3] = -v3dot(direction, eye);
    return result;
}

#endif /* INCLUDE_SRC */

#endif /* FMATH_H */
