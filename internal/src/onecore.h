/* onecore.h - v0.0.1 - public domain, initial release 2026-4-16
 * 
 * MIT License
 *
 * Copyright (c) 2026 Nedas Pranskūnas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef INCLUDE_ONECORE_H
#define INCLUDE_ONECORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t oc_tag;
typedef uint32_t oc_load_flags;
typedef int32_t  oc_16p16;
typedef int32_t  oc_26p6;

#ifndef OCDEF
#define OCDEF
#endif

#define OC_LOAD_DEFAULT 0x0          /* load scaled and fitted metrics */
#define OC_LOAD_NO_SCALE (1l << 0)   /* use font units directly */
#define OC_LOAD_NO_HINTING (1l << 1) /* disable hinting (does nothing for now) */
// todo (stage 2): add these flags
// #define OC_LOAD_VERTICAL (1l << 2)
// #define OC_LOAD_COLOR (1l << 3)
#define OC_LOAD_NO_FITTING (1l << 4) /* disable grid-fitting for 26.6 pixels */

// todo: handle FT_Err_Invalid_Table
// todo: make so if a person is lazy to handle errors
//       every func should just noop and never crash

#define OC_ERROR_LIST                                      \
    X(oc_error_ok, "no error")                             \
    X(oc_error_invalid_param, "invalid parameter")         \
    X(oc_error_table_missing, "table is missing")          \
    X(oc_error_out_of_memory, "out of memory")             \
    X(oc_error_failed_to_open, "failed to open")           \
    X(oc_error_invalid_pixel_size, "invalid pixel size")   \
    X(oc_error_unexpected, "unexpected error")

/* Converts four-letter tags that are used to label TrueType tables. */
#define OC_MAKE_TAG(x1, x2, x3, x4) \
    (((uint32_t)(uint8_t)x1) << 24 | ((uint32_t)(uint8_t)x2) << 16 | ((uint32_t)(uint8_t)x3) << 8 | ((uint32_t)(uint8_t)x4))

#define OC_26P6_FLOOR(x) ((int32_t)(x) & ~63)
#define OC_26P6_ROUND(x) OC_26P6_FLOOR((int32_t)(x) + 32)
#define OC_26P6_CEIL(x) OC_26P6_FLOOR((int32_t)(x) + 63)
#define OC_26P6_ADD(a, b) (int32_t)((uint32_t)(a) + (uint32_t)(b))
#define OC_26P6_SUB(a, b) (int32_t)((uint32_t)(a) - (uint32_t)(b))

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef enum {
#define X(e, s) e,
    OC_ERROR_LIST
#undef X
} oc_error;

typedef enum {
    oc_slant_roman,
    oc_slant_italic,
    oc_slant_oblique,
} oc_slant;

typedef struct {
    uint32_t face_index;   /* index of the face in the font file */
    oc_26p6  desired_size; /* nominal height in 26.6 pixels (default 12 * 64) */
    uint16_t dpi;          /* resolution in dpi (default 72) */
} oc_open_params;

typedef struct {
    uint16_t ppem;  /* pixels per EM */
    oc_16p16 scale; /* scaling value used to convert font units to 26.6 pixels */
} oc_size;

typedef struct {
    oc_26p6 width;     /* glyph's width */
    oc_26p6 height;    /* glyph's height */
    oc_26p6 bearing_x; /* left side bearing */
    oc_26p6 bearing_y; /* top side bearing */
    oc_26p6 advance;   /* advance width */
} oc_glyph_metrics;

// size_metrics, ocl_get_size_metrics

typedef struct {
    uint32_t rows; /* number of extent rows */
    uint32_t cols; /* number of pixels in extent row */
} oc_extent;

typedef struct {
    oc_26p6 min_x; /* horizontal minimum (left-most) */
    oc_26p6 min_y; /* vertical minimum (bottom-most) */
    oc_26p6 max_x; /* horizontal maximum (right-most) */
    oc_26p6 max_y; /* vertical maximum (top-most) */
} oc_bbox;

typedef struct {
    oc_26p6 x;
    oc_26p6 y;
} oc_point;

typedef void (*oc_outline_start_figure)(oc_point at, void* user);
typedef void (*oc_outline_end_figure)(void* user);
typedef void (*oc_outline_line_to)(oc_point to, void* user);
typedef void (*oc_outline_cubic_to)(oc_point c1, oc_point c2, oc_point to, void* user);

