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

#define OC_LOAD_DEFAULT    0x0       /* load scaled and fitted metrics */
#define OC_LOAD_NO_SCALE   (1l << 0) /* use font units directly */
#define OC_LOAD_NO_HINTING (1l << 1) /* disable hinting (does nothing for now) */
// todo (stage 2): add these flags
// #define OC_LOAD_VERTICAL (1l << 2)
// #define OC_LOAD_COLOR (1l << 3)
#define OC_LOAD_NO_FITTING (1l << 4) /* disable grid-fitting for 26.6 pixels */

// todo: handle FT_Err_Invalid_Table
// todo: make so if a person is lazy to handle errors
//       every func should just noop and never crash

#define OC_ERROR_LIST                                    \
    X(oc_error_ok, "no error")                           \
    X(oc_error_invalid_param, "invalid parameter")       \
    X(oc_error_table_missing, "table is missing")        \
    X(oc_error_out_of_memory, "out of memory")           \
    X(oc_error_failed_to_open, "failed to open")         \
    X(oc_error_invalid_pixel_size, "invalid pixel size") \
    X(oc_error_unexpected, "unexpected error")

/* Converts four-letter tags that are used to label TrueType tables. */
#define OC_MAKE_TAG(x1, x2, x3, x4) \
    (((uint32_t)(uint8_t)x1) << 24 | ((uint32_t)(uint8_t)x2) << 16 | ((uint32_t)(uint8_t)x3) << 8 | ((uint32_t)(uint8_t)x4))

#define OC_26P6_FLOOR(x)  ((int32_t)(x) & ~63)
#define OC_26P6_ROUND(x)  OC_26P6_FLOOR((int32_t)(x) + 32)
#define OC_26P6_CEIL(x)   OC_26P6_FLOOR((int32_t)(x) + 63)
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

typedef struct {
    uint8_t*  tags;
    oc_point* points;
    uint16_t* contours;
    uint16_t  npoints;
    uint16_t  ncontours;
} oc_outline;

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

// use outline as the glyph, so maybe oc_outline or oc_glyph
// get bbox from outline, get metrics from the outline

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
OCDEF oc_error
ocl_get_outline(
    const oc_face* face,
    uint16_t       index,
    oc_load_flags  flags,
    oc_outline*    ooutline);

OCDEF void
ocl_free_outline(oc_outline* outline);

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

#if defined(__GNUC__) || defined(__clang__)
#define oc__likely(x)   __builtin_expect(!!(x), 1)
#define oc__unlikely(x) __builtin_expect(!!(x), 0)
#define oc__cold(x)     __builtin_expect(!!(x), 0)
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

#define oc__append(arr, val)                  \
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
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>

typedef SRWLOCK oc__mutex_impl_t;
#define oc__mutex_impl_init(m)    InitializeSRWLock(m)
#define oc__mutex_impl_lock(m)    AcquireSRWLockExclusive(m)
#define oc__mutex_impl_unlock(m)  ReleaseSRWLockExclusive(m)
#define oc__mutex_impl_destroy(m) ((void)0)
#else
#include <pthread.h>

typedef pthread_mutex_t oc__mutex_impl_t;
#define oc__mutex_impl_init(m)    pthread_mutex_init(m, NULL)
#define oc__mutex_impl_lock(m)    pthread_mutex_lock(m)
#define oc__mutex_impl_unlock(m)  pthread_mutex_unlock(m)
#define oc__mutex_impl_destroy(m) pthread_mutex_destroy(m)
#endif

#define oc__exit_critical(e)         \
    do {                             \
        oc__mutex_impl_unlock(lock); \
        err = (e);                   \
        goto exit;                   \
    } while (0)

struct oc_face_impl {
    FT_Face          ft_face;
    oc__mutex_impl_t lock;
};

#define OC__OVERRIDE_LIBRARY_IMPL

#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
#include <initguid.h>

#include <dwrite.h>

struct oc_library {
    FT_Library      ft_library;
    IDWriteFactory* dw_factory;
};
#endif

oc_error oc_init_library(oc_library** olibrary) {
    FT_Error   ft_err;
    FT_Library ft_library;
#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
    HRESULT         result;
    IDWriteFactory* dw_factory;
#endif
    oc_error    err = oc_error_ok;
    oc_library* library = NULL;

    if (!olibrary) {
        return oc_error_invalid_param;
    }

    ft_err = FT_Init_FreeType(&ft_library);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(ft_err));
    }
#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&dw_factory);
    switch (result) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        FT_Done_FreeType(ft_library);
        oc__exit(oc_error_out_of_memory);
    default:
        FT_Done_FreeType(ft_library);
        oc__exit(oc__unexpected(result));
    }

    library = malloc(sizeof(*library));
    if (!library) {
        dw_factory->lpVtbl->Release(dw_factory);
        FT_Done_FreeType(ft_library);
        oc__exit(oc_error_out_of_memory);
    }

    library->ft_library = ft_library;
    library->dw_factory = dw_factory;
#else
    library = (oc_library*)ft_library;
#endif
exit:
    *olibrary = library;
    return err;
}

void oc_free_library(oc_library* library) {
    FT_Library ft_library;
#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
    IDWriteFactory* dw_factory;
#endif
    if (!library) {
        return;
    }
#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
    ft_library = library->ft_library;
    dw_factory = library->dw_factory;

    dw_factory->lpVtbl->Release(dw_factory);
    FT_Done_FreeType(ft_library);

    free(library);
#else
    ft_library = (FT_Library)library;
    FT_Done_FreeType(ft_library);
#endif
}

static oc_error oc__init_face(FT_Face ft_face, const oc_open_params* params, oc_face* oface) {
    FT_Error err;
    oc_face  face;

    err = FT_Set_Char_Size(ft_face, 0, params->desired_size, params->dpi, params->dpi);
    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Invalid_Pixel_Size:
        return oc_error_invalid_pixel_size;
    default:
        return oc__unexpected(err);
    }

    face.impl = malloc(sizeof(oc_face_impl));
    if (face.impl == NULL) {
        return oc_error_out_of_memory;
    }

    face.impl->ft_face = ft_face;
    oc__mutex_impl_init(&face.impl->lock);

    face.size.scale = ft_face->size->metrics.y_scale;
    face.size.ppem = ft_face->size->metrics.y_ppem;
    face.upem = ft_face->units_per_EM;
    face.ascent = ft_face->ascender;
    face.descent = -ft_face->descender;
    face.leading = ft_face->height - ft_face->ascender + ft_face->descender;
    // reverting ajusted underline position by freetype
    face.underline_position = ft_face->underline_position + (ft_face->underline_thickness >> 1);
    face.underline_thickness = ft_face->underline_thickness;

    *oface = face;
    return oc_error_ok;
}

oc_error ocl_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    int32_t        err;
    FT_Face        ft_face;
    FT_Library     ft_library;
    oc_open_params params;
    FT_Open_Args   ft_open_args = { 0 };

    if (!(library && path && oface)) {
        return oc_error_invalid_param;
    }
#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
    ft_library = library->ft_library;
#else
    ft_library = (FT_Library)library;
#endif

    ft_open_args.flags = FT_OPEN_PATHNAME;
    ft_open_args.pathname = (FT_String*)path;

    params = oc__open_params_defaults(uparams);

    // using FT_Open_Face as FT_New_Face fails if file extention does not match file type
    err = FT_Open_Face(ft_library, &ft_open_args, params.face_index, &ft_face);
    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    case FT_Err_Cannot_Open_Resource:
    case FT_Err_Invalid_File_Format:
    case FT_Err_Unknown_File_Format:
        return oc_error_failed_to_open;
    case FT_Err_Invalid_Argument:
        return oc_error_invalid_param;
    default:
        return oc__unexpected(err);
    }

    err = oc__init_face(ft_face, &params, oface);
    if (err != oc_error_ok) {
        FT_Done_Face(ft_face);
    }

    return err;
}

oc_error ocl_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    int32_t        err;
    FT_Face        ft_face;
    FT_Library     ft_library;
    oc_open_params params;

    if (!(library && oface)) {
        return oc_error_invalid_param;
    }
#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
    ft_library = library->ft_library;
#else
    ft_library = (FT_Library)library;
#endif
    params = oc__open_params_defaults(uparams);
    err = FT_New_Memory_Face(ft_library, data, size, params.face_index, &ft_face);

    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    case FT_Err_Invalid_Argument:
        return oc_error_invalid_param;
    case FT_Err_Invalid_File_Format:
    case FT_Err_Unknown_File_Format:
    case FT_Err_Invalid_Stream_Operation:
        return oc_error_failed_to_open;
    default:
        return oc__unexpected(err);
    }

    err = oc__init_face(ft_face, &params, oface);
    if (err != oc_error_ok) {
        FT_Done_Face(ft_face);
    }

    return err;
}

void ocl_free_face(oc_face* face) {
    oc_face_impl* impl;
    if (!face) {
        return;
    }

    impl = face->impl;

    FT_Done_Face(impl->ft_face);
    oc__mutex_impl_destroy(&impl->lock);

    free(impl);
    memset(face, 0, sizeof(*face));
}

oc_error ocl_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    FT_Error err;
    FT_Face  ft_face;

    if (!face) {
        return oc_error_invalid_param;
    }

    if (desired_size < 1 << 6) {
        return oc_error_invalid_param;
    }

    ft_face = face->impl->ft_face;
    err = FT_Set_Char_Size(ft_face, 0, desired_size, dpi, dpi);

    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Invalid_Pixel_Size:
        return oc_error_invalid_pixel_size;
    default:
        return oc__unexpected(err);
    }

    face->size.scale = ft_face->size->metrics.y_scale;
    face->size.ppem = ft_face->size->metrics.y_ppem;

    return oc_error_ok;
}

uint16_t ocl_get_char_index(const oc_face* face, uint32_t charcode) {
    return face ? FT_Get_Char_Index(face->impl->ft_face, charcode) : 0;
}

oc_error ocl_get_sfnt_table(const oc_face* face, oc_tag tag, uint32_t offset, void* data, uint32_t* size) {
    FT_Error err;
    FT_Face  ft_face;

    FT_ULong length;

    if (!(face && size)) {
        return oc_error_invalid_param;
    }

    // freetype has two magic tag values:
    //  1 -> raw font file
    //  2 -> SFNT table dir
    if (3 > tag) {
        return oc_error_table_missing;
    }

    ft_face = face->impl->ft_face;
    length = (FT_ULong)*size;

    assert(length == 0 || length >= offset);

    err = FT_Load_Sfnt_Table(
        ft_face,
        (FT_ULong)tag,
        (FT_ULong)offset,
        (FT_Byte*)data,
        &length);

    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Table_Missing:
        return oc_error_table_missing;
    default:
        return oc__unexpected(err);
    }

    assert(UINT32_MAX >= length);
    *size = (uint32_t)length;

    return oc_error_ok;
}

// todo (stage 2): add option for verticals and maybe load both hori and vert bearings, advances
void ocl_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    FT_Error          err;
    FT_Face           ft_face;
    oc__mutex_impl_t* lock;
    FT_Glyph_Metrics  ft_metrics;
    oc_glyph_metrics  metrics = { 0 };
    FT_Int32          ft_load_flags = FT_LOAD_NO_AUTOHINT | FT_LOAD_BITMAP_METRICS_ONLY | FT_LOAD_NO_HINTING;

    if (!(face && ometrics)) {
        goto exit;
    }

    ft_face = face->impl->ft_face;
    lock = &face->impl->lock;

    if (flags & OC_LOAD_NO_SCALE) {
        flags |= OC_LOAD_NO_FITTING;
        ft_load_flags |= FT_LOAD_NO_SCALE;
    }

    // if (flags & OC_LOAD_NO_HINTING) {
    // ft_load_flags |= FT_LOAD_NO_HINTING;
    //}

    oc__mutex_impl_lock(lock);
    err = FT_Load_Glyph(ft_face, index, ft_load_flags);
    if (err != FT_Err_Ok) {
        oc__mutex_impl_unlock(lock);
        goto exit;
    }

    ft_metrics = ft_face->glyph->metrics;
    oc__mutex_impl_unlock(lock);

    metrics.width = ft_metrics.width;
    metrics.height = ft_metrics.height;
    metrics.bearing_x = ft_metrics.horiBearingX;
    metrics.bearing_y = ft_metrics.horiBearingY;
    metrics.advance = ft_metrics.horiAdvance;

    if (flags & OC_LOAD_NO_FITTING) {
        goto exit;
    }

    oc__fit_metrics(&metrics);
exit:
    if (ometrics)
        *ometrics = metrics;
}

void ocl_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    FT_Error          err;
    FT_Face           ft_face;
    oc__mutex_impl_t* lock;
    FT_BBox           ft_cbox;
    oc_bbox           cbox = { 0 };
    FT_Int32          ft_load_flags = FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING;

    if (!(face && ocbox)) {
        goto exit;
    }

    ft_face = face->impl->ft_face;
    lock = &face->impl->lock;

    if (flags & OC_LOAD_NO_SCALE) {
        ft_load_flags |= FT_LOAD_NO_SCALE;
    }

    // if (flags & OC_LOAD_NO_HINTING) {
    // ft_load_flags |= FT_LOAD_NO_HINTING;
    //}

    oc__mutex_impl_lock(lock);
    err = FT_Load_Glyph(ft_face, index, ft_load_flags);
    if (err != FT_Err_Ok) {
        oc__mutex_impl_unlock(lock);
        goto exit;
    }

    FT_Outline_Get_CBox(&ft_face->glyph->outline, &ft_cbox);
    oc__mutex_impl_unlock(lock);

    cbox.min_x = ft_cbox.xMin;
    cbox.min_y = ft_cbox.yMin;
    cbox.max_x = ft_cbox.xMax;
    cbox.max_y = ft_cbox.yMax;
