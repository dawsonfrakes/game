/* slow but understandable implementation of vector/matrix math */
#ifndef FMATH_H
#define FMATH_H

#define V4FMT "%.6f %.6f %.6f %.6f"
#define V4EXT(V) x(V), y(V), z(V), w(V)
#define V4PTR(V) V.data
#define v4(X, Y, Z, W) (V4) {{X, Y, Z, W}}
#define x(V) V.data[0]
#define y(V) V.data[1]
#define z(V) V.data[2]
#define w(V) V.data[3]
#define m(M, ROW, COL) M.data[ROW][COL]

typedef struct V3 {
    float data[3];
} __attribute__((packed)) V3;

typedef struct V4 {
    float data[4];
} __attribute__((packed)) V4;

typedef struct M4 {
    float data[4][4];
} __attribute__((packed)) M4;

V4 v4add(V4 a, V4 b);
V4 v4sub(V4 a, V4 b);
V4 v4mul(V4 a, float b);
V4 v4div(V4 a, float b);
float v4dot(V4 a, V4 b);
float v4len(V4 v);
float v4len2(V4 v);
V4 m4mulv4(M4 a, V4 b);
M4 m4mul(M4 a, M4 b);

extern const M4 IDENTITY;

#ifdef INCLUDE_SRC

#include <math.h>

const M4 IDENTITY = {{
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1},
}};

V4 v4add(V4 a, V4 b)
{
    V4 result;
    x(result) = x(a) + x(b);
    y(result) = y(a) + y(b);
    z(result) = z(a) + z(b);
    w(result) = w(a) + w(b);
    return result;
}

V4 v4sub(V4 a, V4 b)
{
    V4 result;
    x(result) = x(a) - x(b);
    y(result) = y(a) - y(b);
    z(result) = z(a) - z(b);
    w(result) = w(a) - w(b);
    return result;
}

V4 v4mul(V4 a, float b)
{
    V4 result;
    x(result) = x(a) * b;
    y(result) = y(a) * b;
    z(result) = z(a) * b;
    w(result) = w(a) * b;
    return result;
}

// multiplying 4 times by the reciprocal is cheaper than dividing 4 times (probably)
V4 v4div(V4 a, float b)
{
    V4 result;
    b = 1.0f / b;
    x(result) = x(a) * b;
    y(result) = y(a) * b;
    z(result) = z(a) * b;
    w(result) = w(a) * b;
    return result;
}

float v4dot(V4 a, V4 b)
{
    return x(a) * x(b) + y(a) * y(b) + z(a) * z(b) + w(a) * w(b);
}

float v4len(V4 v)
{
    return sqrtf(x(v) * x(v) + y(v) * y(v) + z(v) * z(v) + w(v) * w(v));
}

// length^2 is useful for comparing two values without a needless (and slow) sqrt
float v4len2(V4 v)
{
    return x(v) * x(v) + y(v) * y(v) + z(v) * z(v) + w(v) * w(v);
}

/*
[a00, a01, a02, a03]   [b.x]   [a00 * b.x + a01 * b.y + a02 * b.z + a03 * b.w]
[a10, a11, a12, a13] * [b.y] = [a10 * b.x + a11 * b.y + a12 * b.z + a13 * b.w]
[a20, a21, a22, a23]   [b.z]   [a20 * b.x + a21 * b.y + a22 * b.z + a23 * b.w]
[a30, a31, a32, a33]   [b.w]   [a30 * b.x + a31 * b.y + a32 * b.z + a33 * b.w]
*/
V4 m4mulv4(M4 a, V4 b)
{
    V4 result;
    x(result) = m(a, 0, 0) * x(b) + m(a, 0, 1) * y(b) + m(a, 0, 2) * z(b) + m(a, 0, 3) * w(b);
    y(result) = m(a, 1, 0) * x(b) + m(a, 1, 1) * y(b) + m(a, 1, 2) * z(b) + m(a, 1, 3) * w(b);
    z(result) = m(a, 2, 0) * x(b) + m(a, 2, 1) * y(b) + m(a, 2, 2) * z(b) + m(a, 2, 3) * w(b);
    w(result) = m(a, 3, 0) * x(b) + m(a, 3, 1) * y(b) + m(a, 3, 2) * z(b) + m(a, 3, 3) * w(b);
    return result;
}

