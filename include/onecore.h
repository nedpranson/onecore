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

#define OC_LOAD_DEFAULT  0x0
#define OC_LOAD_NO_SCALE (1l << 0)
// #define OC_LOAD_VERTICAL (1l << 1)
// #define OC_LOAD_COLOR (1l << 2)
// #define OC_LOAD_NO_HINTING (1l << 3) // for now there is no hinting

typedef uint32_t oc_tag;
typedef uint32_t oc_load_flags;

typedef enum {
    oc_error_ok,
    oc_error_invalid_param,
    oc_error_table_missing,
    oc_error_out_of_memory,
    oc_error_failed_to_open,
    oc_error_insufficient_buffer,
    oc_error_unexpected,
} oc_error;

typedef struct {
    void* internals;
} oc_library;

typedef struct {
    uint16_t ppem;
    uint16_t upem;
    uint16_t ascent;
    uint16_t descent;
    int16_t leading;
    int16_t underline_position;
    uint16_t underline_thickness;
} oc_font_metrics;

// as this thingy grows we prob should pass it by `const oc_face*`
typedef struct {
    void* internals;
    oc_font_metrics metrics;
} oc_face;

// todo: make it not __handle but context and inside oc_free_face just pass it's ctx
typedef struct {
    const void* data;
    size_t size;

    void* __handle;
} oc_table;

// think! if we should return floats or those 26.6 ints?
typedef struct {
    uint32_t width;
    uint32_t height;
    int32_t bearing_x;
    int32_t bearing_y;
    uint32_t advance;
} oc_metrics;

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

// copy variant would be nice which we would not need to free
OC_EXPORT oc_error
oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable);

OC_EXPORT void
oc_free_table(oc_face face, oc_table table);

// on windows dpi is 92 on mac 72
// oc_set_size(oc_face face, float points, uint8_t dpi);
// after implementing scaling (for scalable fonts) then tidy up the code base

// todo: add scaled variant
// returning bools is hmm lazy
// todo: rename to just get_metrics and mb make it void??
OC_EXPORT bool
oc_get_metrics(
    oc_face face,
    uint16_t glyph_index,
    oc_load_flags flags,
    oc_metrics* pmetrics);

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
