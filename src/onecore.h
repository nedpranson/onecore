#ifndef INCLUDE_ONECORE_H
#define INCLUDE_ONECORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t oc_tag;
typedef uint32_t oc_load_flags;
typedef int32_t  oc_16p16;
typedef int32_t  oc_26p6;
// typedef int16_t oc_10p6;

#ifndef OCDEF
#define OCDEF
#endif

#define OC_LOAD_DEFAULT 0x0
#define OC_LOAD_NO_SCALE (1l << 0)
#define OC_LOAD_NO_HINTING (1l << 1)
// todo (stage 2): add these flags
// #define OC_LOAD_VERTICAL (1l << 2)
// #define OC_LOAD_COLOR (1l << 3)
#define OC_LOAD_NO_FITTING (1l << 4)

#define OC_ERROR_LIST                                      \
    X(oc_error_ok, "no error")                             \
    X(oc_error_invalid_param, "invalid parameter")         \
    X(oc_error_table_missing, "table is missing")          \
    X(oc_error_out_of_memory, "out of memory")             \
    X(oc_error_failed_to_open, "failed to open")           \
    X(oc_error_insufficient_buffer, "insufficient buffer") \
    X(oc_error_invalid_pixel_size, "invalid pixel size")   \
    X(oc_error_unexpected, "unexpected error")

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

// todo: document every struct field and method

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
    uint32_t face_index;
    oc_26p6  desired_size;
    uint16_t dpi;
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
    int32_t x;
    int32_t y;
} oc_point;

typedef void (*oc_outline_start_figure)(oc_point at, void* context);
typedef void (*oc_outline_end_figure)(void* context);
typedef void (*oc_outline_line_to)(oc_point to, void* context);
typedef void (*oc_outline_cubic_to)(oc_point c1, oc_point c2, oc_point to, void* context);

typedef struct {
    oc_outline_start_figure start_figure;
    oc_outline_end_figure   end_figure;
    oc_outline_line_to      line_to;
    oc_outline_cubic_to     cubic_to;
} oc_outline_funcs;

// typedef struct oc_library_impl oc_library_impl;
typedef struct oc_face_impl       oc_face_impl;
typedef struct oc_collection_impl oc_collection_impl;
// typedef struct oc_font oc_font;

// todo (stage 2): integrate even more fields
// todo (stage 2): need a way to know homy mady glyphs a font has
typedef struct {
    // todo: check if family names change on diff locales
    const char* family;
    oc_slant    slant;
    uint16_t    weight;
    // -> langs
    // -> way to get ?path
    // -> way to open oc_font
    // -> monoscope
} oc_font;

typedef struct {
    // oc_library_impl* impl;
    void* internals;
} oc_library;

typedef struct {
    oc_face_impl* impl;

    oc_size  size;
    uint16_t upem; /* units per EM */
    uint16_t ascent;
    uint16_t descent;
    int16_t  leading;
    int16_t  underline_position;
    uint16_t underline_thickness;
} oc_face;

typedef struct {
    oc_collection_impl* impl;

    oc_font** fonts;
    uint32_t  nfonts;
} oc_collection;

/*
 * Initializes a new onecore library instance.
 * Call `oc_free_library` to release retrieved resource.
 */
OCDEF oc_error
oc_init_library(oc_library* olibrary);

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

OCDEF oc_error
ocf_load_fonts(oc_collection* collection);

OCDEF bool
ocf_has_character(const oc_font* font, uint32_t character);

OCDEF size_t
ocf_copy_path(const oc_font* font, char* buf, size_t len);

/*
 * Opens a font.
 * Call `ocl_free_face` to release retrieved resource.
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

// todo: give a warning that this function is not thread safe
OCDEF oc_error
ocl_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi);

/*
 * Returns the glyph index of a given character code.
 */
OCDEF uint16_t
ocl_get_char_index(const oc_face* face, uint32_t charcode);

OCDEF void
ocl_get_glyph_metrics(
    const oc_face*    face,
    uint16_t          index,
    oc_load_flags     flags,
    oc_glyph_metrics* ometrics);

OCDEF void
ocl_get_glyph_cbox(
    const oc_face* face,
    uint16_t       index,
    oc_load_flags  flags,
    oc_bbox*       ocbox);

// todo: add comments here explaining that every backend will generate diffrent glyph textures
//       so if u want it modified by every backend it would be recomended to raster it using glyph outlines
// todo (stage 2): now we're rendering these glyphs from [0;0] position which is convenient, but it does lose some extra draw data
//       make so an user could specify how to draw this glyph mb allow to pass matricies and origins mb just some flags??
// todo (stage 2): it is needed to make this method more complicated, now we cannot pass origin where to draw or matricies, nothing
//
// roadmap:
// dwrite and coretext knows how to draw bezier curves hence theoretically hinting can be achieved with manual shapes rasterization,
// essentially onecore would become freetype, but with native font file parsing and rendering engine
OCDEF oc_error
ocl_render_glyph(
    const oc_face* face,
    uint16_t       index,
    oc_extent*     oextent,
    uint8_t*       buffer,
    size_t         buffer_size);

// todo (stage 2): renew this impl
OCDEF bool
ocl_get_outline(
    const oc_face*          face,
    uint16_t                index,
    const oc_outline_funcs* funcs,
    void*                   user);

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
oc_div_16p16(oc_16p16 a, oc_16p16 b);

/*
 * Computes `(a*0x10000)/b` with maximum accuracy.
 */
OCDEF oc_16p16
oc_mul_16p16(oc_16p16 a, oc_16p16 b);

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
#define ONECORE_SHARED_IMPLEMENTATION
#if defined(_MSC_VER) || defined(__MINGW32__)
#define ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION
#elif defined(__APPLE__)
#define ONECORE_CORETEXT_LOADER_IMPLEMENTATION
#else
#define ONECORE_FREETYPE_LOADER_IMPLEMENTATION
#endif
#endif /* ONECORE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_FINDER_IMPLEMENTATION
#define ONECORE_SHARED_IMPLEMENTATION
#if defined(_MSC_VER) || defined(__MINGW32__)
#define ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
#elif defined(__APPLE__)
#define ONECORE_CORETEXT_FINDER_IMPLEMENTATION
#else
#define ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION
#endif
#endif /* ONECORE_FINDER_IMPLEMENTATION */

#ifdef ONECORE_SHARED_IMPLEMENTATION
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
#endif /* ONECORE_SHARED_IMPLEMENTATION */

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
#endif /* ONECORE_FREETYPE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION
#endif /* ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_LOADER_IMPLEMENTATION
#endif /* ONECORE_CORETEXT_LOADER_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_FINDER_IMPLEMENTATION
#endif /* ONECORE_CORETEXT_LINDER_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION
#endif /* ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION
#endif /* ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION */
