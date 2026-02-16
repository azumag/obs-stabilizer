/******************************************************************************
    Minimal Vector2 Definitions for OBS Plugin Development
    This file provides 2D vector structures for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <math.h>

typedef struct vec2 {
    float x, y;
} vec2;

static inline void vec2_zero(struct vec2 *dst)
{
    dst->x = 0.0f;
    dst->y = 0.0f;
}

static inline void vec2_set(struct vec2 *dst, float x, float y)
{
    dst->x = x;
    dst->y = y;
}

static inline void vec2_copy(struct vec2 *dst, const struct vec2 *v)
{
    dst->x = v->x;
    dst->y = v->y;
}

static inline float vec2_dist(const struct vec2 *v1, const struct vec2 *v2)
{
    float x = v1->x - v2->x;
    float y = v1->y - v2->y;
    return (float)sqrt(x * x + y * y);
}

static inline float vec2_len(const struct vec2 *v)
{
    return (float)sqrt(v->x * v->x + v->y * v->y);
}

static inline void vec2_norm(struct vec2 *dst, const struct vec2 *v)
{
    float len = vec2_len(v);
    if (len > 0.0f) {
        dst->x = v->x / len;
        dst->y = v->y / len;
    } else {
        vec2_zero(dst);
    }
}

static inline void vec2_add(struct vec2 *dst, const struct vec2 *v1,
        const struct vec2 *v2)
{
    dst->x = v1->x + v2->x;
    dst->y = v1->y + v2->y;
}

static inline void vec2_sub(struct vec2 *dst, const struct vec2 *v1,
        const struct vec2 *v2)
{
    dst->x = v1->x - v2->x;
    dst->y = v1->y - v2->y;
}

static inline void vec2_mul(struct vec2 *dst, const struct vec2 *v, float m)
{
    dst->x = v->x * m;
    dst->y = v->y * m;
}

static inline float vec2_dot(const struct vec2 *v1, const struct vec2 *v2)
{
    return v1->x * v2->x + v1->y * v2->y;
}

static inline void vec2_neg(struct vec2 *dst, const struct vec2 *v)
{
    dst->x = -v->x;
    dst->y = -v->y;
}

#ifdef __cplusplus
}
#endif