typedef struct {
    oc_outline_start_figure start_figure; /* new figure emitter */
    oc_outline_end_figure   end_figure;   /* figure end emitter */
    oc_outline_line_to      line_to;      /* segment emitter */
    oc_outline_cubic_to     cubic_to;     /* third-order bezier arc emitter */
} oc_outline_funcs;

typedef struct oc_face_impl       oc_face_impl;
typedef struct oc_collection_impl oc_collection_impl;
typedef struct oc_library         oc_library;

// todo (stage 2): integrate even more fields
// todo (stage 2): need a way to know how many glyphs a font has
typedef struct {
    const char* family;
    oc_slant    slant;
    uint16_t    weight;
    // -> langs
    // -> monoscope
} oc_font;

typedef struct {
    oc_face_impl* impl;

    oc_size  size;                /* current active size */
    uint16_t upem;                /* units per EM */
    uint16_t ascent;              /* typographic ascender in font units. */
    uint16_t descent;             /* typographic descender in font units. */
    int16_t  leading;             /* typographic leading in font units. */
    int16_t  underline_position;  /* underline position in font units */
    uint16_t underline_thickness; /* underline thickness in font units */
} oc_face;

typedef struct {
    oc_collection_impl* impl;

    oc_font** fonts;  /* discovered fonts list */
    uint32_t  nfonts; /* number of discovered fonts */
} oc_collection;

/*
 * Initializes a new onecore library instance.
 * Call `oc_free_library` to release retrieved resource.
 */
OCDEF oc_error
oc_init_library(oc_library** olibrary);

/*
 * Releases given library object.
 */
OCDEF void
oc_free_library(oc_library* library);

/*
 * Initializes a new onecore collection instance.
 * Call `oc_free_collection` to release retrieved resource.
 */
OCDEF oc_error
ocf_init_collection(const oc_library* library, oc_collection* ocollection);

/*
 * Releases given collection object.
 */
OCDEF void
ocf_free_collection(oc_collection* collection);

/*
 * Loads the list of available system fonts into the collection.
 *
 * Note this function is not thread-safe.
 */
OCDEF oc_error
ocf_load_fonts(oc_collection* collection);

/*
 * Determines whether the font supports a specified character.
 */
OCDEF bool
ocf_has_character(const oc_font* font, uint32_t character);

/*
 * Copies the font's path into client memory.
 * Passing `length` as 0 will exit immediately and return
 * the full path length.
 *
 * Note on dwrite the path is uppercase.
 */
OCDEF size_t
ocf_copy_path(const oc_font* font, char* buffer, size_t length);

/*
 * Opens a font.
 * Call `ocl_free_face` to release retrieved resource.
 * Passing `desired_size` or `dpi` as 0 will use the defaults.
 */
OCDEF oc_error
ocf_open_font(
    const oc_font* font,
    oc_26p6        desired_size,
    uint16_t       dpi,
    oc_face*       oface);

/*
 * Opens a font by its pathname.
 * Call `ocl_free_face` to release retrieved resource.
 * Passing `uparams` as 0 will use the defaults.
 */
OCDEF oc_error
ocl_open_face(
    const oc_library*     library,
    const char*           path,
    const oc_open_params* uparams,
    oc_face*              oface);

/*
 * Opens a font that has been loaded into memory.
 * Call `ocl_free_face` to release retrieved resource.
 * Passing `uparams` as 0 will use the defaults.
 *
 * Note the caller still owns the memory
 * do not deallocate it before calling `ocl_free_face`.
 */
OCDEF oc_error
ocl_open_memory_face(
    const oc_library*     library,
    const void*           data,
    size_t                data_size,
    const oc_open_params* uparams,
    oc_face*              oface);

/*
 * Releases given face object.
 */
OCDEF void
ocl_free_face(oc_face* face);

/*
 * Resizes the scale of the active size object in a face.
 * Passing `desired_size` or `dpi` as 0 will use the defaults.
 *
 * Note the resulting ppem value for the given resolution is always rounded.
 * Note this function is not thread-safe.
 */
OCDEF oc_error
ocl_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi);

/*
 * Returns the glyph index of a given character code.
 */
OCDEF uint16_t
ocl_get_char_index(const oc_face* face, uint32_t charcode);

