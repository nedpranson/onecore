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

#define OC_WEIGHT_THIN 0
#define OC_WEIGHT_EXTRALIGHT 40
#define OC_WEIGHT_LIGHT 50
#define OC_WEIGHT_SEMILIGHT 55
#define OC_WEIGHT_BOOK 75
#define OC_WEIGHT_REGULAR 80
#define FC_WEIGHT_MEDIUM 100
#define OC_WEIGHT_DEMIBOLD 180
#define OC_WEIGHT_BOLD 200
#define OC_WEIGHT_EXTRABOLD 205
#define OC_WEIGHT_BLACK 210
#define OC_WEIGHT_EXTRABLACK 215

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
    uint16_t ppem;
    oc_16p16 scale;
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
} oc_size;

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

typedef struct {
    void* internals;
} oc_library;

typedef struct oc_face_impl oc_face_impl;

typedef struct {
    oc_face_impl* impl;
    oc_font_metrics metrics;
} oc_face;

typedef struct {
    void* internals;
} oc_font;

typedef struct {
    const void* data;
    size_t size;
} oc_table;

// typedef struct {
//     const char* family;
//     uint8_t weight;
//     // flags for bold | italic
// } oc_discovery_params;

OC_PUBLIC oc_error
oc_init_library(oc_library* plibrary);

OC_PUBLIC void
oc_free_library(oc_library library);

// OC_PUBLIC oc_error
// oc_discover_fonts(
//     oc_library library,
//     const oc_discovery_params* pparams);

OC_PUBLIC oc_error
oc_open_face(
    oc_library library,
    const char* path,
    const oc_open_params* pparams, // can be nil
    oc_face* pface);

/*
 * @note:
 *   You must not deallocate the memory before calling @oc_free_face.
 */
OC_PUBLIC oc_error
oc_open_memory_face(
    oc_library library,
    const void* data,
    size_t data_size,
    const oc_open_params* pparams, // can be nil
    oc_face* pface);

OC_PUBLIC void
oc_free_face(oc_face face);

// OC_PUBLIC oc_error
// oc_set_size(oc_face face, oc_26p6 desired_size, short dpi);

OC_PUBLIC uint16_t
oc_get_char_index(oc_face face, uint32_t charcode);

OC_PUBLIC void
oc_get_glyph_metrics(
    oc_face face,
    uint16_t glyph_index,
    oc_load_flags flags,
    oc_glyph_metrics* pmetrics);

OC_PUBLIC void
oc_get_glyph_bbox(
    oc_face face,
    uint16_t glyph_index,
    oc_load_flags flags,
    oc_bbox* pbbox);

// todo: add comments here explaining that every backend will generate diffrent glyph textures
//       so if u want it modified by every backend it would be recomended to raster it using glyph outlines
// todo: now we're rendering these glyphs from [0;0] position which is convenient, but it does lose some extra draw data
//       make so an user could specify how to draw this glyph mb allow to pass matricies and origins mb just some flags??
// todo: it is needed to make this method more complicated, now we cannot pass origin where to draw or matricies, nothing
OC_PUBLIC oc_error
oc_render_glyph(
    oc_face face,
    uint16_t glyph_index,
    oc_size* psize,
    unsigned char* buffer,
    size_t buffer_size);

OC_PUBLIC bool
oc_get_outline(
    oc_face face,
    uint16_t glyph_index,
    const oc_outline_funcs* outline_funcs,
    void* context);

// todo: copy variant would be nice which we would not need to free
OC_PUBLIC oc_error
oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable, void** pcontext);

OC_PUBLIC void
oc_free_table(oc_face face, void* context);

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

#ifdef ONECORE_NATIVE_IMPLEMENTATION
#endif

#if (defined(ONECORE_FREETYPE_IMPLEMENTATION) || defined(ONECORE_CORETEXT_IMPLEMENTATION) || defined(ONECORE_DIRECTWRITE_IMPLEMENTATION)) && !defined(ONECORE_IMPLEMENTATION)
#define ONECORE_IMPLEMENTATION
#endif

// ONECORE_FORCE_FREETYPE
// ONECORE_FORCE_CORETEXT
// ONECORE_FORCE_DIRECTWRITE
// ONECORE_FORCE_FONTCONFIG

#ifdef ONECORE_IMPLEMENTATION
#endif /* ONECORE_IMPLEMENTATION */

#ifdef ONECORE_FREETYPE_IMPLEMENTATION
#endif /* ONECORE_FREETYPE_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_IMPLEMENTATION
#endif /* ONECORE_CORETEXT_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_IMPLEMENTATION
#endif /* ONECORE_DIRECTWRITE_IMPLEMENTATION */