exit:
    if (ocbox)
        *ocbox = cbox;
}

oc_error ocl_get_outline(const oc_face* face, uint16_t index, oc_load_flags flags, oc_outline* ooutline) {
    FT_Error     ft_err;
    FT_Face      ft_face;
    FT_GlyphSlot ft_glyph;
    FT_Outline   ft_outline;

    oc__mutex_impl_t* lock;

    uint8_t* tags = NULL;
    oc_point* points = NULL;
    uint16_t* contours = NULL;

    oc_error   err = oc_error_ok;
    FT_Int32   ft_load_flags = FT_LOAD_NO_BITMAP;
    oc_outline outline = { 0 };

    if (!(face && ooutline)) {
        oc__exit(oc_error_invalid_param);
    }

    ft_face = face->impl->ft_face;
    lock = &face->impl->lock;

    if (flags & OC_LOAD_NO_SCALE) {
        flags |= OC_LOAD_NO_FITTING;
        ft_load_flags |= FT_LOAD_NO_SCALE;
    }

    if (flags & OC_LOAD_NO_HINTING) {
        ft_load_flags |= FT_LOAD_NO_HINTING;
    }

    oc__mutex_impl_lock(lock);
    ft_err = FT_Load_Glyph(ft_face, index, ft_load_flags);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        oc__exit_critical(oc_error_out_of_memory);
    case FT_Err_Invalid_Argument:
        oc__exit_critical(oc_error_invalid_param);
    default:
        oc__exit_critical(oc__unexpected(ft_err));
    }

    ft_glyph = ft_face->glyph;
    ft_outline = ft_glyph->outline;

    // todo: copy glyph to make lock as short as possible
    //       and take in **outline

    if (ft_glyph->format != FT_GLYPH_FORMAT_OUTLINE && ft_glyph->format != FT_GLYPH_FORMAT_COMPOSITE) {
        oc__exit_critical(oc__unexpected(0));
    }

    // todo: compare sizeof src and dst type if equal just copy ptr

    tags = malloc(ft_outline.n_points * sizeof(*tags));
    if (tags == NULL) {
        oc__exit_critical(oc__unexpected(0));
    }

    points = malloc(ft_outline.n_points * sizeof(*points));
    if (points == NULL) {
        oc__exit_critical(oc__unexpected(0));
    }

    contours = malloc(ft_outline.n_contours * sizeof(*contours));
    if (contours == NULL) {
        oc__exit_critical(oc__unexpected(0));
    }

    for (uint16_t i = 0; i < ft_outline.n_points; i++) {
        tags[i] = ft_outline.tags[i];
        points[i] = (oc_point){ ft_outline.points[i].x, ft_outline.points[i].y };
    }

    for (uint16_t i = 0; i < ft_outline.n_contours; i++) {
        contours[i] = ft_outline.contours[i];
    }

    oc__mutex_impl_unlock(lock);

    outline.tags = tags;
    outline.points = points;
    outline.contours = contours;
    outline.ncontours = ft_outline.n_contours;
    outline.npoints = ft_outline.n_points;
exit:
    if (ooutline != NULL)
        *ooutline = outline;

    if (err != oc_error_ok) {
        free(tags);
        free(points);
        free(contours);
    }

    return err;
}

void ocl_free_outline(oc_outline* outline) {
    if (outline == NULL) {
        return;
    }

    free(outline->tags);
    free(outline->points);
    free(outline->contours);

    memset(outline, 0, sizeof(*outline));
}

oc_error ocl_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t pitch) {
    FT_Face           ft_face;
    oc__mutex_impl_t* lock;
    FT_Error          ft_err;
    FT_Bitmap         ft_bitmap;
    FT_Glyph          ft_glyph = NULL;
    oc_error          err = oc_error_ok;
    oc_extent         extent = { 0 };

    if (!(face && oextent)) {
        oc__exit(oc_error_invalid_param);
    }

    ft_face = face->impl->ft_face;
    lock = &face->impl->lock;

    oc__mutex_impl_lock(lock);
    ft_err = FT_Load_Glyph(ft_face, index, FT_LOAD_BITMAP_METRICS_ONLY | FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        oc__exit_critical(oc_error_out_of_memory);
    case FT_Err_Invalid_Argument:
        oc__exit_critical(oc_error_invalid_param);
    default:
        oc__exit_critical(oc__unexpected(ft_err));
    }

    ft_bitmap = ft_face->glyph->bitmap;
    if (ft_bitmap.width != (FT_UInt)ft_bitmap.pitch) {
        // todo (stage 2): implement diffrent types
        oc__exit_critical(oc_error_unexpected);
    }

    extent.rows = ft_bitmap.rows;
    extent.cols = ft_bitmap.width;

    if (buffer == NULL) {
        oc__exit_critical(oc_error_ok);
    }

    if (extent.rows == 0 || extent.cols == 0) {
        oc__exit_critical(oc_error_ok);
    }

    ft_err = FT_Get_Glyph(ft_face->glyph, &ft_glyph);
    oc__mutex_impl_unlock(lock);

    switch (ft_err) {
    case FT_Err_Out_Of_Memory:
        oc__exit(oc_error_out_of_memory);
    case FT_Err_Ok:
        break;
    default:
        oc__exit(oc__unexpected(ft_err));
    }

    ft_err = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, NULL, 1);
    if (ft_err != FT_Err_Ok) {
        oc__exit(oc__unexpected(ft_err));
    }

    assert(((FT_BitmapGlyph)ft_glyph)->bitmap.rows == extent.rows);
    assert(((FT_BitmapGlyph)ft_glyph)->bitmap.width == extent.cols);
    assert((FT_UInt)((FT_BitmapGlyph)ft_glyph)->bitmap.pitch == extent.cols);

    if (extent.cols == pitch) {
        memcpy(buffer, ((FT_BitmapGlyph)ft_glyph)->bitmap.buffer, (size_t)extent.rows * (size_t)extent.cols);
    } else
        for (uint32_t y = 0; y < extent.rows; y++) {
            memcpy(buffer + y * pitch, ((FT_BitmapGlyph)ft_glyph)->bitmap.buffer + y * extent.cols, extent.cols);
        }
exit:
    if (ft_glyph)
        FT_Done_Glyph(ft_glyph);
    if (oextent)
        *oextent = extent;

    return err;
}
#endif /* ONECORE_FREETYPE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION
#include <fontconfig/fontconfig.h>

struct oc_collection_impl {
    const oc_library* oc_library;
    FcConfig*         fc_config;
};

typedef struct {
    const oc_library* oc_library;
    FcPattern*        fc_pattern;
    oc_font           font;
} oc__font_impl;

typedef enum {
    oc__status_ok,
    oc__status_memory,
    oc__status_skip,
} oc__status;

static inline void oc__free_font(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    // impl->fc_pattern should not be freed as it is owned by FcConfig
    free(impl);
}

oc_error ocf_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error            err = oc_error_ok;
    FcConfig*           fc_config;
    oc_collection_impl* impl;
    oc_collection       collection = { 0 };

    if (!(library && ocollection)) {
        oc__exit(oc_error_invalid_param);
    }

    fc_config = FcInitLoadConfig();
    if (fc_config == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        FcConfigDestroy(fc_config);
        oc__exit(oc_error_out_of_memory);
    }

    impl->oc_library = library;
    impl->fc_config = fc_config;

    collection.impl = impl;
exit:
    if (ocollection)
        *ocollection = collection;
    return err;
}

void ocf_free_collection(oc_collection* collection) {
    FcConfig* fc_config;

    if (collection) {
        fc_config = (FcConfig*)collection->impl;

        while (collection->nfonts--) {
            oc__free_font(collection->fonts[collection->nfonts]);
        }
        free(collection->fonts);

        FcConfigDestroy(fc_config);
        free(collection->impl);

        memset(collection, 0, sizeof(*collection));
    }
}

static oc__status oc__init_font(const oc_library* library, FcPattern* fc_pattern, oc_font** ofont) {
    FcResult result;

    FcValue weight_value;

    int weight;
    int slant;

    FcChar8* family;

    oc__font_impl* impl;

    (void)result;

    result = FcPatternGet(fc_pattern, FC_WEIGHT, 0, &weight_value);
    assert(result == FcResultMatch);

    switch (weight_value.type) {
    case FcTypeInteger:
        weight = weight_value.u.i;
        break;
    case FcTypeDouble:
        weight = weight_value.u.d;
        break;
    default:
        return oc__status_skip;
    }

    weight = FcWeightToOpenType(weight);
    assert(weight >= 0 && weight <= UINT16_MAX);

    result = FcPatternGetInteger(fc_pattern, FC_SLANT, 0, &slant);
    assert(result == FcResultMatch);

    result = FcPatternGetString(fc_pattern, FC_FAMILY, 0, &family);
    assert(result == FcResultMatch);
    assert(family != NULL);

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        return oc__status_memory;
    }

    impl->oc_library = library;
    impl->fc_pattern = fc_pattern;
    impl->font.family = (char*)family;
    impl->font.weight = (uint16_t)weight;

    switch (slant) {
    case FC_SLANT_ROMAN:
        impl->font.slant = oc_slant_roman;
        break;
    case FC_SLANT_ITALIC:
        impl->font.slant = oc_slant_italic;
        break;
    case FC_SLANT_OBLIQUE:
        impl->font.slant = oc_slant_oblique;
        break;
    }

    *ofont = &impl->font;
    return oc__status_ok;
}

oc_error ocf_load_fonts(oc_collection* collection) {
    oc_error          err = oc_error_ok;
    const oc_library* oc_library;

    FcConfig*  fc_config;
    FcFontSet* fc_fonts;

    int font_count;

    oc_font** fonts = NULL;
    uint32_t  nfonts = 0;

    oc_collection tmp_collection;

    if (!collection) {
        oc__exit(oc_error_invalid_param);
    }

    oc_library = collection->impl->oc_library;
    fc_config = collection->impl->fc_config;

    if (!FcConfigBuildFonts(fc_config)) {
        // 'FcConfigBuildFonts' returns FcFalse on oom
        oc__exit(oc_error_out_of_memory);
    }

    // 'FcConfigGetFonts' will never return NULL; it can only return an empty 'FcFontSet' object if no fonts are found
    fc_fonts = FcConfigGetFonts(fc_config, FcSetSystem);
    assert(fc_fonts != NULL);

    font_count = fc_fonts->nfont;
    if (font_count == 0) {
        goto done;
    }

    fonts = malloc(font_count * sizeof(*fonts));
    if (fonts == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    for (int i = 0; i < font_count; i++) {
        oc__status status;

        FcPattern* pattern;
        oc_font*   font;

        pattern = fc_fonts->fonts[i];
        status = oc__init_font(oc_library, pattern, &font);

        switch (status) {
        case oc__status_ok:
            assert(font != NULL);
            fonts[nfonts++] = font;
            break;
        case oc__status_memory:
            oc__exit(oc_error_out_of_memory);
        case oc__status_skip:
            break;
        }
    }
done:
    tmp_collection.impl = collection->impl;
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

    fonts = collection->fonts;
    nfonts = collection->nfonts;

    *collection = tmp_collection;
exit:
    while (nfonts--)
        oc__free_font(fonts[nfonts]);
    free(fonts);

    return err;
}

bool ocf_has_character(const oc_font* font, uint32_t character) {
    oc__font_impl* impl;
    FcCharSet*     charset;
    FcResult       result;

    if (!font) {
        return false;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    result = FcPatternGetCharSet(impl->fc_pattern, FC_CHARSET, 0, &charset);

    return result == FcResultMatch && FcCharSetHasChar(charset, character);
}

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
// todo: add support for dwrite and coretext
// todo (stage 2) make face index logic compatible with other backends
// upper 16 means instance
// lower 16 means index
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;

    FcPattern* pattern;
    FcResult   result;

    FcChar8* file;
    int      index;

    oc_open_params params;

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    pattern = impl->fc_pattern;

    result = FcPatternGetString(pattern, FC_FILE, 0, &file);
    if (result != FcResultMatch) {
        return oc__unexpected(result);
    }

    result = FcPatternGetInteger(pattern, FC_INDEX, 0, &index);
    if (result != FcResultMatch) {
        return oc__unexpected(result);
    }

    params.face_index = index;
    params.desired_size = desired_size;
    params.dpi = dpi;

    return ocl_open_face(impl->oc_library, (char*)file, &params, oface);
}
#endif

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    FcResult       result;

    FcChar8* file;
    size_t   file_len;

    size_t copy_len;

    if (!font) {
        return 0;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    result = FcPatternGetString(impl->fc_pattern, FC_FILE, 0, &file);

    if (result != FcResultMatch) {
        return 0;
    }

    file_len = strlen((char*)file);
    copy_len = len < file_len ? len : file_len;

    if (copy_len == 0) {
        return file_len;
    }

    memcpy(buf, file, copy_len);
    return copy_len;
}
#endif /* ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_LOADER_IMPLEMENTATION
#include <CoreText/CoreText.h>

static oc_error oc__init_face(CTFontDescriptorRef descriptor, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    CTFontRef ct_font;
    oc_face   face;

    oc_16p16 scaled;
    CGFloat  size;

    oc_16p16 ppem;
    uint16_t upem;

    scaled = (desired_size * dpi + 36) / 72;
    ct_font = CTFontCreateWithFontDescriptor(descriptor, scaled / 64.0, NULL);

    if (ct_font == NULL) {
        return oc_error_out_of_memory;
    }

    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        CFRelease(ct_font);
        return oc_error_invalid_pixel_size;
    }

    size = CTFontGetSize(ct_font);
    upem = CTFontGetUnitsPerEm(ct_font);

    face.impl = (oc_face_impl*)ct_font;
    face.size.scale = oc_div_16p16(scaled, upem);
    face.size.ppem = (uint16_t)ppem;
    face.upem = upem;
    face.ascent = CTFontGetAscent(ct_font) * upem / size;
    face.descent = CTFontGetDescent(ct_font) * upem / size;
    face.leading = CTFontGetLeading(ct_font) * upem / size;
    face.underline_position = CTFontGetUnderlinePosition(ct_font) * upem / size;
    face.underline_thickness = CTFontGetUnderlineThickness(ct_font) * upem / size;

    *oface = face;
    return oc_error_ok;
}

