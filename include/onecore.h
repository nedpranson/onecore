#ifndef ONECORE_LIBRARY_H_
#define ONECORE_LIBRARY_H_

#ifdef __cplusplus
extern "C" {
#endif

// todo: make this into into single header library

#if defined(_WIN32) || defined(__CYGWIN__)
#define OC_EXPORT __declspec(dllexport)
#else
#if __GNUC__ >= 4
#define OC_EXPORT __attribute__((visibility("default")))
#else
#define OC_EXPORT
#endif
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// todo: use 26p6, 16p16 percision!!!

//int32_t oc_mul_fix(int32_t a, int32_t b) {
//}

#define OC_LOAD_DEFAULT 0x0
#define OC_LOAD_NO_SCALE (1l << 0)
// #define OC_LOAD_FIXED (1l << 1)
// #define OC_LOAD_VERTICAL (1l << 2)
// #define OC_LOAD_COLOR (1l << 3)
// #define OC_LOAD_NO_HINTING (1l << 4) // for now there is no hinting

typedef uint32_t oc_tag;
typedef uint32_t oc_load_flags;
typedef int32_t oc_i16p16;
typedef int32_t oc_i26p6;

//#define OC_PIX_FLOOR( x )     ( (x) & ~(int32_t)63 )
// unsafe adition bla bla bla!!
//#define OC_PIX_ROUND( x )     OC_PIX_FLOOR( (x) + (int32_t)32 )
//#define OC_PIX_CEIL( x )      OC_PIX_FLOOR( (x) + 63 )

#define OC_ERROR_LIST \
    X(oc_error_ok, "no error") \
    X(oc_error_invalid_param, "invalid parameter") \
    X(oc_error_table_missing, "table is missing") \
    X(oc_error_out_of_memory, "out of memory") \
    X(oc_error_failed_to_open, "failed to open") \
    X(oc_error_insufficient_buffer, "insufficient buffer") \
    X(oc_error_unexpected, "unexpected error")

typedef enum {
#define X(e, s) e,
    OC_ERROR_LIST
#undef X
} oc_error;

// todo: add option for strings
static inline const char* oc_strerror(oc_error err) {
    switch (err) {
#define X(e, s) case e: return s;
        OC_ERROR_LIST
#undef X
        default: return "unknown error";
    }
}

typedef struct {
    void* internals;
} oc_library;

// todo: add height which is just ascept + descent + leading
typedef struct {
    uint16_t upem;
    uint16_t ppem;
    oc_i16p16 scale;
    uint16_t ascent;
    uint16_t descent;
    int16_t leading;
    int16_t underline_position;
    uint16_t underline_thickness;
} oc_font_metrics;

// as this thingy grows we prob should pass it by `const oc_face*`
// todo: pass this struct by ptr
typedef struct {
    void* internals; // we can add anonymous struct called oc_face_handle
    oc_font_metrics metrics;
} oc_face;

typedef struct {
    const void* data;
    size_t size;
} oc_table;

typedef struct {
    oc_i26p6 width;
    oc_i26p6 height;
    oc_i26p6 bearing_x;
    oc_i26p6 bearing_y;
    oc_i26p6 advance;
} oc_glyph_metrics;

typedef struct {
    int32_t x;
    int32_t y;
} oc_point;

typedef struct {
    uint32_t rows;
    uint32_t cols;
} oc_bbox;

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
    uint32_t face_index;
    float desired_size;
    short dpi;
} oc_face_params;

#define OC_MAKE_TAG(x1, x2, x3, x4) \
    (((uint8_t)x1) << 24 | ((uint8_t)x2) << 16 | ((uint8_t)x3) << 8 | ((uint8_t)x4))

OC_EXPORT oc_i16p16
oc_div_ip16p16(oc_i16p16 a, oc_i16p16 b);

OC_EXPORT oc_i16p16
oc_mul_ip16p16(oc_i16p16 a, oc_i16p16 b);

OC_EXPORT oc_error
oc_init_library(oc_library* plibrary);

OC_EXPORT void
oc_free_library(oc_library library);

OC_EXPORT oc_error
oc_open_face(
    oc_library library,
    const char* path,
    const oc_face_params* pparams, // can be nil
    oc_face* pface);

/*
 * @note:
 *   You must not deallocate the memory before calling @oc_free_face.
 */
OC_EXPORT oc_error
oc_open_memory_face(
    oc_library library,
    const void* data,
    size_t data_size,
    const oc_face_params* pparams, // can be nil
    oc_face* pface);

// OC_EXPORT oc_error
// oc_set_size(oc_face face, float desired_size, unsigned short dpi);

// OC_EXPORT float
// oc_get_size(oc_face face);

OC_EXPORT void
oc_free_face(oc_face face);

OC_EXPORT uint16_t
oc_get_char_index(oc_face face, uint32_t charcode);

// todo: copy variant would be nice which we would not need to free
OC_EXPORT oc_error
oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable, void** pcontext);

OC_EXPORT void
oc_free_table(oc_face face, void* context);

// on windows dpi is 92 on mac 72
// oc_set_size(oc_face face, float points, uint8_t dpi);
// after implementing scaling (for scalable fonts) then tidy up the code base

// todo: add scaled variant
// returning bools is hmm lazy
// todo: rename to just get_metrics and mb make it void??
// todo: make it a void on error memset struct to 0
OC_EXPORT void
oc_get_glyph_metrics(
    oc_face face,
    uint16_t glyph_index,
    oc_load_flags flags,
    oc_glyph_metrics* pmetrics);

// scaling is a hard problem to solve cuz of hinting
// so we will just not implement it yet first we need to add rendering
// then look how fonts looks if it's bad then we can look how freetype handled hinting
// and implement scaled function with some flags
// though we can add simple sclae function, but it would require us to change all
// glyph_metric data from ints to floats, we could use 26.6perhaps;
// OC_EXPORT bool
// oc_get_glyph_metrics_scaled(oc_face face, uint16_t glyph_index, oc_glyph_metrics* pglyph_metrics);

OC_EXPORT bool
oc_get_outline(
    oc_face face,
    uint16_t glyph_index,
    const oc_outline_funcs*
        outline_funcs,
    void* context);

// todo: add comments here explaining that every backend will generate diffrent glyph textures
//       so if u want it modified by every backend it would be recomended to raster it using glyph outlines
// todo: now we're rendering these glyphs from [0;0] position which is convenient, but it does lose some extra draw data
//       make so an user could specify how to draw this glyph mb allow to pass matricies and origins mb just some flags??
// todo: it is needed to make this method more complicated, now we cannot pass origin where to draw or matricies, nothing
OC_EXPORT oc_error
oc_render_glyph(
    oc_face face,
    uint16_t glyph_index,
    oc_bbox* pbbox,
    unsigned char* buffer,
    size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // ONECORE_LIBRARY_H_
