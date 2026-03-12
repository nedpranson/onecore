#ifndef INCLUDE_ONECORE_H
#define INCLUDE_ONECORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t oc_tag;
typedef uint32_t oc_load_flags;
typedef int32_t oc_16p16;
typedef int32_t oc_26p6;
// typedef int16_t oc_10p6;

#ifndef OC_PUBLIC
#define OC_PUBLIC
#endif /* OC_PUBLIC */

// todo: add some font collections FontConfig, DWRITE, CORETEXT
//       add ability to filter and sort fonts, say by codepoints, families, sizes

#define OC_LOAD_DEFAULT 0x0
#define OC_LOAD_NO_SCALE (1l << 0)
// #define OC_LOAD_VERTICAL (1l << 1)
// #define OC_LOAD_COLOR (1l << 2)
// #define OC_LOAD_NO_HINTING (1l << 3)
// todo: add hinting https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L861 (grid fitting)
//       hintign it self is a hard problem to solve

#define OC_ERROR_LIST                                      \
    X(oc_error_ok, "no error")                             \
    X(oc_error_invalid_param, "invalid parameter")         \
    X(oc_error_table_missing, "table is missing")          \
    X(oc_error_out_of_memory, "out of memory")             \
    X(oc_error_failed_to_open, "failed to open")           \
    X(oc_error_insufficient_buffer, "insufficient buffer") \
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

typedef enum {
#define X(e, s) e,
    OC_ERROR_LIST
#undef X
} oc_error;

typedef struct {
    uint32_t face_index;
    oc_26p6 desired_size;
    short dpi;
} oc_open_params;

// todo: add height which is just ascept + descent + leading
// todo: rename to oc_face_metrics
typedef struct {
    uint16_t upem;
    uint16_t ppem; // todo: put this into some oc_scale struct
    oc_16p16 scale; // todo: put this into some oc_scale struct
    uint16_t ascent;
    uint16_t descent;
    int16_t leading;
    int16_t underline_position;
    uint16_t underline_thickness;
} oc_font_metrics;

typedef struct {
    oc_26p6 width;
    oc_26p6 height;
    oc_26p6 bearing_x;
    oc_26p6 bearing_y;
    oc_26p6 advance;
} oc_glyph_metrics;

typedef struct {
    uint32_t rows; // uint16_t??
    uint32_t cols; // uint16_t??
} oc_extent;

typedef struct {
    oc_26p6 min_x;
    oc_26p6 min_y;
    oc_26p6 max_x;
    oc_26p6 max_y;
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
    oc_outline_end_figure end_figure;
    oc_outline_line_to line_to;
    oc_outline_cubic_to cubic_to;
} oc_outline_funcs;

// typedef struct oc_library_impl oc_library_impl;
typedef struct oc_face_impl oc_face_impl;
typedef struct oc_collection_impl oc_collection_impl;
//typedef struct oc_font oc_font;

typedef struct {
    const char* path;
    const char* family;
    uint16_t weight;
} oc_font;

typedef struct {
    void* internals;
    // oc_library_impl* impl;
} oc_library;

typedef struct {
    oc_face_impl* impl;
    oc_font_metrics metrics;
} oc_face;

typedef struct {
    oc_collection_impl* impl;
    oc_font** fonts;
    size_t elements; // todo: use 4 bytes
} oc_collection;

typedef struct {
    const void* data;
    size_t size;
} oc_table;

// todo: we need better naming as now we have two seperate project in one lib:
// * discovery
// * loader/parser

OC_PUBLIC oc_error
oc_init_library(oc_library* olibrary);

OC_PUBLIC void
oc_free_library(oc_library* library);

OC_PUBLIC oc_error
oc_init_collection(const oc_library* library, oc_collection* ocollection);

OC_PUBLIC void
oc_free_collection(oc_collection* collection);

OC_PUBLIC oc_error
oc_load_fonts(oc_collection* collection);

// OC_PUBLIC int
// oc_get_weight(const oc_font* font);

OC_PUBLIC oc_error
oc_get_path(const oc_font* font, char* buffer, size_t buffer_size);

OC_PUBLIC oc_error
oc_open_face(
    const oc_library* library,
    const char* path,
    const oc_open_params* uparams,
    oc_face* face);

/*
 * @note:
 *   You must not deallocate the memory before calling @oc_free_face.
 */
OC_PUBLIC oc_error
oc_open_memory_face(
    const oc_library* library,
    const void* data,
    size_t data_size,
    const oc_open_params* uparams,
    oc_face* oface);

OC_PUBLIC void
oc_free_face(oc_face* face);

// todo: give a warning that this function is not thread safe
OC_PUBLIC oc_error
oc_set_size(oc_face* face, oc_26p6 desired_size, short dpi);

OC_PUBLIC uint16_t
oc_get_char_index(const oc_face* face, uint32_t charcode);

OC_PUBLIC void
oc_get_glyph_metrics(
    const oc_face* face,
    uint16_t index,
    oc_load_flags flags,
    oc_glyph_metrics* ometrics);

// todo: rename to oc_get_glyph_cbox
OC_PUBLIC void
oc_get_glyph_cbox(
    const oc_face* face,
    uint16_t index,
    oc_load_flags flags,
    oc_bbox* ocbox);

// todo: add comments here explaining that every backend will generate diffrent glyph textures
//       so if u want it modified by every backend it would be recomended to raster it using glyph outlines
// todo: now we're rendering these glyphs from [0;0] position which is convenient, but it does lose some extra draw data
//       make so an user could specify how to draw this glyph mb allow to pass matricies and origins mb just some flags??
// todo: it is needed to make this method more complicated, now we cannot pass origin where to draw or matricies, nothing
OC_PUBLIC oc_error
oc_render_glyph(
    const oc_face* face,
    uint16_t index,
    oc_extent* oextent,
    uint8_t* buffer,
    size_t buffer_size);

OC_PUBLIC bool
oc_get_outline(
    const oc_face* face,
    uint16_t index,
    const oc_outline_funcs* funcs,
    void* user);

// todo: copy variant would be nice which we would not need to free
OC_PUBLIC oc_error
oc_get_sfnt_table(
    const oc_face* face,
    oc_tag tag,
    oc_table* otable,
    void** ocontext);

OC_PUBLIC void
oc_free_table(const oc_face* face, void* context);

OC_PUBLIC
const char* oc_strerror(oc_error err);

OC_PUBLIC oc_16p16
oc_div_16p16(oc_16p16 a, oc_16p16 b);

OC_PUBLIC oc_16p16
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

// these defines should work fine
// but should we even allow this?
// if someone would be using them it would just defeat the purpose of this lib
// ONECORE_FORCE_FREETYPE
// ONECORE_FORCE_FONTCONFIG

#ifdef ONECORE_IMPLEMENTATION

#if defined(_MSC_VER) || defined(__MINGW32__)
#define ONECORE_DIRECTWRITE_IMPLEMENTATION
#elif defined(__APPLE__)
#define ONECORE_CORETEXT_IMPLEMENTATION
#else
#define ONECORE_FREETYPE_IMPLEMENTATION
#endif

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
    bool s = false;
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

// todo: make default dpi to 72
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

    if (params.dpi <= 0) {
        params.dpi = 72;
    }

    return params;
}

#ifdef ONECORE_FREETYPE_IMPLEMENTATION
#endif /* ONECORE_FREETYPE_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_IMPLEMENTATION
#endif /* ONECORE_CORETEXT_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_IMPLEMENTATION
#endif /* ONECORE_DIRECTWRITE_IMPLEMENTATION */

#endif /* ONECORE_IMPLEMENTATION */
