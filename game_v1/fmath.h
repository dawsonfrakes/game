#ifndef FMATH_H
#define FMATH_H

#include <math.h>

#define v2(X, Y) (V2) {X, Y}
#define V2_ZERO (V2) {0.0f, 0.0f}

typedef struct V2 {
    float x, y;
} V2;

V2 v2add(V2 a, V2 b);
V2 v2mul(V2 a, float b);
V2 v2div(V2 a, float b);
float v2len(V2 v);
V2 v2nrm(V2 v);

#ifdef INCLUDE_SRC

V2 v2add(V2 a, V2 b)
{
    return (V2) {a.x + b.x, a.y + b.y};
}

V2 v2mul(V2 a, float b)
{
    return (V2) {a.x * b, a.y * b};
}

V2 v2div(V2 a, float b)
{
    const float one_over_b = 1.0f / b;
    return (V2) {a.x * one_over_b, a.y * one_over_b};
}

float v2len(V2 v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

V2 v2nrm(V2 v)
{
    const float magnitude = v2len(v);
    if (magnitude == 0.0f) {
        return V2_ZERO;
    }
    return v2div(v, magnitude);
}

#endif /* INClUDE_SRC */

#endif /* FMATH_H */
