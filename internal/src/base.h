/*
 * @fuse
 * @base BASE
 */
#include "../onecore.h"
/* ---- */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef NDEBUG
#define oc__unexpected(e) oc_error_unexpected
#else
#include <stdio.h>
static inline oc_error oc__unexpected_impl(long err, const char* file, int line) {
    fprintf(stderr, "%s:%d: unexpected error: %ld\n", file, line, err);
    return oc_error_unexpected;
}
#define oc__unexpected(e) oc__unexpected_impl((long)e, __FILE__, __LINE__)
#endif /* NDEBUG */

#define oc__parentof(type, ptr, member) ((type*)((char*)(ptr) - offsetof(type, member)))

#define OC__MAX(a, b) ((a) > (b) ? (a) : (b))
#define OC__MIN(a, b) ((a) < (b) ? (a) : (b))

#define OC__CURVE_TAG_ON    0x1
#define OC__CURVE_TAG_CONIC 0x0
#define OC__CURVE_TAG_CUBIC 0x2

#if defined(__GNUC__) || defined(__clang__)
#define oc__likely(x)   __builtin_expect(!!(x), 1)
#define oc__unlikely(x) __builtin_expect(!!(x), 0)
#else
#define oc__likely(x)   (!!(x))
#define oc__unlikely(x) (!!(x))
#endif

#define oc__exit(e) \
    do {            \
        err = (e);  \
        goto exit;  \
    } while (0)

#define OC__MOVE_SIGN(utype, ix, ux, s) \
    do {                                \
        if (ix < 0) {                   \
            ux = 0U - (utype)ix;        \
            s = !s;                     \
        } else {                        \
            ux = (utype)ix;             \
        }                               \
    } while (0)

const char* oc_strerror(oc_error err) {
#ifdef ONECORE_NO_ERROR_STRINGS
    return NULL;
#else
    switch (err) {
#define X(e, s) \
    case e:     \
        return s;
        OC_ERROR_LIST
#undef X
    default:
        return "unknown error";
    }
#endif /* ONECORE_NO_ERROR_STRINGS */
}

oc_16p16 oc_div_16p16(oc_16p16 a, oc_16p16 b) {
    bool     s = false;
    uint64_t ua, ub, uq;
    oc_16p16 q;

    OC__MOVE_SIGN(uint64_t, a, ua, s);
    OC__MOVE_SIGN(uint64_t, b, ub, s);

    uq = ub > 0 ? ((ua << 16) + (ub >> 1)) / ub : 0x7FFFFFFFUL;
    q = (int32_t)uq;

    return s ? (0U - (uint32_t)q) : q;
}

oc_16p16 oc_mul_16p16(oc_16p16 a, oc_16p16 b) {
    int64_t ab = (uint64_t)a * (uint64_t)b;
    return (int32_t)((ab + 0x8000L + (ab >> 63)) >> 16);
}

static inline oc_open_params oc__open_params_defaults(const oc_open_params* uparams) {
    oc_open_params params = { 0 };

    if (uparams != NULL) {
        params = *uparams;
    }

    if (params.desired_size == 0) {
        params.desired_size = 12 << 6;
    } else if (params.desired_size < 1 << 6) {
        params.desired_size = 1 << 6;
    }

    if (params.dpi == 0) {
        params.dpi = 72;
    }

    return params;
}

static inline void oc__fit_metrics(oc_glyph_metrics* pmetrics) {
    oc_26p6 right = OC_26P6_CEIL(OC_26P6_ADD(pmetrics->bearing_x, pmetrics->width));
    oc_26p6 bottom = OC_26P6_FLOOR(OC_26P6_SUB(pmetrics->bearing_y, pmetrics->height));

    pmetrics->bearing_x = OC_26P6_FLOOR(pmetrics->bearing_x);
    pmetrics->bearing_y = OC_26P6_CEIL(pmetrics->bearing_y);

    pmetrics->width = OC_26P6_SUB(right, pmetrics->bearing_x);
    pmetrics->height = OC_26P6_SUB(pmetrics->bearing_y, bottom);

    pmetrics->advance = OC_26P6_ROUND(pmetrics->advance);
}

typedef struct {
    size_t len;
    size_t cap;
} oc__array;

#define oc__head(arr)      ((oc__array*)(arr) - 1)
#define oc__make(type)     ((type)NULL)
#define oc__free(arr)      ((void)((arr) ? free(oc__head(arr)) : (void)0), (arr) = NULL)
#define oc__len(arr)       ((arr) ? oc__head(arr)->len : 0)
#define oc__cap(arr)       ((arr) ? oc__head(arr)->cap : 0)
#define oc__grow(arr, cap) oc__grow_impl(arr, sizeof(*arr), cap)

#define oc__reserve(arr, count) \
    (oc__cap(arr) >= oc__len(arr) + (count) || \
        ((arr) = oc__grow(arr, oc__len(arr) + (count))) != NULL)

#define oc__append(arr, val)                  \
    ((arr = oc__grow(arr, oc__len(arr) + 1)), \
        (arr) ? ((arr)[oc__head(arr)->len++] = (val), 1) : 0)

// note: this impl will not preserve arr on oom
static inline void* oc__grow_impl(void* arr, size_t size, size_t new_cap) {
    size_t len = oc__len(arr);
    size_t cap = oc__cap(arr);

    if (cap >= new_cap) {
        return arr;
    }

    size_t init_cap = OC__MAX(1, 128 / size);
    new_cap += new_cap / 2 + init_cap;

    // note: we ignore alignment for now as for basic types it will be fine
    //       though when we will use structs with bigger allignment then 16, we should update
    oc__array* new_head = (oc__array*)malloc(sizeof(*new_head) + new_cap * size);
    if (new_head) {
        void* new_arr = new_head + 1;

        new_head->len = len;
        new_head->cap = new_cap;

        if (arr) {
            memcpy(new_arr, arr, len * size);
            oc__free(arr);
        }
        return new_arr;
    }

    oc__free(arr);
    return NULL;
}

static inline bool oc__is_midpoint(oc_point pt, oc_point a, oc_point b) {
    uint64_t sumx = (uint64_t)a.x + (uint64_t)b.x;
    uint64_t sumy = (uint64_t)a.y + (uint64_t)b.y;
    return (uint64_t)pt.x * 2 == sumx && (uint64_t)pt.y * 2 == sumy;
}

static inline bool oc__points_equal(oc_point a, oc_point b) {
    return a.x == b.x && a.y == b.y;
}

static inline uint16_t u16_from_be(const void* data) {
    const uint8_t* p = (const uint8_t*)data;
    return ((uint16_t)p[0] << 8) |
           ((uint16_t)p[1]);
}

static inline int16_t i16_from_be(const void* data) {
    return (int16_t)u16_from_be(data);
}

static inline uint32_t u32_from_be(const void* data) {
    const uint8_t* p = (const uint8_t*)data;
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |
           ((uint32_t)p[3]);
}
static inline int32_t i32_from_be(const void* data) {
    return (int32_t)u32_from_be(data);
}