/*
 * Returns the metrics of a given glyph.
 */
OCDEF void
ocl_get_glyph_metrics(
    const oc_face*    face,
    uint16_t          index,
    oc_load_flags     flags,
    oc_glyph_metrics* ometrics);

/*
 * Returns the control box of a given glyph.
 */
OCDEF void
ocl_get_glyph_cbox(
    const oc_face* face,
    uint16_t       index,
    oc_load_flags  flags,
    oc_bbox*       ocbox);

// when we will have rendering implemented we will not need to store the size of current face
// it would be better to have some `size_t` that will habe ppem, scale, ascent, descent
// this way one face will have multiple sizes when rasterizing a glyph we could just pass that &size
// or to get glyph metrics we can pass that size again and nil could be initial base size
/* 
ocl_open_face(library, path, NULL, &face);
ocl_init_size(&face, 12 << 6, 96, &size);

// it would even be thread-safe though it looks cursed and does not solve the main problem
// we want to be able to render any glyph by its index and size lock free and without mem allocations
ocl_render_glyph(&face, &size, index, &extent, buffer, pitch);
*/

// todo: update outline logic and enable hinting for them

// todo (stage 2): now we're rendering these glyphs from [0;0] position which is convenient, but it does lose some extra draw data
//       make so an user could specify how to draw this glyph mb allow to pass matricies and origins mb just some flags??
// todo (stage 2): it is needed to make this method more complicated, now we cannot pass origin where to draw or matricies, nothing
//
// roadmap:
// dwrite and coretext knows how to draw bezier curves hence theoretically hinting can be achieved with manual shapes rasterization,
// essentially onecore would become freetype, but with native font file parsing and rendering engine
/*
 * Rasterizes a glyph into the pixel buffer.
 *
 * Note passing `buffer` as 0 will exit immediately after setting `oextent`.
 * Note each backend may produce different pixel data for the glyph.
 */
OCDEF oc_error
ocl_render_glyph(
    const oc_face* face,
    uint16_t       index,
    oc_extent*     oextent,
    uint8_t*       buffer,
    size_t         pitch);

// todo (stage 2): add hori kerning support
// OCDEF oc_26p6
// ocl_get_kerning(const oc_face* face, uint16_t li, uint16_t ri, some_flags...);

// todo (stage 2): renew this impl

/*
 * Walk over an outline's structure to decompose it into individual
 * segments and bezier arcs.
 */
OCDEF bool
ocl_get_outline(
    const oc_face*          face,
    uint16_t                index,
    oc_load_flags           flags,
    const oc_outline_funcs* funcs,
    void*                   user);

OCDEF void
ocl_print_raw_outline(
    const oc_face* face,
    uint16_t       index);

/*
 * Loads any SFNT font table into client memory.
 *
 * Note passing `*size` as 0 will exit immediately while returning the
 * table's full size in it.
 */
OCDEF oc_error
ocl_get_sfnt_table(
    const oc_face* face,
    oc_tag         tag,
    uint32_t       offset,
    void*          data,
    uint32_t*      size);

/*
 * Retrieves the description of a valid onecore error.
 */
OCDEF const char*
oc_strerror(oc_error err);

/*
 * Computes `(a*b)/0x10000` with maximum accuracy.
 */
OCDEF oc_16p16
oc_mul_16p16(oc_16p16 a, oc_16p16 b);

/*
 * Computes `(a*0x10000)/b` with maximum accuracy.
 */
OCDEF oc_16p16
oc_div_16p16(oc_16p16 a, oc_16p16 b);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* INCLUDE_ONECORE_H */

/******************************************************************************************************/
/*                                                                                                    */
/*                                           IMPLEMENTATION                                           */
/*                                                                                                    */
/******************************************************************************************************/

#ifdef ONECORE_LOADER_IMPLEMENTATION
#if defined(_MSC_VER) || defined(__MINGW32__)
#define ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION
#elif defined(__APPLE__)
#define ONECORE_CORETEXT_LOADER_IMPLEMENTATION
#else
#define ONECORE_FREETYPE_LOADER_IMPLEMENTATION
#endif
#endif /* ONECORE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_FINDER_IMPLEMENTATION
#if defined(_MSC_VER) || defined(__MINGW32__)
#define ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
#elif defined(__APPLE__)
#define ONECORE_CORETEXT_FINDER_IMPLEMENTATION
#else
#define ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION
#endif
#endif /* ONECORE_FINDER_IMPLEMENTATION */

