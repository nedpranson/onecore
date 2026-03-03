#include "onecore.h"

/* ONECORE_IMPLEMENTATION */
#ifndef OC_ASSERT
#include <assert.h>
#define OC_ASSERT(x) assert(x)
#endif /* OC_ASSERT */

// todo: move oc_mutex_impl_t to freetype impl

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
typedef SRWLOCK oc_mutex_impl_t;

#define oc_mutex_impl_init(m) InitializeSRWLock(m)
#define oc_mutex_impl_lock(m) AcquireSRWLockExclusive(m)
#define oc_mutex_impl_unlock(m) ReleaseSRWLockExclusive(m)
#define oc_mutex_impl_destroy(m) ((void)0)
#else
#include <pthread.h>
typedef pthread_mutex_t oc_mutex_impl_t;

#define oc_mutex_impl_init(m) pthread_mutex_init(m, NULL)
#define oc_mutex_impl_lock(m) pthread_mutex_lock(m)
#define oc_mutex_impl_unlock(m) pthread_mutex_unlock(m)
#define oc_mutex_impl_destroy(m) pthread_mutex_destroy(m)
#endif /* defined(_MSC_VER) || defined(__MINGW32__) */

// todo: implement this!
#define oc_unexpected(e) oc_error_unexpected

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

#define MOVE_SIGN(utype, ix, ux, s) \
    do {                            \
        if (ix < 0) {               \
            ux = 0U - (utype)ix;    \
            s = !s;                 \
        } else {                    \
            ux = (utype)ix;         \
        }                           \
    } while (0)

oc_16p16 oc_div_16p16(oc_16p16 a, oc_16p16 b) {
    bool s = false;
    uint64_t ua, ub, uq;
    oc_16p16 q;

    MOVE_SIGN(uint64_t, a, ua, s);
    MOVE_SIGN(uint64_t, b, ub, s);

    uq = ub > 0 ? ((ua << 16) + (ub >> 1)) / ub : 0x7FFFFFFFUL;
    q = (int32_t)uq;

    return s ? (0U - (uint32_t)q) : q;
}

oc_16p16 oc_mul_16p16(oc_16p16 a, oc_16p16 b) {
    int64_t ab = (uint64_t)a * (uint64_t)b;
    return (int32_t)((ab + 0x8000L + (ab >> 63)) >> 16);
}

// todo: rename to oc__
static inline oc_open_params oc_open_params_defaults(const oc_open_params* pparams) {
    if (pparams == NULL)
        return (oc_open_params) {
            .face_index = 0,
            .desired_size = 12 << 6,
            .dpi = 96,
        };

    oc_open_params params = *pparams;

    if (params.desired_size <= 0) {
        params.desired_size = 12 << 6;
    }

    if (params.dpi <= 0) {
        params.dpi = 96;
    }

    return params;
}