static oc_error oc__open_face_from_descriptors(CFArrayRef descriptors, const oc_open_params* uparams, oc_face* oface) {
    CTFontDescriptorRef descriptor;

    CFIndex        count = CFArrayGetCount(descriptors);
    oc_open_params params = oc__open_params_defaults(uparams);

    if (count == 0) {
        return oc_error_failed_to_open;
    }

    if (params.face_index >= count) {
        return oc_error_invalid_param;
    }

    descriptor = (CTFontDescriptorRef)CFArrayGetValueAtIndex(descriptors, params.face_index);
    if (descriptor == NULL) {
        return oc_error_out_of_memory;
    }

    return oc__init_face(descriptor, params.desired_size, params.dpi, oface);
}

oc_error ocl_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    CFStringRef ct_path;
    CFURLRef    url_path;

    CFArrayRef descriptors;
    oc_error   err;

    if (!(library && path && oface)) {
        return oc_error_invalid_param;
    }

    // CFURLCreateWithFileSystemPath returns NULL when allocating empty string
    if (*path == '\0') {
        return oc_error_failed_to_open;
    }

    // todo (stage 2): validate utf8 so ct_path would only fail on oom
    ct_path = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    if (ct_path == NULL) {
        return oc_error_failed_to_open; // or oom
    }

    url_path = CFURLCreateWithFileSystemPath(NULL, ct_path, kCFURLPOSIXPathStyle, false);
    CFRelease(ct_path);

    if (url_path == NULL) {
        return oc_error_out_of_memory;
    }

    descriptors = CTFontManagerCreateFontDescriptorsFromURL(url_path);
    CFRelease(url_path);

    // todo (stage 2): think how to reliably handle this err
    if (descriptors == NULL) {
        // file not found
        // invalid file
        // oom
        return oc_error_failed_to_open;
    }

    err = oc__open_face_from_descriptors(descriptors, uparams, oface);
    CFRelease(descriptors);

    return err;
}

oc_error ocl_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    CFDataRef  ct_data;
    CFArrayRef descriptors;
    oc_error   err;

    if (!(library && data && oface)) {
        return oc_error_invalid_param;
    }

    ct_data = CFDataCreateWithBytesNoCopy(NULL, data, size, kCFAllocatorNull);
    if (ct_data == NULL) {
        return oc_error_out_of_memory;
    }

    descriptors = CTFontManagerCreateFontDescriptorsFromData(ct_data);
    CFRelease(ct_data);

    if (descriptors == NULL) {
        // invalid file
        // oom
        return oc_error_failed_to_open;
    }

    err = oc__open_face_from_descriptors(descriptors, uparams, oface);
    CFRelease(descriptors);

    return err;
}

void ocl_free_face(oc_face* face) {
    if (face) {
        CFRelease(face->impl);
        memset(face, 0, sizeof(*face));
    }
}

uint16_t ocl_get_char_index(const oc_face* face, uint32_t charcode) {
    CTFontRef ct_font;

    CGGlyph glyphs[2];
    UniChar chars[2];

    if (!face || charcode > 0x10FFFF) {
        return 0;
    }

    ct_font = (CTFontRef)face->impl;

    // check out CFStringGetSurrogatePairForLongCharacter

    // CTFontGetGlyphsForCharacters writes cg_glyph[1] when the length is 2 (i.e. when encoding a surrogate pair)
    // in this case it will always be set to 0, but we still need to pass 2 elements
    // we reuse the second element to store the utf16 character sequence length
    if (charcode <= 0xFFFF) {
        chars[0] = charcode;
        glyphs[1] = 1;
    } else {
        uint32_t norm = charcode - 0x10000;
        chars[0] = (norm >> 10) + 0xD800;
        chars[1] = (norm & 0x3FF) + 0xDC00;
        glyphs[1] = 2;
    }

    // cg_glyph[0] will always be set by Core Text no matter the status
    // thus we can ignore returned value
    CTFontGetGlyphsForCharacters(
        ct_font,
        chars,
        glyphs,
        glyphs[1]);

    return glyphs[0];
}

oc_error ocl_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t  ppem;

    CTFontRef ct_font;
    CTFontRef ct_font_copy;

    if (!face) {
        return oc_error_invalid_param;
    }

    if (desired_size < 1 << 6) {
        return oc_error_invalid_param;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    scaled = (desired_size * dpi + 36) / 72;
    scale = oc_div_16p16(scaled, face->upem);

    ct_font = (CTFontRef)face->impl;
    ct_font_copy = CTFontCreateCopyWithAttributes(ct_font, scaled / 64.0, NULL, NULL);

    if (ct_font_copy == NULL) {
        return oc_error_out_of_memory;
    }

    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        CFRelease(ct_font_copy);
        return oc_error_invalid_pixel_size;
    }

    face->impl = (oc_face_impl*)ct_font_copy;
    face->size.scale = scale;
    face->size.ppem = (uint16_t)ppem;

    CFRelease(ct_font);
    return oc_error_ok;
}

oc_error ocl_get_sfnt_table(const oc_face* face, oc_tag tag, uint32_t offset, void* data, uint32_t* size) {
    CTFontRef ct_font;
    CFDataRef ct_table;

    const UInt8* buffer;
    CFIndex      length;

    if (!(face && size)) {
        return oc_error_invalid_param;
    }

    ct_font = (CTFontRef)face->impl;
    ct_table = CTFontCopyTable(ct_font, tag, kCTFontTableOptionNoOptions);
    length = (CFIndex)*size;

    assert(length == 0 || length >= offset);

    if (ct_table == NULL) {
        // todo (stage 2): check if this can oom
        return oc_error_table_missing; // or oom
    }

    if (length == 0) {
        length = CFDataGetLength(ct_table);

        assert(UINT32_MAX >= length);
        *size = (uint32_t)length;
    } else {
        buffer = CFDataGetBytePtr(ct_table);
        memcpy(data, buffer + offset, length);
    }

    CFRelease(ct_table);
    return oc_error_ok;
}

void ocl_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    CTFontRef ct_font;
    CFIndex   count;

    CGSize advance;
    CGRect rect;

    uint16_t upem;
    CGFloat  size;
    oc_26p6  scale;

    oc_glyph_metrics metrics = { 0 };

    if (!(face && ometrics)) {
        goto exit;
    }

    ct_font = (CTFontRef)face->impl;
    count = CTFontGetGlyphCount(ct_font);

    if (index >= count) {
        goto exit;
    }

    CTFontGetAdvancesForGlyphs(ct_font, kCTFontOrientationHorizontal, &index, &advance, 1);
    rect = CTFontGetBoundingRectsForGlyphs(ct_font, kCTFontOrientationHorizontal, &index, NULL, 1);

    upem = face->upem;
    size = CTFontGetSize(ct_font);

    metrics.width = rect.size.width * upem / size;
    metrics.height = rect.size.height * upem / size;
    metrics.bearing_x = rect.origin.x * upem / size;
    metrics.bearing_y = (rect.size.height + rect.origin.y) * upem / size;
    metrics.advance = advance.width * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->size.scale;

    metrics.width = oc_mul_16p16(metrics.width, scale);
    metrics.height = oc_mul_16p16(metrics.height, scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, scale);
    metrics.advance = oc_mul_16p16(metrics.advance, scale);

    if (flags & OC_LOAD_NO_FITTING) {
        goto exit;
    }

    oc__fit_metrics(&metrics);
exit:
    if (ometrics)
        *ometrics = metrics;
}

void ocl_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    CTFontRef ct_font;
    CGRect    rect;

    uint16_t upem;
    CGFloat  size;
    oc_26p6  scale;

    oc_bbox cbox = { 0 };

    if (!(face && ocbox)) {
        goto exit;
    }

    ct_font = (CTFontRef)face->impl;

    CTFontGetBoundingRectsForGlyphs(
        ct_font,
        kCTFontOrientationHorizontal,
        &index,
        &rect,
        1);

    upem = face->upem;
    size = CTFontGetSize(ct_font);

    cbox.min_x = CGRectGetMinX(rect) * upem / size;
    cbox.min_y = CGRectGetMinY(rect) * upem / size;
    cbox.max_x = CGRectGetMaxX(rect) * upem / size;
    cbox.max_y = CGRectGetMaxY(rect) * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->size.scale;

    cbox.min_x = oc_mul_16p16(cbox.min_x, scale);
    cbox.min_y = oc_mul_16p16(cbox.min_y, scale);
    cbox.max_x = oc_mul_16p16(cbox.max_x, scale);
    cbox.max_y = oc_mul_16p16(cbox.max_y, scale);

exit:
    if (ocbox)
        *ocbox = cbox;
}

typedef struct {
    oc_point          pt1;
    oc_point          pt2;
    oc_point          pt3;
    CGPathElementType type;
} oc__path_element;

typedef struct {
    uint8_t*  tags;
    oc_point* points;
    uint16_t* contours;

    oc__path_element element;
    oc_point         start;

    CGFloat fppem;
    CGFloat fupem;
} oc__applier_context;

static void oc__walk_applier(void* info, const CGPathElement* element) {
    oc__applier_context* ctx = (oc__applier_context*)info;

    oc__path_element next_element = {
        { element->points[0].x * ctx->fupem / ctx->fppem * 2.0, element->points[0].y * ctx->fupem / ctx->fppem * 2.0 },
        { element->points[1].x * ctx->fupem / ctx->fppem * 2.0, element->points[1].y * ctx->fupem / ctx->fppem * 2.0 },
        { element->points[2].x * ctx->fupem / ctx->fppem * 2.0, element->points[2].y * ctx->fupem / ctx->fppem * 2.0 },
        element->type
    };

    bool implicit;
    bool closing;

    // todo: do not forget to check oom
    switch (ctx->element.type) {
    case kCGPathElementMoveToPoint:
        // todo: check if this can happen
        // if (final_count > 0) {
        //     if (contour_count == 0 || contour_ends[contour_count - 1] != final_count - 1) {
        //         contour_ends[contour_count++] = final_count - 1;
        //     }
        // }

        oc__append(ctx->points, oc__point_bsr(ctx->element.pt1, 1));
        oc__append(ctx->tags, OC__CURVE_TAG_ON);

        ctx->start = ctx->element.pt1;
        break;
    case kCGPathElementAddLineToPoint:
        if (!oc__points_equal(ctx->element.pt1, ctx->start)) {
            oc__append(ctx->points, oc__point_bsr(ctx->element.pt1, 1));
            oc__append(ctx->tags, OC__CURVE_TAG_ON);
        }
        break;
    case kCGPathElementAddQuadCurveToPoint:
        oc__append(ctx->points, oc__point_bsr(ctx->element.pt1, 1));
        oc__append(ctx->tags, OC__CURVE_TAG_CONIC);

        implicit = false;
        closing = false;

        if (next_element.type == kCGPathElementCloseSubpath) {
            closing = oc__points_equal(ctx->element.pt2, ctx->start);
        }

        if (next_element.type == kCGPathElementAddQuadCurveToPoint) {
            implicit = oc__is_midpoint(ctx->element.pt2, ctx->element.pt1, next_element.pt1);
        }

        if (!implicit && !closing) {
            oc__append(ctx->points, oc__point_bsr(ctx->element.pt2, 1));
            oc__append(ctx->tags, OC__CURVE_TAG_ON);
        }
        break;
    case kCGPathElementAddCurveToPoint:
        oc__append(ctx->points, oc__point_bsr(ctx->element.pt1, 1));
        oc__append(ctx->points, oc__point_bsr(ctx->element.pt2, 1));

        oc__append(ctx->tags, OC__CURVE_TAG_CUBIC);
        oc__append(ctx->tags, OC__CURVE_TAG_CUBIC);

        closing = false;
        if (next_element.type == kCGPathElementCloseSubpath) {
            closing = oc__points_equal(ctx->element.pt3, ctx->start);
        }

        if (!closing) {
            oc__append(ctx->points, oc__point_bsr(ctx->element.pt3, 1));
            oc__append(ctx->tags, OC__CURVE_TAG_ON);
        }
        break;
    case kCGPathElementCloseSubpath:
        assert(oc__len(ctx->points) > 0);
        oc__append(ctx->contours, oc__len(ctx->points) - 1);
        break;
    default:
        break;
    }

    ctx->element = next_element;
}

