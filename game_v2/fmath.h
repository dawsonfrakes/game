#ifndef FMATH_H
#define FMATH_H

#include <math.h>

#define PI 3.14159265358979323846f

#define RAD(D) (D*(PI/180.0f))
#define DEG(R) (R*(180.0f/PI))

#define v3(X, Y, Z) (V3) {X, Y, Z}
#define v3f(F) (V3) {F, F, F}
#define V3_ZERO (V3) {0.0f, 0.0f, 0.0f}

#define v4(X, Y, Z, W) (V4) {X, Y, Z, W}
#define v4f(F) (V4) {F, F, F, F}
#define V4_ZERO (V4) {0.0f, 0.0f, 0.0f, 0.0f}

#define m4p(M) (const float *) (M.m)
#define M4_IDENTITY (M4) {{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}}}

typedef struct V3 {
    float x, y, z;
} V3;

typedef struct V4 {
    float x, y, z, w;
} V4;

typedef struct M4 {
    float m[4][4];
} M4;

M4 m4mul(M4 a, M4 b);
V4 m4mulv4(M4 a, V4 b);
M4 m4trans(M4 m, V3 translation);
M4 m4rotx(M4 m, float angle);
M4 m4roty(M4 m, float angle);
M4 m4rotz(M4 m, float angle);
M4 m4rot(M4 m, V3 rotation);
M4 m4scale(M4 m, V3 scale);

#ifdef INCLUDE_SRC

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

#endif /* INCLUDE_SRC */

#endif /* FMATH_H */
