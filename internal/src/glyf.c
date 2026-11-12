/*
 * @fuse
 * @base GLYF
 * @deps
 *  - BASE
 */
#include "base.h"
/* ---- */

// note: internal for now; this will move into onecore.h when integrated.
typedef struct oc__glyf_font {
    const uint8_t* head;
    const uint8_t* loca;
    uint32_t       loca_size;
    const uint8_t* glyf;
    uint32_t       glyf_size;
    uint16_t       num_glyphs;
} oc__glyf_font;

// todo: add a test for ensuring max_depth is respected between backends
#define OC__GLYF_MAX_DEPTH 64

// note: onecore does not define the glyf point/composite flag bits yet.
enum {
    OC__GLYF_REPEAT              = 0x0008,
    OC__GLYF_ARGS_ARE_WORDS      = 0x0001,
    OC__GLYF_WE_HAVE_A_SCALE     = 0x0008,
    OC__GLYF_MORE_COMPONENTS     = 0x0020,
    OC__GLYF_WE_HAVE_AN_XY_SCALE = 0x0040,
    OC__GLYF_WE_HAVE_A_2X2       = 0x0080,
};

// Replicates FreeType's tt_face_get_location() sanitization.
static bool oc__glyf_range(const oc__glyf_font* font, uint16_t index, uint32_t* ooffset, uint32_t* olength) {
    int16_t  format;
    uint32_t pos1;
    uint32_t pos2;

    if (!font || !font->head || !font->loca || !font->glyf) {
        return false;
    }

    format = i16_from_be(font->head + 50);

    if (format == 0) {
        if ((size_t)(index + 1u) * 2u + 2u > font->loca_size) {
            return false;
        }
        pos1 = (uint32_t)u16_from_be(font->loca + (size_t)index * 2) * 2u;
        pos2 = (uint32_t)u16_from_be(font->loca + (size_t)(index + 1u) * 2) * 2u;
    } else if (format == 1) {
        if ((size_t)(index + 1u) * 4u + 4u > font->loca_size) {
            return false;
        }
        pos1 = u32_from_be(font->loca + (size_t)index * 4);
        pos2 = u32_from_be(font->loca + (size_t)(index + 1u) * 4);
    } else {
        return false;
    }

    if (pos1 > font->glyf_size) {
        *ooffset = 0;
        *olength = 0;
        return true;
    }

    if (pos2 > font->glyf_size) {
        if ((uint32_t)index + 1u == (uint32_t)font->num_glyphs) {
            pos2 = font->glyf_size;
        } else {
            *ooffset = 0;
            *olength = 0;
            return true;
        }
    }

    *ooffset = pos1;
    *olength = (pos2 >= pos1) ? (pos2 - pos1) : (font->glyf_size - pos1);
    return true;
}

// Appends one simple glyph's tags. Contour data is not validated.
static bool oc__glyf_load_simple(uint8_t** tags, const uint8_t* p, const uint8_t* end, int16_t ncontours) {
    const uint8_t* q;
    size_t         npoints;
    size_t         i;
    size_t         base;
    uint16_t       last;
    uint16_t       ilen;

    if (ncontours >= 0xFFF) {
        return false;
    }

    // endPtsOfContours + instructionLength
    if ((size_t)(end - p) < (size_t)ncontours * 2 + 2) {
        return false;
    }

    // only the final endpoint matters: it gives the point count
    last = u16_from_be(p + (size_t)(ncontours - 1) * 2);
    npoints = (size_t)last + 1;

    // instructionLength + instructions
    q = p + (size_t)ncontours * 2;
    ilen = u16_from_be(q);
    q += 2;
    if ((size_t)(end - q) < ilen) {
        return false;
    }
    q += ilen;

    base = oc__len(*tags);
    if (!oc__reserve(*tags, npoints)) {
        return false;
    }

    // point flags, REPEAT-aware; only the on-curve bit is kept
    i = 0;
    while (i < npoints) {
        uint8_t flag;
        size_t  count;

        if (q >= end) {
            return false;
        }

        flag = *q++;
        count = 1;

        if (flag & OC__GLYF_REPEAT) {
            if (q >= end) {
                return false;
            }
            count += *q++;
        }

        if (i + count > npoints) {
            return false;
        }

        while (count-- > 0) {
            (*tags)[base + i++] = flag & OC__CURVE_TAG_ON;
        }
    }

    oc__head(*tags)->len = base + npoints;
    return true;
}