oc_error ocl_get_outline(const oc_face* face, uint16_t index, oc_load_flags flags, oc_outline* ooutline) {
    CTFontRef ct_font;
    CGPathRef ct_outline;
    CFIndex   glyph_count;

    oc_error err = oc_error_ok;

    oc_outline outline      = { 0 };
    oc__applier_context ctx = { 0 };

    (void)flags;

    if (!(face && ooutline)) {
        oc__exit(oc_error_invalid_param);
    }

    ct_font = (CTFontRef)face->impl;
    glyph_count = CTFontGetGlyphCount(ct_font);

    // todo: cache the glyph_count
    if (index >= glyph_count) {
        oc__exit(oc_error_invalid_param);
    }

    ct_outline = CTFontCreatePathForGlyph(ct_font, index, NULL);
    if (!ct_outline) {
        oc__exit(oc__unexpected(0));
    }

    ctx.fppem = CTFontGetSize(ct_font);
    ctx.fupem = CTFontGetUnitsPerEm(ct_font);
    ctx.element.type = -1;

    CGPathApply(ct_outline, &ctx, oc__walk_applier);
    CGPathRelease(ct_outline);

    // todo: check if it is even possible to get Close without any points
    if (oc__len(ctx.points) > 0) {
        assert(ctx.element.type == kCGPathElementCloseSubpath);
        oc__append(ctx.contours, oc__len(ctx.points) - 1);
    }

    outline.npoints = (uint16_t)oc__len(ctx.points);
    outline.ncontours = (uint16_t)oc__len(ctx.contours);

    outline.tags = ctx.tags;
    outline.points = ctx.points;
    outline.contours = ctx.contours;
exit:
    if (ooutline != NULL)
        *ooutline = outline;

    return err;
}

