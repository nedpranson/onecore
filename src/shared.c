#include "shared.h"

// todo: in header just #define oc_div_16p16 FT_DivFix

#ifdef ONECORE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#else /* !ONECORE_FREETYPE */

#define MOVE_SIGN(utype, ix, ux, s) \
    do {                            \
        if (ix < 0) {               \
            ux = 0U - (utype)ix;    \
            s = !s;                 \
        } else {                    \
            ux = (utype)ix;         \
        }                           \
    } while (0)

#endif /* !ONECORE_FREETYPE */

// https://github.com/freetype/freetype/blob/85c8efe0afa5ad0df35114e317a065f544943c52/src/base/ftcalc.c#L233
oc_16p16 oc_div_16p16(oc_16p16 a, oc_16p16 b) {
#ifdef ONECORE_FREETYPE
    return FT_DivFix(a, b);
#else /* !ONECORE_FREETYPE */
    bool s = false;
    uint64_t ua, ub, uq;
    oc_16p16 q;

    MOVE_SIGN(uint64_t, a, ua, s);
    MOVE_SIGN(uint64_t, b, ub, s);

    uq = ub > 0 ? ((ua << 16) + (ub >> 1)) / ub : 0x7FFFFFFFUL;
    q = (int32_t)uq;

    return s ? (0U - (uint32_t)q) : q;
#endif /* !ONECORE_FREETYPE */
}

oc_16p16 oc_mul_16p16(oc_16p16 a, oc_16p16 b) {
#ifdef ONECORE_FREETYPE
    return FT_MulFix(a, b);
#else /* !ONECORE_FREETYPE */
    int64_t ab = (uint64_t)a * (uint64_t)b;
    return (int32_t)((ab + 0x8000L + (ab >> 63)) >> 16);
#endif /* !ONECORE_FREETYPE */
}