static bool oc__glyf_load_glyph(const oc__glyf_font* font, uint16_t index, uint8_t** tags, uint16_t* stack, uint32_t depth);

// Appends every component of a composite glyph.
static bool oc__glyf_load_composite(const oc__glyf_font* font, const uint8_t* p, const uint8_t* end, uint8_t** tags, uint16_t* stack, uint32_t depth) {
    uint16_t flags;

    do {
        uint16_t component;
        size_t   skip;

        if ((size_t)(end - p) < 4) {
            return false;
        }

        flags = u16_from_be(p);
        component = u16_from_be(p + 2);
        p += 4;

        if (component >= font->num_glyphs) {
            return false;
        }

        // arguments: 2 bytes, or 4 with ARGS_ARE_WORDS
        skip = (flags & OC__GLYF_ARGS_ARE_WORDS) ? 4 : 2;

        // optional 2x2 / scale / xy-scale transform
        if (flags & OC__GLYF_WE_HAVE_A_SCALE) {
            skip += 2;
        } else if (flags & OC__GLYF_WE_HAVE_AN_XY_SCALE) {
            skip += 4;
        } else if (flags & OC__GLYF_WE_HAVE_A_2X2) {
            skip += 8;
        }

        if ((size_t)(end - p) < skip) {
            return false;
        }
        p += skip;

        if (!oc__glyf_load_glyph(font, component, tags, stack, depth)) {
            return false;
        }

    } while (flags & OC__GLYF_MORE_COMPONENTS);

    return true;
}

static bool oc__glyf_load_glyph(const oc__glyf_font* font, uint16_t index, uint8_t** tags, uint16_t* stack, uint32_t depth) {
    uint32_t       offset;
    uint32_t       length;
    const uint8_t* p;
    const uint8_t* end;
    int16_t        ncontours;
    uint32_t       i;

    if (depth >= OC__GLYF_MAX_DEPTH) {
        return false;
    }

    // reject cyclic composites
    for (i = 0; i < depth; i++) {
        if (stack[i] == index) {
            return false;
        }
    }
    stack[depth] = index;

    if (!oc__glyf_range(font, index, &offset, &length)) {
        return false;
    }
    if (length == 0) {
        return true; // empty glyph
    }
    if (length < 10) {
        return false;
    }

    p = font->glyf + offset;
    end = p + length;

    ncontours = i16_from_be(p);
    p += 10; // numberOfContours + bounding box

    if (ncontours == 0) {
        return true; // empty glyph
    }
    if (ncontours > 0) {
        return oc__glyf_load_simple(tags, p, end, ncontours);
    }
    return oc__glyf_load_composite(font, p, end, tags, stack, depth + 1);
}

// Loads the point tags for a simple or composite glyph.
// On success *otags is an oc__array of uint8_t: 0 = conic/off-curve,
// 1 = on-curve. Use oc__len(*otags) for the point count and oc__free()
// to release it. Empty glyphs succeed with *otags == NULL.
// On failure returns false and sets *otags to NULL.
static bool oc__glyf_load_tags(const oc__glyf_font* font, uint16_t index, uint8_t** otags) {
    uint16_t stack[OC__GLYF_MAX_DEPTH];
    uint8_t* tags = oc__make(uint8_t*);

    if (!font || !otags) {
        return false;
    }

    *otags = NULL;

    if (index >= font->num_glyphs) {
        return false;
    }

    if (!oc__glyf_load_glyph(font, index, &tags, stack, 0)) {
        oc__free(tags);
        return false;
    }

    *otags = tags;
    return true;
}