oc_error ocl_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t pitch) {
    oc_error err = oc_error_ok;

    CTFontRef ct_font;
    CFIndex   count;

    oc_bbox cbox;
    oc_bbox pbox;

    CGColorSpaceRef linear_gray;
    CGContextRef    context;
    CGRect          rect;
    CGPoint         pos;

    oc_extent extent = { 0 };

    if (!(face && oextent)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    ct_font = (CTFontRef)face->impl;
    count = CTFontGetGlyphCount(ct_font);

    if (index >= count) {
        err = oc_error_invalid_param;
        goto exit;
    }

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L414
    ocl_get_glyph_cbox(face, index, OC_LOAD_DEFAULT, &cbox);

    pbox.min_x = cbox.min_x >> 6;
    pbox.min_y = cbox.min_y >> 6;
    pbox.max_x = cbox.max_x >> 6;
    pbox.max_y = cbox.max_y >> 6;

    // take fractional part and ceil it
    pbox.max_x += ((cbox.max_x & 63) + 63) >> 6;
    pbox.max_y += ((cbox.max_y & 63) + 63) >> 6;

    extent.rows = pbox.max_y - pbox.min_y;
    extent.cols = pbox.max_x - pbox.min_x;

    if (buffer == NULL) {
        goto exit;
    }

    if (extent.rows == 0 || extent.cols == 0) {
        goto exit;
    }

    linear_gray = CGColorSpaceCreateWithName(kCGColorSpaceLinearGray);
    if (linear_gray == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    context = CGBitmapContextCreate(
        buffer,
        extent.cols,
        extent.rows,
        8,
        pitch,
        linear_gray,
        kCGImageAlphaNone);
    CGColorSpaceRelease(linear_gray);

    if (context == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    rect.origin.x = 0;
    rect.origin.y = 0;
    rect.size.height = extent.rows;
    rect.size.width = extent.cols;

    // https://github.com/ghostty-org/ghostty/blob/main/src/font/face/coretext.zig#L478

    CGContextClearRect(context, rect);

    CGContextSetGrayFillColor(context, 0.0, 0.0);
    CGContextFillRect(context, rect);

    CGContextSetAllowsFontSmoothing(context, false);
    CGContextSetShouldSmoothFonts(context, false);

    CGContextSetAllowsFontSubpixelPositioning(context, true);
    CGContextSetShouldSubpixelPositionFonts(context, true);

    CGContextSetAllowsFontSubpixelQuantization(context, false);
    CGContextSetShouldSubpixelQuantizeFonts(context, false);

    CGContextSetAllowsAntialiasing(context, true);
    CGContextSetShouldAntialias(context, true);

    CGContextSetGrayFillColor(context, 1.0, 1.0);
    CGContextSetGrayStrokeColor(context, 1.0, 1.0);

    CGContextTranslateCTM(context, (cbox.min_x & 63) / 64.0, (cbox.min_y & 63) / 64.0);

    pos.x = -cbox.min_x / 64.0;
    pos.y = -cbox.min_y / 64.0;

    CTFontDrawGlyphs(ct_font, &index, &pos, 1, context);
    CGContextRelease(context);
exit:
    if (oextent)
        *oextent = extent;
    return err;
}
#endif /* ONECORE_CORETEXT_LOADER_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_FINDER_IMPLEMENTATION
#include <CoreText/CoreText.h>

typedef struct {
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    const oc_library* oc_library;
#endif
    CTFontDescriptorRef ct_font;
    CTFontRef           ct_face;
    CFStringRef         ct_family;
    oc_font             font;
} oc__font_impl;

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
struct oc_collection_impl {
    const oc_library* oc_library;
    CFArrayRef        ct_fonts;
};
#endif

static inline void oc__free_font_impl(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    CFRelease(impl->ct_family);
    CFRelease(impl->ct_face);
    free(impl);
}

oc_error ocf_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error      err = oc_error_ok;
    oc_collection collection = { 0 };

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    collection.impl = calloc(1, sizeof(oc_collection_impl));
    if (!collection.impl) {
        err = oc_error_out_of_memory;
        goto exit;
    }
    collection.impl->oc_library = library;
#endif
exit:
    if (ocollection)
        *ocollection = collection;
    return err;
}

void ocf_free_collection(oc_collection* collection) {
    if (collection) {
        while (collection->nfonts--) {
            oc__free_font_impl(collection->fonts[collection->nfonts]);
        }

        free(collection->fonts);

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
        if (collection->impl->ct_fonts) {
            CFRelease(collection->impl->ct_fonts);
        }
        free(collection->impl);
#else
        if (collection->impl)
            CFRelease(collection->impl);
#endif
        memset(collection, 0, sizeof(*collection));
    }
}

static oc__font_impl* oc__init_font_impl(const oc_library* oc_library, CTFontDescriptorRef ct_font) {
    oc__font_impl* impl = NULL;

    CFDictionaryRef ct_traits = NULL;
    CFStringRef     ct_family;
    CTFontRef       ct_face;

    const char* family;

    CFNumberRef symbolic_obj;
    CFNumberRef weight_obj;

    int      weight;
    uint32_t ct_symbolic;

    assert(ct_font != NULL);
    (void)oc_library;

    // todo (stage 2): do some assumptions based on this assumption
    // is_immortal = CFGetRetainCount(obj) == 0x7FFFFFFFFFFFFFFF

    // Cheers to AI it has found private api to 'CTFontCSSWeightAttribute'
    weight_obj = CTFontDescriptorCopyAttribute(ct_font, CFSTR("CTFontCSSWeightAttribute"));
    // Notify developer on GitHub if this assertion ever fails:
    // https://github.com/nedpranson/onecore/issues
    assert(weight_obj != NULL);

    CFNumberGetValue(weight_obj, kCFNumberIntType, &weight);
    CFRelease(weight_obj);

    ct_traits = CTFontDescriptorCopyAttribute(ct_font, kCTFontTraitsAttribute);
    if (ct_traits == NULL) {
        goto exit;
    }

    symbolic_obj = CFDictionaryGetValue(ct_traits, kCTFontSymbolicTrait);
    assert(symbolic_obj != NULL);

    CFNumberGetValue(symbolic_obj, kCFNumberSInt32Type, &ct_symbolic);

    ct_family = CTFontDescriptorCopyAttribute(ct_font, kCTFontFamilyNameAttribute);
    assert(ct_family != NULL);

    // family seems to always be utf8 and null terminated
    family = CFStringGetCStringPtr(ct_family, kCFStringEncodingUTF8);
    assert(family != NULL);

    ct_face = CTFontCreateWithFontDescriptor(ct_font, 0.0, NULL);
    if (ct_face == NULL) {
        CFRelease(ct_family);
        goto exit;
    }

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        CFRelease(ct_family);
        CFRelease(ct_face);
        goto exit;
    }

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    impl->oc_library = oc_library;
#endif
    impl->ct_font = ct_font;
    impl->ct_family = ct_family;
    impl->ct_face = ct_face;
    impl->font.family = family;
    impl->font.weight = (uint16_t)weight;
    impl->font.slant = oc_slant_roman;

    // todo (stage 2): implement valid one
    // we need crossplatform solution
    if (ct_symbolic & kCTFontItalicTrait) {
        impl->font.slant = oc_slant_italic;
    }

exit:
    if (ct_traits)
        CFRelease(ct_traits);
    return impl;
}

// todo: clean!
oc_error ocf_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;

    CTFontCollectionRef ct_collection;
    CFArrayRef          ct_fonts;

    CFIndex font_count;

    oc_font** fonts = NULL;
    uint32_t  nfonts = 0;

    oc_collection     tmp_collection;
    const oc_library* oc_library = NULL;

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    CFArrayRef ct_fonts2;
#endif

    if (!collection) {
        err = oc_error_invalid_param;
        goto exit;
    }

    ct_collection = CTFontCollectionCreateFromAvailableFonts(NULL);
    if (ct_collection == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    ct_fonts = CTFontCollectionCreateMatchingFontDescriptors(ct_collection);
    CFRelease(ct_collection);

    if (ct_fonts == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    font_count = CFArrayGetCount(ct_fonts);
    if (font_count == 0) {
        goto done;
    }

    fonts = malloc(font_count * sizeof(*fonts));
    if (fonts == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    oc_library = collection->impl->oc_library;
#endif

    for (CFIndex i = 0; i < font_count; i++) {
        CTFontDescriptorRef ct_font = CFArrayGetValueAtIndex(ct_fonts, i);
        oc__font_impl*      impl = oc__init_font_impl(oc_library, ct_font);
        if (impl == NULL) {
            err = oc_error_out_of_memory;
            goto exit;
        }

        fonts[nfonts++] = &impl->font;
    }
done:
// todo: clean this shi up
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    ct_fonts2 = ct_fonts;
#else
    tmp_collection.impl = (oc_collection_impl*)ct_fonts;
#endif
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    ct_fonts2 = collection->impl->ct_fonts;
#else
    ct_fonts = (CFArrayRef)collection->impl;
#endif
    fonts = collection->fonts;
    nfonts = collection->nfonts;
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    tmp_collection.impl = collection->impl;
    collection->impl->ct_fonts = ct_fonts;
    ct_fonts = ct_fonts2;
#endif
    *collection = tmp_collection;
exit:
    while (nfonts--)
        oc__free_font_impl(fonts[nfonts]);
    free(fonts);
    if (ct_fonts)
        CFRelease(ct_fonts);

    return err;
}

bool ocf_has_character(const oc_font* font, uint32_t charcode) {
    oc__font_impl* impl;
    CTFontRef      ct_face;

    CGGlyph glyphs[2];
    UniChar chars[2];

    if (!font || charcode > 0x10FFFF) {
        return false;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    ct_face = impl->ct_face;

    // check out CFStringGetSurrogatePairForLongCharacter

    // CTFontGetGlyphsForCharacters writes cg_glyph[1] when the length is 2 (i.e. when encoding a surrogate pair)
    // in this case it will always be set to 0, but we still need to pass 2 elements
    // we reuse the second element to store the utf16 character sequence length
    if (charcode <= 0xFFFF) {
        chars[0] = charcode;
        glyphs[1] = 1;
    } else {
        uint32_t norm = charcode - 0x10000;
        chars[0] = (norm >> 10) + 0xD800;
        chars[1] = (norm & 0x3FF) + 0xDC00;
        glyphs[1] = 2;
    }

    // cg_glyph[0] will always be set by Core Text no matter the status
    // thus we can ignore returned value
    CTFontGetGlyphsForCharacters(
        ct_face,
        chars,
        glyphs,
        glyphs[1]);

    return glyphs[0];
}

#ifdef ONECORE_CORETEXT_LOADER_IMPLEMENTATION
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;
    oc_error       err;
    oc_face        face = { 0 };

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);

    if (desired_size == 0) {
        desired_size = 12 << 6;
    } else if (desired_size < 1 << 6) {
        desired_size = 1 << 6;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    err = oc__init_face(impl->ct_font, desired_size, dpi, &face);
    *oface = face;

    return err;
}
#elif defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION)
// todo (stage 2): handle memory only fonts!
// when we will implement correct reconstruction function
// we could use FT_StreamRec to stream data and emulate it
// just build these ocf__offset_table ocf__table_record
// give to freetype and forget
// any other data we track on which tag we are and just give it back
typedef struct {
    int32_t  sfnt_version;
    uint16_t num_tables;
    uint16_t search_range;
    uint16_t entry_selector;
    uint16_t range_shift;
} ocf__offset_table;

typedef struct {
    uint32_t tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
} ocf__table_record;

static void* ocf__make_head(CGFontRef cg_font, CFArrayRef tags, uint32_t* size) {
    CFIndex ntags;

    uint32_t file_size;
    uint32_t head_size;

    bool  cff = false;
    void* head_data;

    ocf__offset_table* table;
    ocf__table_record* records;

    assert(cg_font != NULL);
    assert(tags != NULL);

    ntags = CFArrayGetCount(tags);
    assert(0 < ntags && ntags <= UINT16_MAX);

    head_size = sizeof(ocf__offset_table) + sizeof(ocf__table_record) * ntags;
    head_data = malloc(head_size);

    if (!head_data) {
        return NULL;
    }

    table = head_data;
    records = head_data + sizeof(ocf__offset_table);

    file_size = head_size;
    for (CFIndex i = 0; i < ntags; i++) {
        uint32_t tag = (uint32_t)(uintptr_t)CFArrayGetValueAtIndex(tags, i);

        CFDataRef table = CGFontCopyTableForTag(cg_font, tag);
        CFIndex   table_length = CFDataGetLength(table);

        ocf__table_record* record = records + i;

        if (tag == 'CFF ') {
            cff = true;
        }

        record->tag = CFSwapInt32HostToBig(tag);
        record->checksum = 0;
        record->offset = CFSwapInt32HostToBig(file_size);
        record->length = CFSwapInt32HostToBig((uint32_t)table_length);

        file_size += (table_length + 3) & ~3;
        CFRelease(table);
    }

    table->sfnt_version = cff ? 'OTTO' : CFSwapInt32HostToBig(0x10000);
    table->num_tables = CFSwapInt16HostToBig((uint16_t)ntags);
    table->search_range = 0;
    table->entry_selector = 0;
    table->range_shift = 0;

    *size = file_size;
    return head_data;
}

typedef struct {
    CGFontRef font;
    void*     head;
} ocf__context;

static void ocf__stream_close(FT_Stream stream) {
    const ocf__context* context = stream->descriptor.pointer;

    free(context->head);
    CFRelease(context->font);
}

static unsigned long ocf__stream_read(
    FT_Stream      stream,
    unsigned long  offset,
    unsigned char* buffer,
    unsigned long  count) {
    const ocf__context* context;

    const ocf__offset_table* table;
    const ocf__table_record* records;

    uint16_t ntags;

    const void* ptr;
    uint32_t    len;

    uint32_t head_size;

    CFDataRef ct_table = NULL;

    assert(stream != NULL);
    assert(stream->size >= offset && stream->size - offset >= count);

    if (count == 0) {
        return 0;
    }

    context = stream->descriptor.pointer;

    table = context->head;
    records = context->head + sizeof(ocf__offset_table);

    ntags = CFSwapInt16BigToHost(table->num_tables);
    head_size = sizeof(*table) + sizeof(*records) * ntags;

    if (head_size >= offset) {
        ptr = context->head + offset;
        len = head_size - offset;
    } else {
        uint16_t lo = 0;
        uint16_t hi = ntags;

        while (lo < hi) {
            uint16_t mid = lo + ((hi - lo) >> 1);
            if (CFSwapInt32BigToHost(records[mid].offset) < offset) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        assert(lo != ntags);

        ct_table = CGFontCopyTableForTag(context->font, CFSwapInt32BigToHost(records[lo].tag));

        ptr = CFDataGetBytePtr(ct_table);
        len = CFDataGetLength(ct_table);
    }

    memcpy(buffer, ptr, OC__MIN(count, len));
    if (ct_table)
        CFRelease(ct_table);

    return OC__MIN(count, len);
}

oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;

    CGFontRef  cg_font;
    CFArrayRef tags;

    oc_face  face = { 0 };
    oc_error err = oc_error_ok;

    void*    file_head;
    uint32_t file_size;

    ocf__context* context;

    FT_Open_Args args = { 0 };
    FT_Stream    stream;

    FT_Face  ft_face;
    FT_Error ft_err;

    oc_open_params params;

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    assert(impl->ct_face != NULL); // todo: add these types asserts everywhere

    cg_font = CTFontCopyGraphicsFont(impl->ct_face, NULL);
    if (!cg_font) {
        oc__exit(oc_error_out_of_memory);
    }

    tags = CGFontCopyTableTags(cg_font);
    if (!tags) {
        CFRelease(cg_font);
        oc__exit(oc_error_out_of_memory);
    }

    file_head = ocf__make_head(cg_font, tags, &file_size);
    CFRelease(tags);

    if (!file_head) {
        CFRelease(cg_font);
        oc__exit(oc_error_out_of_memory);
    }

    context = malloc(sizeof(*context));
    if (!context) {
        CFRelease(cg_font);
        oc__exit(oc_error_out_of_memory);
    }

    context->font = cg_font;
    context->head = file_head;

    stream = calloc(1, sizeof(*stream));
    if (!stream) {
        free(file_head);
        CFRelease(cg_font);
        oc__exit(oc_error_out_of_memory);
    }

    stream->descriptor.pointer = context;
    stream->read = &ocf__stream_read;
    stream->close = &ocf__stream_close;
    stream->size = file_size;

    args.flags = FT_OPEN_STREAM;
    args.stream = stream;

    ft_err = FT_Open_Face((FT_Library)impl->oc_library, &args, 0, &ft_face);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        oc__exit(oc_error_out_of_memory);
    case FT_Err_Invalid_File_Format:
    case FT_Err_Unknown_File_Format:
    case FT_Err_Invalid_Stream_Operation:
        oc__exit(oc_error_failed_to_open);
    default:
        oc__exit(oc__unexpected(ft_err));
    }

    params.face_index = 0;
    params.desired_size = desired_size;
    params.dpi = dpi;

    params = oc__open_params_defaults(&params);
    err = oc__init_face(ft_face, &params, &face);

    if (err != oc_error_ok) {
        FT_Done_Face(ft_face);
    }
exit:
    *oface = face;
    return err;
}
#endif

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    CFURLRef       url;

    CFStringRef path;
    CFIndex     path_len;

    size_t copy_len;

    if (!font) {
        return 0;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    url = CTFontDescriptorCopyAttribute(impl->ct_font, kCTFontURLAttribute);

    if (url == NULL) {
        return 0;
    }

    path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
    CFRelease(url);

    if (path == NULL) {
        return 0;
    }

    path_len = CFStringGetBytes(
        path,
        CFRangeMake(0, CFStringGetLength(path)),
        kCFStringEncodingUTF8,
        0,
        false,
        NULL,
        0,
        NULL);

    copy_len = len < (size_t)path_len ? len : (size_t)path_len;
    if (copy_len == 0) {
        CFRelease(path);
        return (size_t)path_len;
    }

    CFStringGetBytes(
        path,
        CFRangeMake(0, CFStringGetLength(path)),
        kCFStringEncodingUTF8,
        0,
        false,
        (UInt8*)buf,
        copy_len,
        NULL);

    CFRelease(path);
    return copy_len;
}
#endif /* ONECORE_CORETEXT_LINDER_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION
#include <initguid.h>

#include <d2d1.h>
#include <dwrite.h>
#include <math.h>

struct oc_face_impl {
    IDWriteFontFace* dw_face;
    IDWriteFactory*  dw_factory;
};

typedef struct {
    const void* data;
    size_t      size;
} oc__memory_view;

typedef struct {
    const IDWriteFontFileStreamVtbl* lpVtbl;
    LONG                             ref_count;
    oc__memory_view                  memory_view;
} OC__IDWriteFontFileStream;

typedef struct {
    const IDWriteFontFileLoaderVtbl* lpVtbl;
    LONG                             ref_count;
} OC__IDWriteFontFileLoader;

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileStream_GetLastWriteTime(IDWriteFontFileStream* This, UINT64* last_writetime) {
    (void)This;
    if (last_writetime == NULL) {
        return E_POINTER;
    }

    *last_writetime = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileStream_GetFileSize(IDWriteFontFileStream* This, UINT64* size) {
    OC__IDWriteFontFileStream* this = (OC__IDWriteFontFileStream*)This;

    if (size == NULL) {
        return E_POINTER;
    }

    *size = this->memory_view.size;
    return S_OK;
}

static void STDMETHODCALLTYPE
OC__IDWriteFontFileStream_ReleaseFileFragment(IDWriteFontFileStream* This, void* fragment_context) {
    (void)This;
    (void)fragment_context;
}

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileStream_ReadFileFragment(
    IDWriteFontFileStream* This,
    const void**           fragment_start,
    UINT64                 offset,
    UINT64                 fragment_size,
    void**                 fragment_context) {

    OC__IDWriteFontFileStream* this = (OC__IDWriteFontFileStream*)This;

    if (fragment_start == NULL) {
        return E_POINTER;
    }
    *fragment_start = NULL;

    if (fragment_context == NULL) {
        return E_POINTER;
    }
    *fragment_context = NULL;

    if (offset > this->memory_view.size || fragment_size > this->memory_view.size - offset) {
        return E_FAIL;
    }

    *fragment_start = (uint8_t*)this->memory_view.data + offset;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
OC__IDWriteFontFileStream_Release(IDWriteFontFileStream* This) {
    OC__IDWriteFontFileStream* this = (OC__IDWriteFontFileStream*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    if (refs == 0) {
        free(this);
    }

    assert(refs != -1);
    return refs;
}

static ULONG STDMETHODCALLTYPE
OC__IDWriteFontFileStream_AddRef(IDWriteFontFileStream* This) {
    OC__IDWriteFontFileStream* this = (OC__IDWriteFontFileStream*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileStream_QueryInterface(IDWriteFontFileStream* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileStream)) {
        OC__IDWriteFontFileStream_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const IDWriteFontFileStreamVtbl OC__IDWriteFontFileStreamVtbl = {
    OC__IDWriteFontFileStream_QueryInterface,
    OC__IDWriteFontFileStream_AddRef,
    OC__IDWriteFontFileStream_Release,
    OC__IDWriteFontFileStream_ReadFileFragment,
    OC__IDWriteFontFileStream_ReleaseFileFragment,
    OC__IDWriteFontFileStream_GetFileSize,
    OC__IDWriteFontFileStream_GetLastWriteTime,
};

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileLoader_CreateStreamFromKey(IDWriteFontFileLoader* This, const void* key, UINT32 key_size, IDWriteFontFileStream** stream) {
    (void)This;

    if (stream == NULL) {
        return E_POINTER;
    }
    *stream = NULL;

    if (key == NULL) {
        return E_POINTER;
    }

    if (key_size != sizeof(oc__memory_view)) {
        return E_INVALIDARG;
    }

    oc__memory_view view = *(const oc__memory_view*)key;
    if (view.data == NULL) {
        return E_INVALIDARG;
    }

    OC__IDWriteFontFileStream* file_stream = malloc(sizeof(OC__IDWriteFontFileStream));
    if (file_stream == NULL) {
        return E_OUTOFMEMORY;
    }

    file_stream->lpVtbl = &OC__IDWriteFontFileStreamVtbl;
    file_stream->ref_count = 1;
    file_stream->memory_view = view;

    *stream = (IDWriteFontFileStream*)file_stream;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
OC__IDWriteFontFileLoader_Release(IDWriteFontFileLoader* This) {
    OC__IDWriteFontFileLoader* this = (OC__IDWriteFontFileLoader*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    assert(refs != -1);
    return refs;
}

static ULONG STDMETHODCALLTYPE
OC__IDWriteFontFileLoader_AddRef(IDWriteFontFileLoader* This) {
    OC__IDWriteFontFileLoader* this = (OC__IDWriteFontFileLoader*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileLoader_QueryInterface(IDWriteFontFileLoader* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader)) {
        OC__IDWriteFontFileLoader_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const IDWriteFontFileLoaderVtbl OC__IDWriteFontFileLoaderVtbl = {
    OC__IDWriteFontFileLoader_QueryInterface,
    OC__IDWriteFontFileLoader_AddRef,
    OC__IDWriteFontFileLoader_Release,
    OC__IDWriteFontFileLoader_CreateStreamFromKey
};

OC__IDWriteFontFileLoader oc__file_loader = { &OC__IDWriteFontFileLoaderVtbl, 0 };
IDWriteFontFileLoader*    oc__dw_file_loader = (IDWriteFontFileLoader*)&oc__file_loader;

#define OC__OVERRIDE_LIBRARY_IMPL

oc_error oc_init_library(oc_library** olibrary) {
    HRESULT         err;
    IDWriteFactory* dw_factory;

    if (olibrary == NULL) {
        return oc_error_invalid_param;
    }

    err = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&dw_factory);
    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    err = dw_factory->lpVtbl->RegisterFontFileLoader(dw_factory, oc__dw_file_loader);
    if (err != S_OK) {
        // DWRITE_E_ALREADYREGISTERED;
        dw_factory->lpVtbl->Release(dw_factory);
        return oc__unexpected(err);
    }

    *olibrary = (oc_library*)dw_factory;
    return oc_error_ok;
}

void oc_free_library(oc_library* library) {
    IDWriteFactory* dw_factory;

    if (!library) {
        return;
    }

    dw_factory = (IDWriteFactory*)library;

    dw_factory->lpVtbl->UnregisterFontFileLoader(dw_factory, oc__dw_file_loader);
    dw_factory->lpVtbl->Release(dw_factory);
}

static oc_error oc__init_face(IDWriteFactory* dw_factory, IDWriteFontFace* dw_face, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    DWRITE_FONT_METRICS metrics;
    oc_16p16            scaled;
    oc_16p16            scale;
    int32_t             ppem;
    oc_face             face;

    assert(dpi > 0);
    assert(desired_size >= 1 << 6);

    face.impl = malloc(sizeof(face));
    if (face.impl == NULL) {
        dw_face->lpVtbl->Release(dw_face);
        return oc_error_out_of_memory;
    }

    dw_face->lpVtbl->GetMetrics(dw_face, &metrics);

    // https://github.com/freetype/freetype/blob/85c8efe0afa5ad0df35114e317a065f544943c52/include/freetype/internal/ftobjs.h#L665
    scaled = (desired_size * dpi + 36) / 72;
    scale = oc_div_16p16(scaled, metrics.designUnitsPerEm);

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L3368
    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        dw_face->lpVtbl->Release(dw_face);
        return oc_error_invalid_pixel_size;
    }

    face.impl->dw_face = dw_face;
    face.impl->dw_factory = dw_factory;
    face.size.scale = scale;
    face.size.ppem = (uint16_t)ppem;
    face.upem = metrics.designUnitsPerEm;
    face.ascent = metrics.ascent;
    face.descent = metrics.descent;
    face.leading = metrics.lineGap;
    face.underline_position = metrics.underlinePosition;
    face.underline_thickness = metrics.underlineThickness;

    *oface = face;
    return oc_error_ok;
}

static oc_error oc__open_face_from_font_file(IDWriteFactory* dw_factory, IDWriteFontFile* font_file, const oc_open_params* uparams, oc_face* oface) {
    HRESULT               err;
    WINBOOL               is_supported_fonttype;
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFace*      dw_face;
    UINT32                face_num;
    oc_open_params        params = oc__open_params_defaults(uparams);

    err = font_file->lpVtbl->Analyze(
        font_file,
        &is_supported_fonttype,
        &file_type,
        &face_type,
        &face_num);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    if (!is_supported_fonttype) {
        return oc_error_failed_to_open;
    }

    err = dw_factory->lpVtbl->CreateFontFace(
        dw_factory,
        face_type,
        1,
        &font_file,
        params.face_index,
        DWRITE_FONT_SIMULATIONS_NONE,
        &dw_face);

    switch (err) {
    case S_OK:
        break;
    case E_INVALIDARG:
        return oc_error_invalid_param;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    return oc__init_face(dw_factory, dw_face, params.desired_size, params.dpi, oface);
}

oc_error ocl_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    int32_t          err;
    int              size;
    wchar_t*         dw_path;
    IDWriteFactory*  dw_factory;
    IDWriteFontFile* dw_font_file;

    if (!(library && path && oface)) {
        return oc_error_invalid_param;
    }

    size = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (size <= 1) {
        return oc_error_failed_to_open;
    }

    dw_path = malloc(size * sizeof(wchar_t));
    if (dw_path == NULL) {
        return oc_error_out_of_memory;
    }

    size = MultiByteToWideChar(CP_UTF8, 0, path, -1, dw_path, size);
    assert(size > 0);

    dw_factory = (IDWriteFactory*)library;
    err = dw_factory->lpVtbl->CreateFontFileReference(
        dw_factory,
        dw_path,
        NULL,
        &dw_font_file);
    free(dw_path);

    switch (err) {
    case S_OK:
        break;
    case DWRITE_E_FILENOTFOUND:
        return oc_error_failed_to_open;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    err = oc__open_face_from_font_file(dw_factory, dw_font_file, uparams, oface);
    dw_font_file->lpVtbl->Release(dw_font_file);

    return err;
}

oc_error ocl_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    int32_t          err;
    IDWriteFontFile* font_file;
    IDWriteFactory*  dw_factory;
    oc__memory_view  key;

    if (!(library && data && oface)) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    dw_factory = (IDWriteFactory*)library;

    key.data = data;
    key.size = size;

    err = dw_factory->lpVtbl->CreateCustomFontFileReference(
        dw_factory,
        &key,
        sizeof(key),
        oc__dw_file_loader,
        &font_file);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    err = oc__open_face_from_font_file(dw_factory, font_file, uparams, oface);
    font_file->lpVtbl->Release(font_file);

    return err;
}

void ocl_free_face(oc_face* face) {
    face->impl->dw_face->lpVtbl->Release(face->impl->dw_face);
    free(face->impl);
    memset(face, 0, sizeof(*face));
}

uint16_t ocl_get_char_index(const oc_face* face, uint32_t charcode) {
    HRESULT          err;
    IDWriteFontFace* dw_face;
    UINT16           index;

    if (face == NULL) {
        return 0;
    }

    dw_face = face->impl->dw_face;
    err = dw_face->lpVtbl->GetGlyphIndices(
        dw_face,
        &charcode,
        1,
        &index);

    (void)err;
    assert(err == S_OK);

    return index;
}

oc_error ocl_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t  ppem;

    if (!face) {
        return oc_error_invalid_param;
    }

    if (desired_size < 1 << 6) {
        return oc_error_invalid_param;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    scaled = (desired_size * dpi + 36) / 72;
    scale = oc_div_16p16(scaled, face->upem);
    ppem = (scaled + 32) >> 6;

    if (ppem > UINT16_MAX) {
        return oc_error_invalid_pixel_size;
    }

    face->size.scale = scale;
    face->size.ppem = (uint16_t)ppem;

    return oc_error_ok;
}

oc_error ocl_get_sfnt_table(const oc_face* face, oc_tag tag, uint32_t offset, void* data, uint32_t* size) {
    HRESULT          err;
    IDWriteFontFace* dw_face;

    const void* table_data;
    UINT32      table_size;

    void*   context;
    WINBOOL exists;

    uint32_t length;

    if (!(face && size)) {
        return oc_error_invalid_param;
    }

    dw_face = face->impl->dw_face;
    length = *size;

    assert(sizeof(UINT32) == sizeof(uint32_t));
    assert(length == 0 || length >= offset);

    err = dw_face->lpVtbl->TryGetFontTable(
        dw_face,
        _byteswap_ulong(tag), // swapping bytes as windows table tags are little-endian
        &table_data,
        &table_size,
        &context,
        &exists);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    if (!exists) {
        return oc_error_table_missing;
    }

    if (length == 0) {
        *size = table_size;
    } else {
        memcpy(data, (uint8_t*)table_data + offset, length);
    }

    dw_face->lpVtbl->ReleaseFontTable(dw_face, context);
    return oc_error_ok;
}

void ocl_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    HRESULT              err;
    DWRITE_GLYPH_METRICS dw_metrics;
    IDWriteFontFace*     dw_face;
    oc_16p16             scale;
    UINT16               count;
    oc_glyph_metrics     metrics = { 0 };

    if (!(face && ometrics)) {
        goto exit;
    }

    dw_face = face->impl->dw_face;
    count = dw_face->lpVtbl->GetGlyphCount(dw_face);

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    if (index >= count) {
        goto exit;
    }

    err = dw_face->lpVtbl->GetDesignGlyphMetrics(
        dw_face,
        &index,
        1,
        &dw_metrics,
        FALSE);

    (void)err;
    assert(err == S_OK);

    metrics.width = (INT32)dw_metrics.advanceWidth - dw_metrics.leftSideBearing - dw_metrics.rightSideBearing;
    metrics.height = (INT32)dw_metrics.advanceHeight - dw_metrics.topSideBearing - dw_metrics.bottomSideBearing;
    metrics.bearing_x = dw_metrics.leftSideBearing;
    metrics.bearing_y = dw_metrics.verticalOriginY - dw_metrics.topSideBearing;
    metrics.advance = dw_metrics.advanceWidth;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->size.scale;

    metrics.width = oc_mul_16p16(metrics.width, scale);
    metrics.height = oc_mul_16p16(metrics.height, scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, scale);
    metrics.advance = oc_mul_16p16(metrics.advance, scale);

    if (flags & OC_LOAD_NO_FITTING) {
        goto exit;
    }

    oc__fit_metrics(&metrics);
exit:
    if (ometrics)
        *ometrics = metrics;
}

void ocl_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    HRESULT              err;
    DWRITE_GLYPH_METRICS metrics;
    IDWriteFontFace*     dw_face;
    UINT16               count;
    oc_16p16             scale;
    oc_bbox              cbox = { 0 };

    if (!(face && ocbox)) {
        goto exit;
    }

    dw_face = face->impl->dw_face;
    count = dw_face->lpVtbl->GetGlyphCount(dw_face);

    if (index >= count) {
        goto exit;
    }

    err = dw_face->lpVtbl->GetDesignGlyphMetrics(
        dw_face,
        &index,
        1,
        &metrics,
        FALSE);

    (void)err;
    assert(err == S_OK);

    cbox.min_x = metrics.leftSideBearing;
    cbox.min_y = metrics.verticalOriginY + metrics.bottomSideBearing - (INT32)metrics.advanceHeight;
    cbox.max_x = metrics.advanceWidth - metrics.rightSideBearing;
    cbox.max_y = metrics.verticalOriginY - metrics.topSideBearing;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->size.scale;

    cbox.min_x = oc_mul_16p16(cbox.min_x, scale);
    cbox.min_y = oc_mul_16p16(cbox.min_y, scale);
    cbox.max_x = oc_mul_16p16(cbox.max_x, scale);
    cbox.max_y = oc_mul_16p16(cbox.max_y, scale);

exit:
    if (ocbox)
        *ocbox = cbox;
}

typedef enum {
    oc__path_move,
    oc__path_line,
    oc__path_conic,
    oc__path_cubic,
    oc__path_close,
} oc__path;

typedef struct {
    oc_point pt1;
    oc_point pt2;
    oc_point pt3;
    oc__path type;
} oc__path_element;

typedef struct {
    D2D1_POINT_2F pt1;
    D2D1_POINT_2F pt2;
    D2D1_POINT_2F pt3;
    oc__path      type;
} oc__path_felement;

typedef struct {
    const ID2D1SimplifiedGeometrySinkVtbl* lpVtbl;

    uint8_t*  tags;
    oc_point* points;
    uint16_t* contours;

    oc__path_element element;

    D2D1_POINT_2F origin;
    D2D1_POINT_2F start;

    LONG ref_count;
    bool doomed; // if true oom is reached
} OC__PathSink;

static bool oc__walk_outline(OC__PathSink* sink, const oc__path_felement* element) {
    oc__path_element next_element = {
        .pt1 = { element->pt1.x * 2.0f, element->pt1.y * -2.0f },
        .pt2 = { element->pt2.x * 2.0f, element->pt2.y * -2.0f },
        .pt3 = { element->pt3.x * 2.0f, element->pt3.y * -2.0f },
        .type = element->type,
    };

    oc_point start = { sink->start.x * 2.0f, sink->start.y * -2.0f };

    bool implicit;
    bool closing;

    switch (sink->element.type) {
    case oc__path_move:
        if (!oc__append(sink->points, oc__point_bsr(sink->element.pt1, 1))) return true;
        if (!oc__append(sink->tags, OC__CURVE_TAG_ON)) return true;
        break;
    case oc__path_line:
        if (!oc__points_equal(sink->element.pt1, start)) {
            if (!oc__append(sink->points, oc__point_bsr(sink->element.pt1, 1))) return true;
            if (!oc__append(sink->tags, OC__CURVE_TAG_ON)) return true;
        }
        break;
    case oc__path_conic:
        if (!oc__append(sink->points, oc__point_bsr(sink->element.pt1, 1))) return true;
        if (!oc__append(sink->tags, OC__CURVE_TAG_CONIC)) return true;

        implicit = false;
        closing = false;

        if (next_element.type == oc__path_close) {
            closing = oc__points_equal(sink->element.pt2, start);
        }

        if (next_element.type == oc__path_conic) {
            implicit = oc__is_midpoint(sink->element.pt2, sink->element.pt1, next_element.pt1);
        }

        if (!implicit && !closing) {
            if (!oc__append(sink->points, oc__point_bsr(sink->element.pt2, 1))) return true;
            if (!oc__append(sink->tags, OC__CURVE_TAG_ON)) return true;
        }
        break;
    case oc__path_cubic:
        if (!oc__append(sink->points, oc__point_bsr(sink->element.pt1, 1))) return true;
        if (!oc__append(sink->points, oc__point_bsr(sink->element.pt2, 1))) return true;

        if (!oc__append(sink->tags, OC__CURVE_TAG_CUBIC)) return true;
        if (!oc__append(sink->tags, OC__CURVE_TAG_CUBIC)) return true;

        closing = false;
        if (next_element.type == oc__path_close) {
            closing = oc__points_equal(sink->element.pt3, start);
        }

        if (!closing) {
            if (!oc__append(sink->points, oc__point_bsr(sink->element.pt3, 1))) return true;
            if (!oc__append(sink->tags, OC__CURVE_TAG_ON)) return true;
        }
        break;
    case oc__path_close:
        assert(oc__len(sink->points) > 0);
        if (!oc__append(sink->contours, oc__len(sink->points) - 1)) return true;
        break;
    default:
        break;
    }

    sink->element = next_element;
    return false;
}

static HRESULT STDMETHODCALLTYPE
OC__PathSink_Close(ID2D1SimplifiedGeometrySink* This) {
    (void)This;
    return S_OK;
}

static void STDMETHODCALLTYPE
OC__PathSink_EndFigure(ID2D1SimplifiedGeometrySink* This, D2D1_FIGURE_END figureEnd) {
    OC__PathSink*     this = (OC__PathSink*)This;
    oc__path_felement element = { 0 };

    (void)figureEnd;

    if (oc__unlikely(this->doomed)) {
        return;
    }

    element.type = oc__path_close;
    this->doomed = oc__walk_outline(this, &element);
}

static void STDMETHODCALLTYPE
OC__PathSink_AddBeziers(ID2D1SimplifiedGeometrySink* This, const D2D1_BEZIER_SEGMENT* beziers, UINT beziersCount) {
    OC__PathSink*     this = (OC__PathSink*)This;
    oc__path_felement element = { 0 };

    if (oc__unlikely(this->doomed)) {
        return;
    }

    for (UINT32 i = 0; i < beziersCount; i++) {
        D2D1_POINT_2F pt1 = beziers[i].point1;
        D2D1_POINT_2F pt2 = beziers[i].point2;
        D2D1_POINT_2F pt3 = beziers[i].point3;

        D2D1_POINT_2F origin = this->origin;

        float cx1 = (3.0f * pt1.x - origin.x) * 0.5f;
        float cy1 = (3.0f * pt1.y - origin.y) * 0.5f;

        float cx2 = (3.0f * pt2.x - pt3.x) * 0.5f;
        float cy2 = (3.0f * pt2.y - pt3.y) * 0.5f;

        float dx = cx1 - cx2;
        float dy = cy1 - cy2;

        if (dx * dx + dy * dy < 0.05f * 0.05f) {
            element.pt1 = (D2D1_POINT_2F) { roundf(cx2), roundf(cy2) };
            element.pt2 = pt3;
            element.type = oc__path_conic;
        } else {
            element.pt1 = pt1;
            element.pt2 = pt2;
            element.pt3 = pt3;
            element.type = oc__path_cubic;
        }

        if (oc__unlikely(this->doomed = oc__walk_outline(this, &element))) {
            break;
        }

        this->origin = pt3;
    }
}

static void STDMETHODCALLTYPE
OC__PathSink_AddLines(ID2D1SimplifiedGeometrySink* This, const D2D1_POINT_2F* points, UINT pointsCount) {
    OC__PathSink*     this = (OC__PathSink*)This;
    oc__path_felement element = { 0 };

    if (oc__unlikely(this->doomed)) {
        return;
    }

    element.type = oc__path_line;
    for (UINT32 i = 0; i < pointsCount; i++) {
        element.pt1 = points[i];

        if (oc__unlikely(this->doomed = oc__walk_outline(this, &element))) {
            break;
        }

        this->origin = points[i];
    }
}

static void STDMETHODCALLTYPE
OC__PathSink_BeginFigure(ID2D1SimplifiedGeometrySink* This, D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) {
    OC__PathSink*     this = (OC__PathSink*)This;
    oc__path_felement element = { 0 };

    (void)figureBegin;

    if (oc__unlikely(this->doomed)) {
        return;
    }

    element.type = oc__path_move;
    element.pt1 = startPoint;

    this->doomed = oc__walk_outline(this, &element);

    this->origin = startPoint;
    this->start = startPoint;
}

static void STDMETHODCALLTYPE
OC__PathSink_SetSegmentFlags(ID2D1SimplifiedGeometrySink* This, D2D1_PATH_SEGMENT vertexFlags) {
    (void)This;
    (void)vertexFlags;
}

static void STDMETHODCALLTYPE
OC__PathSink_SetFillMode(ID2D1SimplifiedGeometrySink* This, D2D1_FILL_MODE fillMode) {
    (void)This;
    (void)fillMode;
}

static ULONG STDMETHODCALLTYPE
OC__PathSink_Release(IUnknown* This) {
    OC__PathSink* this = (OC__PathSink*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    assert(refs != -1);
    return refs;
}

static ULONG STDMETHODCALLTYPE
OC__PathSink_AddRef(IUnknown* This) {
    OC__PathSink* this = (OC__PathSink*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
OC__PathSink_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader)) {
        OC__PathSink_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const ID2D1SimplifiedGeometrySinkVtbl OC__PathSinkVtbl = {
    { OC__PathSink_QueryInterface,
        OC__PathSink_AddRef,
        OC__PathSink_Release },
    OC__PathSink_SetFillMode,
    OC__PathSink_SetSegmentFlags,
    OC__PathSink_BeginFigure,
    OC__PathSink_AddLines,
    OC__PathSink_AddBeziers,
    OC__PathSink_EndFigure,
    OC__PathSink_Close,
};

oc_error ocl_get_outline(const oc_face* face, uint16_t index, oc_load_flags flags, oc_outline* ooutline) {
    HRESULT          hr;
    ULONG            refs;
    IDWriteFontFace* dw_face;
    UINT16           count;

    oc_error err = oc_error_ok;
    oc_outline outline = { 0 };

    OC__PathSink sink = { 0 };

    (void)flags;

    if (!(face && ooutline)) {
        return oc_error_invalid_param;
    }

    // todo: cache nglyphs
    dw_face = face->impl->dw_face;
    count = dw_face->lpVtbl->GetGlyphCount(dw_face);

    if (index >= count) {
        oc__exit(oc_error_invalid_param);
    }

    sink.lpVtbl = &OC__PathSinkVtbl;
    sink.element.type = -1;
    sink.ref_count = 1;

    hr = face->impl->dw_face->lpVtbl->GetGlyphRunOutline(
        face->impl->dw_face,
        face->upem,
        &index,
        NULL,
        NULL,
        1,
        FALSE,
        FALSE,
        (IDWriteGeometrySink*)&sink);

    if (hr != S_OK) {
        oc__exit(oc__unexpected(hr));
    }

    refs = sink.lpVtbl->Base.Release((IUnknown*)&sink);

    (void)refs;
    assert(refs == 0);

    if (oc__unlikely(sink.doomed)) {
        oc__free(sink.tags);
        oc__free(sink.points);
        oc__free(sink.contours);

        oc__exit(oc_error_out_of_memory);
    }

    // todo: catch and handle oom
    if (oc__len(sink.points) > 0) {
        assert(sink.element.type == oc__path_close);
        oc__append(sink.contours, oc__len(sink.points) - 1);
    }

    outline.npoints = (uint16_t)oc__len(sink.points);
    outline.ncontours = (uint16_t)oc__len(sink.contours);

    outline.tags = sink.tags;
    outline.points = sink.points;
    outline.contours = sink.contours;
exit:
    if (ooutline != NULL)
        *ooutline = outline;

    return err;
}

void ocl_free_outline(oc_outline* outline) {
    if (outline == NULL) {
        return;
    }

    oc__free(outline->tags);
    oc__free(outline->points);
    oc__free(outline->contours);

    memset(outline, 0, sizeof(*outline));
}

oc_error ocl_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t pitch) {
    oc_error err = oc_error_ok;
    HRESULT  dw_err = S_OK;

    IDWriteFontFace* dw_face;
    IDWriteFactory*  dw_factory;

    oc_bbox cbox;
    oc_bbox pbox;

    DWRITE_MATRIX transform;
    UINT16        count;
    RECT          bounds;

    IDWriteGlyphRunAnalysis* analysis = NULL;
    DWRITE_GLYPH_RUN         glyph_run = { 0 };
    oc_extent                extent = { 0 };

    uint8_t* bitmap = NULL;
    size_t   length;

    uint8_t* idx;

    if (!(face && oextent)) {
        oc__exit(oc_error_invalid_param);
    }

    dw_face = face->impl->dw_face;
    dw_factory = face->impl->dw_factory;
    count = dw_face->lpVtbl->GetGlyphCount(dw_face);

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    if (index >= count) {
        oc__exit(oc_error_invalid_param);
    }

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L414
    ocl_get_glyph_cbox(face, index, OC_LOAD_DEFAULT, &cbox);

    pbox.min_x = cbox.min_x >> 6;
    pbox.min_y = cbox.min_y >> 6;
    pbox.max_x = cbox.max_x >> 6;
    pbox.max_y = cbox.max_y >> 6;

    // take fractional part and ceil it
    pbox.max_x += ((cbox.max_x & 63) + 63) >> 6;
    pbox.max_y += ((cbox.max_y & 63) + 63) >> 6;

    extent.rows = pbox.max_y - pbox.min_y;
    extent.cols = pbox.max_x - pbox.min_x;

    if (buffer == NULL) {
        goto exit;
    }

    if (extent.rows == 0 || extent.cols == 0) {
        goto exit;
    }

    transform.m11 = 1.0f;
    transform.m12 = 0.0f;
    transform.m21 = 0.0f;
    transform.m22 = 1.0f;
    transform.dx = (cbox.min_x & 63) / 64.0f;
    transform.dy = -(cbox.min_y & 63) / 64.0f;

    glyph_run.fontFace = dw_face;
    glyph_run.fontEmSize = oc_mul_16p16(face->upem, face->size.scale) / 64.0f;
    glyph_run.glyphCount = 1;
    glyph_run.glyphIndices = &index;

    // todo: when no hinting use dwrite_2.h
    // wih DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE

    dw_err = dw_factory->lpVtbl->CreateGlyphRunAnalysis(
        dw_factory,
        &glyph_run,
        1.0f,
        &transform,
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
        DWRITE_MEASURING_MODE_NATURAL,
        -cbox.min_x / 64.0f,
        cbox.min_y / 64.0f,
        &analysis);
    switch (dw_err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(err));
    }

    length = (size_t)extent.rows * (size_t)extent.cols * 3;
    bitmap = malloc(length);

    if (bitmap == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    bounds.left = 0;
    bounds.bottom = 0;

    bounds.top = -(int32_t)extent.rows;
    bounds.right = (int32_t)extent.cols;

    err = analysis->lpVtbl->CreateAlphaTexture(
        analysis,
        DWRITE_TEXTURE_CLEARTYPE_3x1,
        &bounds,
        bitmap,
        length);

    if (err != S_OK) {
        oc__exit(oc__unexpected(err));
    }

    idx = bitmap;
    for (uint32_t y = 0; y < extent.rows; y++) {
        for (uint32_t x = 0; x < extent.cols; x++) {
            uint8_t r = *idx++;
            uint8_t g = *idx++;
            uint8_t b = *idx++;

            buffer[y * pitch + x] = (r + b + g) / 3;
        }
    }
exit:
    if (bitmap)
        free(bitmap);
    if (analysis)
        analysis->lpVtbl->Release(analysis);
    if (oextent)
        *oextent = extent;
    return err;
}
#endif /* ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
#include <initguid.h>

#include <dwrite.h>

struct oc_collection_impl {
    const oc_library* oc_library;
    char**            families;
    UINT32            nfamilies;
};

typedef struct {
    const oc_library* oc_library;
    IDWriteFont*      dw_font;
    oc_font           font;
} oc__font_impl;

#ifndef OC__OVERRIDE_LIBRARY_IMPL
#define OC__OVERRIDE_LIBRARY_IMPL

oc_error oc_init_library(oc_library** olibrary) {
    HRESULT         result;
    IDWriteFactory* dw_factory;

    if (olibrary == NULL) {
        return oc_error_invalid_param;
    }

    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&dw_factory);
    switch (result) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(result);
    }

    *olibrary = (oc_library*)dw_factory;
    return oc_error_ok;
}

void oc_free_library(oc_library* library) {
    IDWriteFactory* dw_factory;

    if (!library) {
        return;
    }

    dw_factory = (IDWriteFactory*)library;
    dw_factory->lpVtbl->Release(dw_factory);
}
#endif

static inline void oc__free_font(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    impl->dw_font->lpVtbl->Release(impl->dw_font);
    free(impl);
}

oc_error ocf_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_collection collection = { 0 };
    oc_error      err = oc_error_ok;

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    collection.impl = calloc(1, sizeof(oc_collection_impl));
    if (collection.impl == NULL) {
        return oc_error_out_of_memory;
    }

    collection.impl->oc_library = library;
exit:
    if (ocollection)
        *ocollection = collection;
    return err;
}

void ocf_free_collection(oc_collection* collection) {
    if (collection) {
        while (collection->nfonts--) {
            oc__free_font(collection->fonts[collection->nfonts]);
        }

        while (collection->impl->nfamilies--) {
            free(collection->impl->families[collection->impl->nfamilies]);
        }

        free(collection->fonts);
        free(collection->impl->families);
        free(collection->impl);

        memset(collection, 0, sizeof(*collection));
    }
}

static const oc_slant oc__slant_map[] = {
    [DWRITE_FONT_STYLE_NORMAL] = oc_slant_roman,
    [DWRITE_FONT_STYLE_OBLIQUE] = oc_slant_oblique,
    [DWRITE_FONT_STYLE_ITALIC] = oc_slant_italic,
};

static oc_font* oc__init_font(const oc_library* oc_library, IDWriteFont* dw_font, const char* family) {
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE  style;

    oc__font_impl* impl;

    weight = dw_font->lpVtbl->GetWeight(dw_font);
    style = dw_font->lpVtbl->GetStyle(dw_font);

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        return NULL;
    }

    impl->oc_library = oc_library;
    impl->dw_font = dw_font;
    impl->font.family = family;
    impl->font.weight = (uint16_t)weight;
    impl->font.slant = oc__slant_map[style];

    return &impl->font;
}

oc_error ocf_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;
    HRESULT  hr;

    const oc_library* oc_library;

    IDWriteFactory*        dw_factory;
    IDWriteFontCollection* dw_collection = NULL;

    UINT32 family_count;
    UINT32 font_count;

    union {
        char*  str;
        UINT32 len;
    }*     families = NULL;
    UINT32 nfamilies = 0;

    WCHAR* wide_buf = NULL;
    UINT32 wide_buf_len;

    oc_font** fonts = NULL;
    uint32_t  nfonts = 0;

    oc_collection      tmp_collection;
    oc_collection_impl tmp_impl;

    if (!collection) {
        oc__exit(oc_error_invalid_param);
    }

    oc_library = collection->impl->oc_library;
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    dw_factory = oc_library->dw_factory;
#else
    dw_factory = (IDWriteFactory*)oc_library;
#endif
    hr = dw_factory->lpVtbl->GetSystemFontCollection(dw_factory, &dw_collection, TRUE);

    switch (hr) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(hr));
    }

    family_count = dw_collection->lpVtbl->GetFontFamilyCount(dw_collection);
    if (family_count == 0) {
        goto done;
    }

    font_count = 0;
    families = malloc(family_count * sizeof(*families));

    if (families == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    for (UINT32 i = 0; i < family_count; i++) {
        IDWriteFontFamily*       family;
        IDWriteLocalizedStrings* names;
        UINT32                   length;

        hr = dw_collection->lpVtbl->GetFontFamily(dw_collection, i, &family);
        assert(hr == S_OK);

        hr = family->lpVtbl->GetFamilyNames(family, &names);
        assert(hr == S_OK);

        hr = names->lpVtbl->GetStringLength(names, 0, &length);
        assert(hr == S_OK);

        families[i].len = length;
        wide_buf_len = OC__MAX(wide_buf_len, length);
        font_count += family->lpVtbl->GetFontCount(family);

        names->lpVtbl->Release(names);
        family->lpVtbl->Release(family);
    }

    assert(font_count > 0);

    wide_buf = malloc((wide_buf_len + 1) * sizeof(WCHAR));
    if (wide_buf == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    fonts = malloc(sizeof(*fonts) * font_count);
    if (fonts == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    for (UINT32 i = 0; i < family_count; i++) {
        IDWriteFontFamily*       font_family;
        IDWriteLocalizedStrings* names;

        UINT32 wide_length = families[i].len;
        UINT32 font_index;

        char* family;
        int   length;

        hr = dw_collection->lpVtbl->GetFontFamily(dw_collection, i, &font_family);
        assert(hr == S_OK);

        hr = font_family->lpVtbl->GetFamilyNames(font_family, &names);
        assert(hr == S_OK);

        hr = names->lpVtbl->GetString(names, 0, wide_buf, wide_length + 1);
        names->lpVtbl->Release(names);
        assert(hr == S_OK);

        length = WideCharToMultiByte(
            CP_UTF8,
            0,
            wide_buf,
            wide_length,
            NULL,
            0,
            NULL,
            NULL);

        assert(length > 0);

        family = malloc(length + 1);
        if (family == NULL) {
            font_family->lpVtbl->Release(font_family);
            oc__exit(oc_error_out_of_memory);
        }

        length = WideCharToMultiByte(
            CP_UTF8,
            0,
            wide_buf,
            wide_length,
            family,
            length,
            NULL,
            NULL);

        assert(length > 0);

        family[length] = '\0';
        families[nfamilies++].str = family;
        font_index = font_family->lpVtbl->GetFontCount(font_family);

        while (font_index--) {
            IDWriteFont* dw_font;
            oc_font*     font;

            hr = font_family->lpVtbl->GetFont(font_family, font_index, &dw_font);
            assert(hr == S_OK);

            font = oc__init_font(oc_library, dw_font, family);
            if (font == NULL) {
                font_family->lpVtbl->Release(font_family);
                oc__exit(oc_error_out_of_memory);
            }

            fonts[nfonts++] = font;
        }

        font_family->lpVtbl->Release(font_family);
    }
done:
    tmp_impl.oc_library = oc_library;
    tmp_impl.families = (char**)families;
    tmp_impl.nfamilies = nfamilies;

    tmp_collection.impl = collection->impl;
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

    families = (void*)collection->impl->families;
    nfamilies = collection->impl->nfamilies;
    fonts = collection->fonts;
    nfonts = collection->nfonts;

    *collection->impl = tmp_impl;
    *collection = tmp_collection;
exit:
    while (nfonts--)
        oc__free_font(fonts[nfonts]);
    while (nfamilies--)
        free(families[nfamilies].str);

    if (dw_collection)
        dw_collection->lpVtbl->Release(dw_collection);

    free(fonts);
    free(families);
    free(wide_buf);

    return err;
}

bool ocf_has_character(const oc_font* font, uint32_t character) {
    oc__font_impl* impl;
    IDWriteFont*   dw_font;

    HRESULT result;
    WINBOOL exists;

    if (!font) {
        return false;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    dw_font = impl->dw_font;

    result = dw_font->lpVtbl->HasCharacter(dw_font, character, &exists);
    return result == S_OK && exists;
}

#if defined(ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION)
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc_error err;
    HRESULT  result;

    oc__font_impl* impl;

    IDWriteFactory*  dw_factory;
    IDWriteFontFace* dw_face;

    oc_face face = { 0 };

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    dw_factory = (IDWriteFactory*)impl->oc_library; // this is true
    result = impl->dw_font->lpVtbl->CreateFontFace(impl->dw_font, &dw_face);

    switch (result) {
    case S_OK:
        break;
    default:
        oc__exit(oc__unexpected(result));
    }

    if (desired_size == 0) {
        desired_size = 12 << 6;
    } else if (desired_size < 1 << 6) {
        desired_size = 1 << 6;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    err = oc__init_face(dw_factory, dw_face, desired_size, dpi, &face);
exit:
    *oface = face;
    return err;
}
#elif defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION)
static void ocf__stream_close(FT_Stream stream) {
    IDWriteFontFileStream* dw_stream;

    assert(stream != NULL);

    dw_stream = (IDWriteFontFileStream*)stream->descriptor.pointer;
    dw_stream->lpVtbl->Release(dw_stream);

    free(stream);
}

static unsigned long ocf__stream_read(
    FT_Stream      stream,
    unsigned long  offset,
    unsigned char* buffer,
    unsigned long  count) {
    IDWriteFontFileStream* dw_stream;
    HRESULT                result;

    const void* fragement_start;
    void*       fragement_context;

    assert(stream != NULL);
    assert(stream->size >= offset && stream->size - offset >= count);

    if (count == 0) {
        return 0;
    }

    dw_stream = (IDWriteFontFileStream*)stream->descriptor.pointer;
    result = dw_stream->lpVtbl->ReadFileFragment(
        dw_stream,
        &fragement_start,
        offset,
        count,
        &fragement_context);

    if (result != S_OK) {
        return 0;
    }

    memcpy(buffer, fragement_start, count);
    dw_stream->lpVtbl->ReleaseFileFragment(dw_stream, fragement_context);

    return count;
}

oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;

    HRESULT  result;
    oc_error err;

    IDWriteFontFace* dw_face;
    IDWriteFontFile* dw_file;

    IDWriteFontFileLoader* dw_loader;
    IDWriteFontFileStream* dw_stream;

    const void* key;
    UINT32      key_size;

    UINT64 file_size;
    UINT32 nfiles = 1;

    oc_face face = { 0 };

    FT_Open_Args args = { 0 };
    FT_Stream    stream;

    FT_Face  ft_face;
    FT_Error ft_err;

    oc_open_params params;

    impl = oc__parentof(oc__font_impl, font, font);
    result = impl->dw_font->lpVtbl->CreateFontFace(impl->dw_font, &dw_face);

    switch (result) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(result));
    }

    result = dw_face->lpVtbl->GetFiles(dw_face, &nfiles, &dw_file);
    dw_face->lpVtbl->Release(dw_face);

    switch (result) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(result));
    }

    result = dw_file->lpVtbl->GetReferenceKey(dw_file, &key, &key_size);
    assert(result == S_OK);

    result = dw_file->lpVtbl->GetLoader(dw_file, &dw_loader);
    dw_file->lpVtbl->Release(dw_file);

    assert(result == S_OK);

    result = dw_loader->lpVtbl->CreateStreamFromKey(dw_loader, key, key_size, &dw_stream);
    dw_loader->lpVtbl->Release(dw_loader);

    switch (result) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(result));
    }

    result = dw_stream->lpVtbl->GetFileSize(dw_stream, &file_size);
    if (result != S_OK) {
        dw_stream->lpVtbl->Release(dw_stream);
        oc__exit(oc__unexpected(result));
    }

    stream = calloc(1, sizeof(*stream));
    if (!stream) {
        dw_stream->lpVtbl->Release(dw_stream);
        oc__exit(oc_error_out_of_memory);
    }

    stream->descriptor.pointer = dw_stream;
    stream->read = &ocf__stream_read;
    stream->close = &ocf__stream_close;
    stream->size = (unsigned long)file_size;

    args.flags = FT_OPEN_STREAM;
    args.stream = stream;

    ft_err = FT_Open_Face(impl->oc_library->ft_library, &args, 0, &ft_face);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        oc__exit(oc_error_out_of_memory);
    case FT_Err_Invalid_File_Format:
    case FT_Err_Unknown_File_Format:
    case FT_Err_Invalid_Stream_Operation:
        oc__exit(oc_error_failed_to_open);
    default:
        oc__exit(oc__unexpected(ft_err));
    }

    params.face_index = 0;
    params.desired_size = desired_size;
    params.dpi = dpi;

    params = oc__open_params_defaults(&params);
    err = oc__init_face(ft_face, &params, &face);

    if (err != oc_error_ok) {
        FT_Done_Face(ft_face);
    }
