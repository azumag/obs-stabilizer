/******************************************************************************
    Minimal Vector3 Definitions for OBS Plugin Development
    This file provides 3D vector structures for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <math.h>

typedef struct vec3 {
    float x, y, z;
} vec3;

static inline void vec3_zero(struct vec3 *dst)
{
    dst->x = 0.0f;
    dst->y = 0.0f;
    dst->z = 0.0f;
}

static inline void vec3_set(struct vec3 *dst, float x, float y, float z)
{
    dst->x = x;
    dst->y = y;
    dst->z = z;
}

static inline void vec3_copy(struct vec3 *dst, const struct vec3 *v)
{
    dst->x = v->x;
    dst->y = v->y;
    dst->z = v->z;
}

static inline float vec3_len(const struct vec3 *v)
{
    return (float)sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

static inline void vec3_norm(struct vec3 *dst, const struct vec3 *v)
{
    float len = vec3_len(v);
    if (len > 0.0f) {
        dst->x = v->x / len;
        dst->y = v->y / len;
        dst->z = v->z / len;
    } else {
        vec3_zero(dst);
    }
}

static inline void vec3_add(struct vec3 *dst, const struct vec3 *v1,
        const struct vec3 *v2)
{
    dst->x = v1->x + v2->x;
    dst->y = v1->y + v2->y;
    dst->z = v1->z + v2->z;
}

static inline void vec3_sub(struct vec3 *dst, const struct vec3 *v1,
        const struct vec3 *v2)
{
    dst->x = v1->x - v2->x;
    dst->y = v1->y - v2->y;
    dst->z = v1->z - v2->z;
}

static inline void vec3_mul(struct vec3 *dst, const struct vec3 *v, float m)
{
    dst->x = v->x * m;
    dst->y = v->y * m;
    dst->z = v->z * m;
}

static inline float vec3_dot(const struct vec3 *v1, const struct vec3 *v2)
{
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

static inline void vec3_neg(struct vec3 *dst, const struct vec3 *v)
{
    dst->x = -v->x;
    dst->y = -v->y;
    dst->z = -v->z;
}

static inline void vec3_cross(struct vec3 *dst, const struct vec3 *v1,
        const struct vec3 *v2)
{
    struct vec3 temp;
    temp.x = v1->y * v2->z - v1->z * v2->y;
    temp.y = v1->z * v2->x - v1->x * v2->z;
    temp.z = v1->x * v2->y - v1->y * v2->x;
    vec3_copy(dst, &temp);
}

#ifdef __cplusplus
}
#endif