/*
[a00, a01, a02, a03]   [b00, b01, b02, b03]   [a00 * b00 + a01 * b10 + a02 * b20 + a03 * b30, a00 * b01 + a01 * b11 + a02 * b21 + a03 * b31, a00 * b02 + a01 * b12 + a02 * b22 + a03 * b32, a00 * b03 + a01 * b13 + a02 * b23 + a03 * b33]
[a10, a11, a12, a13] * [b10, b11, b12, b13] = [a10 * b00 + a11 * b10 + a12 * b20 + a13 * b30, a10 * b01 + a11 * b11 + a12 * b21 + a13 * b31, a10 * b02 + a11 * b12 + a12 * b22 + a13 * b32, a10 * b03 + a11 * b13 + a12 * b23 + a13 * b33]
[a20, a21, a22, a23]   [b20, b21, b22, b23]   [a20 * b00 + a21 * b10 + a22 * b20 + a23 * b30, a20 * b01 + a21 * b11 + a22 * b21 + a23 * b31, a20 * b02 + a21 * b12 + a22 * b22 + a23 * b32, a20 * b03 + a21 * b13 + a22 * b23 + a23 * b33]
[a30, a31, a32, a33]   [b30, b31, b32, b33]   [a30 * b00 + a31 * b10 + a32 * b20 + a33 * b30, a30 * b01 + a31 * b11 + a32 * b21 + a33 * b31, a30 * b02 + a31 * b12 + a32 * b22 + a33 * b32, a30 * b03 + a31 * b13 + a32 * b23 + a33 * b33]
*/
M4 m4mul(M4 a, M4 b)
{
    M4 result;
    m(result, 0, 0) = m(a, 0, 0) * m(b, 0, 0) + m(a, 0, 1) * m(b, 1, 0) + m(a, 0, 2) * m(b, 2, 0) + m(a, 0, 3) * m(b, 3, 0);
    m(result, 0, 1) = m(a, 0, 0) * m(b, 0, 1) + m(a, 0, 1) * m(b, 1, 1) + m(a, 0, 2) * m(b, 2, 1) + m(a, 0, 3) * m(b, 3, 1);
    m(result, 0, 2) = m(a, 0, 0) * m(b, 0, 2) + m(a, 0, 1) * m(b, 1, 2) + m(a, 0, 2) * m(b, 2, 2) + m(a, 0, 3) * m(b, 3, 2);
    m(result, 0, 3) = m(a, 0, 0) * m(b, 0, 3) + m(a, 0, 1) * m(b, 1, 3) + m(a, 0, 2) * m(b, 2, 3) + m(a, 0, 3) * m(b, 3, 3);
    m(result, 1, 0) = m(a, 1, 0) * m(b, 0, 0) + m(a, 1, 1) * m(b, 1, 0) + m(a, 1, 2) * m(b, 2, 0) + m(a, 1, 3) * m(b, 3, 0);
    m(result, 1, 1) = m(a, 1, 0) * m(b, 0, 1) + m(a, 1, 1) * m(b, 1, 1) + m(a, 1, 2) * m(b, 2, 1) + m(a, 1, 3) * m(b, 3, 1);
    m(result, 1, 2) = m(a, 1, 0) * m(b, 0, 2) + m(a, 1, 1) * m(b, 1, 2) + m(a, 1, 2) * m(b, 2, 2) + m(a, 1, 3) * m(b, 3, 2);
    m(result, 1, 3) = m(a, 1, 0) * m(b, 0, 3) + m(a, 1, 1) * m(b, 1, 3) + m(a, 1, 2) * m(b, 2, 3) + m(a, 1, 3) * m(b, 3, 3);
    m(result, 2, 0) = m(a, 2, 0) * m(b, 0, 0) + m(a, 2, 1) * m(b, 1, 0) + m(a, 2, 2) * m(b, 2, 0) + m(a, 2, 3) * m(b, 3, 0);
    m(result, 2, 1) = m(a, 2, 0) * m(b, 0, 1) + m(a, 2, 1) * m(b, 1, 1) + m(a, 2, 2) * m(b, 2, 1) + m(a, 2, 3) * m(b, 3, 1);
    m(result, 2, 2) = m(a, 2, 0) * m(b, 0, 2) + m(a, 2, 1) * m(b, 1, 2) + m(a, 2, 2) * m(b, 2, 2) + m(a, 2, 3) * m(b, 3, 2);
    m(result, 2, 3) = m(a, 2, 0) * m(b, 0, 3) + m(a, 2, 1) * m(b, 1, 3) + m(a, 2, 2) * m(b, 2, 3) + m(a, 2, 3) * m(b, 3, 3);
    m(result, 3, 0) = m(a, 3, 0) * m(b, 0, 0) + m(a, 3, 1) * m(b, 1, 0) + m(a, 3, 2) * m(b, 2, 0) + m(a, 3, 3) * m(b, 3, 0);
    m(result, 3, 1) = m(a, 3, 0) * m(b, 0, 1) + m(a, 3, 1) * m(b, 1, 1) + m(a, 3, 2) * m(b, 2, 1) + m(a, 3, 3) * m(b, 3, 1);
    m(result, 3, 2) = m(a, 3, 0) * m(b, 0, 2) + m(a, 3, 1) * m(b, 1, 2) + m(a, 3, 2) * m(b, 2, 2) + m(a, 3, 3) * m(b, 3, 2);
    m(result, 3, 3) = m(a, 3, 0) * m(b, 0, 3) + m(a, 3, 1) * m(b, 1, 3) + m(a, 3, 2) * m(b, 2, 3) + m(a, 3, 3) * m(b, 3, 3);
    return result;
}

#endif /* INCLUDE_SRC */

#endif /* FMATH_H */