exit:
    *oface = face;
    return err;
}
#endif

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    HRESULT        result;

    IDWriteFontFace* face;
    IDWriteFontFile* file;

    UINT32 nfiles;

    const void* key;
    UINT32      key_size;

    IDWriteFontFileLoader*      loader;
    IDWriteLocalFontFileLoader* local_loader;

    WCHAR* wide_path;
    UINT32 wide_len;

    int    path_len;
    size_t copy_len;

    if (!font) {
        return 0;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    result = impl->dw_font->lpVtbl->CreateFontFace(impl->dw_font, &face);

    if (result != S_OK) {
        return 0;
    }

    nfiles = 1;
    result = face->lpVtbl->GetFiles(face, &nfiles, &file);
    face->lpVtbl->Release(face);

    if (result != S_OK || nfiles == 0) {
        return 0;
    }

    result = file->lpVtbl->GetReferenceKey(file, &key, &key_size);
    assert(result == S_OK);

    result = file->lpVtbl->GetLoader(file, &loader);
    file->lpVtbl->Release(file);

    assert(result == S_OK);

    result = loader->lpVtbl->QueryInterface(
        loader,
        &IID_IDWriteLocalFontFileLoader,
        (void**)&local_loader);
    loader->lpVtbl->Release(loader);

    if (result != S_OK) {
        return 0;
    }

    result = local_loader->lpVtbl->GetFilePathLengthFromKey(
        local_loader,
        key,
        key_size,
        &wide_len);

    assert(result == S_OK);
    if (wide_len == 0) {
        local_loader->lpVtbl->Release(local_loader);
        return 0;
    }

    wide_path = malloc((wide_len + 1) * sizeof(WCHAR));
    if (wide_path == NULL) {
        local_loader->lpVtbl->Release(local_loader);
        return 0;
    }

    result = local_loader->lpVtbl->GetFilePathFromKey(
        local_loader,
        key,
        key_size,
        wide_path,
        wide_len + 1);

    local_loader->lpVtbl->Release(local_loader);
    assert(result == S_OK);

    path_len = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide_path,
        wide_len,
        NULL,
        0,
        NULL,
        NULL);

    assert(path_len > 0);
    if (len == 0) {
        free(wide_path);
        return path_len;
    }

    copy_len = len < (size_t)path_len ? len : (size_t)path_len;
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide_path,
        wide_len,
        buf,
        (int)copy_len,
        NULL,
        NULL);

    free(wide_path);
    return copy_len;
}
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
