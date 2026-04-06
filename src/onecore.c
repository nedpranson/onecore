#include "onecore.h"
#define ONECORE_FREETYPE_LOADER_IMPLEMENTATION
/* ONECORE_IMPLEMENTATION */
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

#define oc__parentof(type, ptr, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

#define OC__MAX(a, b) \
    ((a) > (b) ? (a) : (b))

#define OC__MIN(a, b) \
    ((a) < (b) ? (a) : (b))

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

/* oc_library:
 * - (freetype; fconfig)  -> (freetype) + -
 * - (freetype; dwrite)   -> (freetype; dwrite) + -
 * - (freetype; coretext) -> (freetype) + -
 * - (dwrite;   dwrite)   -> (dwrite) - -
 * - (coretext; coretext) -> (NULL) - -
 * - (dwrite;   fconfig)  -> (dwrite) - -
 * - (coretext; fconfig)  -> (NULL) - -
 */

// #if defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION)
// tood: move this s out of here
// #elif defined(ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION)
// ...
// #else
// static char oc__noop_library;
// oc_error oc_init_library(oc_library** olibrary) {
//     if (!olibrary) {
//         return oc_error_invalid_param;
//     }
//
//     *olibrary = (oc_library*)&oc__noop_library;
//     return oc_error_ok;
// }
//
// void oc_free_library(oc_library* library) {
//     (void)library;
// }
// #endif