#if defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION) || defined(ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION) || defined(ONECORE_CORETEXT_LOADER_IMPLEMENTATION) || defined(ONECORE_CORETEXT_FINDER_IMPLEMENTATION) || defined(ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION) || defined(ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION)
#define ONECORE_IMPLEMENTATION
#endif

#ifdef ONECORE_IMPLEMENTATION
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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
#define oc__free(arr)      ((void) ((arr) ? free(oc__head(arr)) : (void)0), (arr)=NULL)
#define oc__len(arr)       ((arr) ? oc__head(arr)->len : 0)
#define oc__cap(arr)       ((arr) ? oc__head(arr)->cap : 0)
#define oc__grow(arr, cap) oc__grow_impl(arr, sizeof(*arr), cap)

#define oc__append(arr, val) \
    ((arr = oc__grow(arr, oc__len(arr) + 1)), \
     (arr) ? ((arr)[oc__head(arr)->len++] = (val), 1) : 0)

// note: this impl will not preserve arr on oom
static inline void* oc__grow_impl(void* arr, size_t size, size_t new_cap) {
    void* new_arr = NULL;

    size_t len = oc__len(arr);
    size_t cap = oc__cap(arr);

    if (cap >= new_cap) {
        return arr;
    }

    size_t init_cap = OC__MAX(1, 128 / size);
    new_cap += new_cap / 2 + init_cap;

    // note: we ignore alignment for now as for basic types it will be fine
    //       though when we will use structs with bigger allignment then 16, we should update
    oc__array* new_head = malloc(sizeof(*new_head) + new_cap * size);
    if (new_head) {
        new_arr = new_head + 1;

        new_head->len = len;
        new_head->cap = new_cap;

        memcpy(new_arr, arr, len * size);

        oc__free(arr);
        return new_head + 1;
    }

    return arr;
}

static inline bool oc__is_midpoint(oc_point pt, oc_point a, oc_point b) {
    uint64_t sumx = (uint64_t)a.x + (uint64_t)b.x;
    uint64_t sumy = (uint64_t)a.y + (uint64_t)b.y;
    return (uint64_t)pt.x * 2 == sumx && (uint64_t)pt.y * 2 == sumy;
}

static inline bool oc__points_equal(oc_point a, oc_point b) {
    return a.x == b.x && a.y == b.y;
}

static inline oc_point oc__point_bsr(oc_point pt, uint8_t amt) {
    pt.x >>= amt;
    pt.y >>= amt;
    return pt;
}
#endif /* ONECORE_IMPLEMENTATION */

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
/// ONECORE_FREETYPE_LOADER_IMPLEMENTATION ///
#endif /* ONECORE_FREETYPE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION
/// ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION ///
#endif /* ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_LOADER_IMPLEMENTATION
/// ONECORE_CORETEXT_LOADER_IMPLEMENTATION ///
#endif /* ONECORE_CORETEXT_LOADER_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_FINDER_IMPLEMENTATION
/// ONECORE_CORETEXT_FINDER_IMPLEMENTATION ///
#endif /* ONECORE_CORETEXT_LINDER_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION
/// ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION ///
#endif /* ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
/// ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION ///
#endif /* ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION */

#if defined(ONECORE_IMPLEMENTATION) && !defined(OC__OVERRIDE_LIBRARY_IMPL)
#ifndef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
static void* oc__noop_library;
#endif
oc_error oc_init_library(oc_library** olibrary) {
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    FT_Error   ft_err;
    FT_Library ft_library;
#endif
    if (!olibrary) {
        return oc_error_invalid_param;
    }
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    ft_err = FT_Init_FreeType(&ft_library);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        *olibrary = NULL;
        return oc_error_out_of_memory;
    default:
        *olibrary = NULL;
        return oc__unexpected(ft_err);
    }
    *olibrary = (oc_library*)ft_library;
#else
    *olibrary = (oc_library*)&oc__noop_library;
#endif
    return oc_error_ok;
}

void oc_free_library(oc_library* library) {
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    FT_Library ft_library;

    if (!library) {
        return;
    }

    ft_library = (FT_Library)library;
    FT_Done_FreeType(ft_library);
#else
    (void)library;
#endif
}
#endif
