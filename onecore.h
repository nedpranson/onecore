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
    oc_26p6 desired_size;
    uint16_t dpi;
} oc_open_params;

typedef struct {
    uint16_t ppem;
    oc_16p16 scale;
} oc_size;

typedef struct {
    oc_26p6 width;
    oc_26p6 height;
    oc_26p6 bearing_x;
    oc_26p6 bearing_y;
    oc_26p6 advance;
} oc_glyph_metrics;

typedef struct {
    uint32_t rows;
    uint32_t cols;
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

// todo (stage 2): integrate even more fields
// todo (stage 2): need a way to know homy mady glyphs a font has
typedef struct {
    // todo: check if family names change on diff locales
    const char* family;
    oc_slant slant;
    uint16_t weight;
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

    oc_size size;
    uint16_t upem;
    uint16_t ascent;
    uint16_t descent;
    int16_t leading;
    int16_t underline_position;
    uint16_t underline_thickness;
} oc_face;

typedef struct {
    oc_collection_impl* impl;

    oc_font** fonts;
    uint32_t nfonts;
} oc_collection;

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

// we could even split this project into 2 diffrent header files
// just not sure how would opening fonts from oc_font obj would work
// ocf_ -> onecore finder
// ocl_ -> onecore loader

OC_PUBLIC bool
ocf_has_character(const oc_font* font, uint32_t character);

OC_PUBLIC size_t 
ocf_copy_path(const oc_font* font, char* buf, size_t len);

OC_PUBLIC oc_error
ocf_open_font(
    const oc_font* font,
    oc_26p6 desired_size,
    uint16_t dpi,
    oc_face* oface);

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
oc_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi);

OC_PUBLIC uint16_t
oc_get_char_index(const oc_face* face, uint32_t charcode);

OC_PUBLIC void
oc_get_glyph_metrics(
    const oc_face* face,
    uint16_t index,
    oc_load_flags flags,
    oc_glyph_metrics* ometrics);

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
//
// roadmap:
// dwrite and coretext knows how to draw bezier curves hence theoretically hinting can be achieved with manual shapes rasterization,
// essentially onecore would become freetype, but with native font file parsing and rendering engine
OC_PUBLIC oc_error
oc_render_glyph(
    const oc_face* face,
    uint16_t index,
    oc_extent* oextent,
    uint8_t* buffer,
    size_t buffer_size);

// todo: renew this impl
OC_PUBLIC bool
oc_get_outline(
    const oc_face* face,
    uint16_t index,
    const oc_outline_funcs* funcs,
    void* user);

OC_PUBLIC oc_error
oc_get_sfnt_table(
    const oc_face* face,
    oc_tag tag,
    uint32_t offset,
    void* data,
    uint32_t* size);

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
#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
typedef SRWLOCK oc__mutex_impl_t;

#define oc__mutex_impl_init(m) InitializeSRWLock(m)
#define oc__mutex_impl_lock(m) AcquireSRWLockExclusive(m)
#define oc__mutex_impl_unlock(m) ReleaseSRWLockExclusive(m)
#define oc__mutex_impl_destroy(m) ((void)0)
#else
#include <pthread.h>
typedef pthread_mutex_t oc__mutex_impl_t;

#define oc__mutex_impl_init(m) pthread_mutex_init(m, NULL)
#define oc__mutex_impl_lock(m) pthread_mutex_lock(m)
#define oc__mutex_impl_unlock(m) pthread_mutex_unlock(m)
#define oc__mutex_impl_destroy(m) pthread_mutex_destroy(m)
#endif /* defined(_MSC_VER) || defined(__MINGW32__) */

#define oc__exit_critical(e)         \
    do {                             \
        oc__mutex_impl_unlock(lock); \
        err = (e);                   \
        goto exit;                   \
    } while (0)

struct oc_face_impl {
    FT_Face ft_face;
    oc__mutex_impl_t lock;
};

oc_error oc_init_library(oc_library* plibrary) {
    FT_Library library;
    FT_Error err;

    if (plibrary == NULL) {
        return oc_error_invalid_param;
    }

    err = FT_Init_FreeType(&library);
    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    plibrary->internals = library;
    return oc_error_ok;
}

void oc_free_library(oc_library* library) {
    FT_Library ft_library;
    if (!library) {
        return;
    }

    ft_library = library->internals;
    FT_Done_FreeType(ft_library);
    memset(library, 0, sizeof(*library));
}

static oc_error oc__init_face(FT_Face ft_face, const oc_open_params* params, oc_face* oface) {
    FT_Error err;
    oc_face face;

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

oc_error oc_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    int32_t err;
    FT_Face ft_face;
    FT_Library ft_library;
    oc_open_params params;
    FT_Open_Args ft_open_args = { 0 };

    if (!(library && path && oface)) {
        return oc_error_invalid_param;
    }

    ft_library = library->internals;

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

oc_error oc_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    int32_t err;
    FT_Face ft_face;
    FT_Library ft_library;
    oc_open_params params;

    if (!(library && oface)) {
        return oc_error_invalid_param;
    }

    ft_library = library->internals;
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

void oc_free_face(oc_face* face) {
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

oc_error oc_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    FT_Error err;
    FT_Face ft_face;

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

uint16_t oc_get_char_index(const oc_face* face, uint32_t charcode) {
    return face ? FT_Get_Char_Index(face->impl->ft_face, charcode) : 0;
}

oc_error oc_get_sfnt_table(const oc_face* face, oc_tag tag, uint32_t offset, void* data, uint32_t* size) {
    FT_Error err;
    FT_Face ft_face;

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

// todo: add option for verticals and maybe load both hori and vert bearings, advances
void oc_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    FT_Error err;
    FT_Face ft_face;
    oc__mutex_impl_t* lock;
    FT_Glyph_Metrics ft_metrics;
    oc_glyph_metrics metrics = { 0 };
    FT_Int32 ft_load_flags = FT_LOAD_NO_AUTOHINT | FT_LOAD_BITMAP_METRICS_ONLY | FT_LOAD_NO_HINTING;

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

typedef struct {
    const oc_outline_funcs* funcs;
    void* ctx;

    FT_Vector x2origin;
    bool figure_started;
} oc__outline_context;

static int oc__move_to(const FT_Vector* to, void* user) {
    oc__outline_context* ctx = (oc__outline_context*)user;
    oc_point point = { (int32_t)(to->x >> 1), (int32_t)(to->y >> 1) };

    if (ctx->figure_started) {
        ctx->funcs->end_figure(ctx->ctx);
    }

    ctx->funcs->start_figure(point, ctx->ctx);
    ctx->x2origin = *to;
    ctx->figure_started = true;

    return 0;
}

static int oc__line_to(const FT_Vector* x2to, void* user) {
    oc__outline_context* ctx = (oc__outline_context*)user;
    oc_point point = { (int32_t)(x2to->x >> 1), (int32_t)(x2to->y >> 1) };

    ctx->funcs->line_to(point, ctx->ctx);
    ctx->x2origin = *x2to;

    return 0;
}

typedef struct {
    float x;
    float y;
} oc__point_2f;

static int oc__conic_to(const FT_Vector* x2control, const FT_Vector* x2to, void* user) {
    oc__outline_context* ctx = (oc__outline_context*)user;

    oc__point_2f forigin = { (float)ctx->x2origin.x * 0.5f, (float)ctx->x2origin.y * 0.5f };
    oc__point_2f fto = { (float)x2to->x * 0.5f, (float)x2to->y * 0.5f };

    // comes extremely closes to dwrites internal implemintation
    // but is not 100% perfect
    oc__point_2f cubic[2];
    cubic[0].x = forigin.x + (float)(x2control->x - ctx->x2origin.x) / 3.0f;
    cubic[0].y = forigin.y + (float)(x2control->y - ctx->x2origin.y) / 3.0f;
    cubic[1].x = fto.x + (float)(x2control->x - x2to->x) / 3.0f;
    cubic[1].y = fto.y + (float)(x2control->y - x2to->y) / 3.0f;

    oc_point points[3] = {
        { (int32_t)cubic[0].x, (int32_t)cubic[0].y },
        { (int32_t)cubic[1].x, (int32_t)cubic[1].y },
        { (int32_t)(x2to->x >> 1), (int32_t)(x2to->y >> 1) }
    };

    ctx->funcs->cubic_to(points[0], points[1], points[2], ctx->ctx);
    ctx->x2origin = *x2to;

    return 0;
}

static int oc__cubic_to(const FT_Vector* x2c1, const FT_Vector* x2c2, const FT_Vector* x2to, void* user) {
    oc__outline_context* ctx = (oc__outline_context*)user;

    oc_point points[3] = {
        { (int32_t)(x2c1->x >> 1), (int32_t)(x2c1->y >> 1) },
        { (int32_t)(x2c2->x >> 1), (int32_t)(x2c2->y >> 1) },
        { (int32_t)(x2to->x >> 1), (int32_t)(x2to->y >> 1) }
    };

    ctx->funcs->cubic_to(points[0], points[1], points[2], ctx->ctx);
    ctx->x2origin = *x2to;

    return 0;
}

void oc_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    FT_Error err;
    FT_Face ft_face;
    oc__mutex_impl_t* lock;
    FT_BBox ft_cbox;
    oc_bbox cbox = { 0 };
    FT_Int32 ft_load_flags = FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING;

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

bool oc_get_outline(const oc_face* face, uint16_t index, const oc_outline_funcs* funcs, void* user) {
    FT_Error err;
    FT_Face ft_face;
    oc__mutex_impl_t* lock;
    FT_GlyphSlot glyph;
    FT_Outline outline;
    oc__outline_context context = { 0 };

    if (!(face && funcs)) {
        goto exit;
    }

    ft_face = face->impl->ft_face;
    lock = &face->impl->lock;

    oc__mutex_impl_lock(lock);
    err = FT_Load_Glyph(ft_face, index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);
    if (err != FT_Err_Ok) {
        goto exit_critical;
    }

    glyph = ft_face->glyph;
    outline = glyph->outline;

    if (glyph->format != FT_GLYPH_FORMAT_OUTLINE && glyph->format != FT_GLYPH_FORMAT_COMPOSITE) {
        goto exit_critical;
    }
    oc__mutex_impl_unlock(lock);

    context.funcs = funcs;
    context.ctx = user;

    // shift is set to one as we want all point to be multiplied by 2
    // to restore conic 'to' position to its original floating point value
    static const FT_Outline_Funcs ft_funcs = {
        oc__move_to,
        oc__line_to,
        oc__conic_to,
        oc__cubic_to,
        1,
        0,
    };

    err = FT_Outline_Decompose(&outline, &ft_funcs, &context);
    if (err != FT_Err_Ok) {
        return false;
    }

    if (context.figure_started) {
        context.funcs->end_figure(context.ctx);
    }

    return true;
exit_critical:
    oc__mutex_impl_unlock(lock);
exit:
    return false;
}

oc_error oc_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t buffer_size) {
    FT_Face ft_face;
    oc__mutex_impl_t* lock;
    FT_Error ft_err;
    FT_Bitmap ft_bitmap;
    FT_Glyph ft_glyph = NULL;
    oc_error err = oc_error_ok;
    oc_extent extent = { 0 };

    size_t length;

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
        // todo: implement diffrent types
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

    length = (size_t)extent.rows * (size_t)extent.cols;
    if (buffer_size < length) {
        oc__exit_critical(oc_error_insufficient_buffer);
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

    memcpy(buffer, ((FT_BitmapGlyph)ft_glyph)->bitmap.buffer, length);

exit:
    if (ft_glyph)
        FT_Done_Glyph(ft_glyph);
    if (oextent)
        *oextent = extent;

    return err;
}
#endif /* ONECORE_FREETYPE_LOADER_IMPLEMENTATION */

#ifdef ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION
#include <assert.h>
#include <fontconfig/fontconfig.h>
#include <stdlib.h>
#include <string.h>

struct oc_collection_impl {
    const oc_library* oc_library;
    FcConfig* fc_config;
};

typedef struct {
    const oc_library* oc_library;
    FcPattern* fc_pattern;
    oc_font font;
} oc__font_impl;

typedef enum {
    oc__status_ok,
    oc__status_memory,
    oc__status_skip,
} oc__status;

static inline void oc__free_font(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    free(impl);
}

oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error err = oc_error_ok;
    FcConfig* fc_config;
    oc_collection_impl* impl;
    oc_collection collection = { 0 };

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

void oc_free_collection(oc_collection* collection) {
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

oc_error oc_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;
    const oc_library* oc_library;

    FcConfig* fc_config;
    FcFontSet* fc_fonts;

    int font_count;

    oc_font** fonts = NULL;
    uint32_t nfonts = 0;

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
        oc_font* font;

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
    FcCharSet* charset;
    FcResult result;

    if (!font) {
        return false;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    result = FcPatternGetCharSet(impl->fc_pattern, FC_CHARSET, 0, &charset);

    return result == FcResultMatch && FcCharSetHasChar(charset, character);
}

// todo (stage 2): make this stuff crossplatform
// upper 16 means instance
// lower 16 means index
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;

    FcPattern* pattern;
    FcResult result;

    FcChar8* file;
    int index;

    oc_open_params params;

    if (!font) {
        return oc_error_invalid_param;
    }

    impl  = oc__parentof(oc__font_impl, font, font);
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

    return oc_open_face(impl->oc_library, (char*)file, &params, oface);
}

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    FcResult result;

    FcChar8* file;
    size_t file_len;

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

oc_error oc_init_library(oc_library* olibrary) {
    return olibrary == NULL ? oc_error_invalid_param : oc_error_ok;
}

void oc_free_library(oc_library* library) {
    if (library)
        memset(library, 0, sizeof(*library));
}

static oc_error oc__init_face(CTFontDescriptorRef  descriptor, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    CTFontRef ct_font;
    oc_face face;

    oc_16p16 scaled;
    CGFloat size;

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

    CFIndex count = CFArrayGetCount(descriptors);
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

oc_error oc_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    CFStringRef ct_path;
    CFURLRef url_path;

    CFArrayRef descriptors;
    oc_error err;

    if (!(library && path && oface)) {
        return oc_error_invalid_param;
    }

    // DOING!!!
    // todo: validate utf8 so ct_path would only fail on oom

    ct_path = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    if (ct_path == NULL) {
        return oc_error_failed_to_open; // or oom
    }

    url_path = CFURLCreateWithFileSystemPath(NULL, ct_path, kCFURLPOSIXPathStyle, false);
    CFRelease(ct_path);

    if (url_path == NULL) {
        return oc_error_failed_to_open;
    }

    descriptors = CTFontManagerCreateFontDescriptorsFromURL(url_path);
    CFRelease(url_path);

    // todo: think how to reliably handle this err
    if (descriptors == NULL) {
        return oc_error_failed_to_open; // or oom
    }

    err = oc__open_face_from_descriptors(descriptors, uparams, oface);
    CFRelease(descriptors);

    return err;
}

oc_error oc_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    CFDataRef ct_data;
    CFArrayRef descriptors;
    oc_error err;

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
        return oc_error_failed_to_open;
    }

    err = oc__open_face_from_descriptors(descriptors, uparams, oface);
    CFRelease(descriptors);

    return err;
}

void oc_free_face(oc_face* face) {
    if (face) {
        CFRelease(face->impl);
        memset(face, 0, sizeof(*face));
    }
}

uint16_t oc_get_char_index(const oc_face* face, uint32_t charcode) {
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

oc_error oc_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t ppem;

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

oc_error oc_get_sfnt_table(const oc_face* face, oc_tag tag, uint32_t offset, void* data, uint32_t* size) {
    CTFontRef ct_font;
    CFDataRef ct_table;

    const UInt8* buffer;
    CFIndex length;

    if (!(face && size)) {
        return oc_error_invalid_param;
    }

    ct_font = (CTFontRef)face->impl;
    ct_table = CTFontCopyTable(ct_font, tag, kCTFontTableOptionNoOptions);
    length = (CFIndex)*size;

    assert(length == 0 || length >= offset);

    if (ct_table == NULL) {
        return oc_error_table_missing;
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

void oc_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    CTFontRef ct_font;
    CFIndex count;

    CGSize advance;
    CGRect rect;

    uint16_t upem;
    CGFloat size;
    oc_26p6 scale;

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

void oc_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    CTFontRef ct_font;
    CGRect rect;

    uint16_t upem;
    CGFloat size;
    oc_26p6 scale;

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
    float x;
    float y;
} oc__point_2f;

typedef struct {
    const oc_outline_funcs* funcs;
    void* ctx;
    CGPoint start;
    CGPoint origin;
    CGFloat fsize;
    CGFloat funits_per_em;
} oc__outline_context;

static void oc__path_applier(void* info, const CGPathElement* element) {
    oc__outline_context* ctx = (oc__outline_context*)info;
    CGFloat fppem = ctx->fsize;
    CGFloat fupem = ctx->funits_per_em;

    switch (element->type) {
    case kCGPathElementMoveToPoint: {
        oc_point point = {
            element->points[0].x * fupem / fppem,
            element->points[0].y * fupem / fppem
        };

        ctx->funcs->start_figure(point, ctx->ctx);
        ctx->start = element->points[0];
        ctx->origin = element->points[0];
    }; break;
    case kCGPathElementAddLineToPoint: {
        oc_point point = {
            element->points[0].x * fupem / fppem,
            element->points[0].y * fupem / fppem
        };

        ctx->funcs->line_to(point, ctx->ctx);
        ctx->origin = element->points[0];
    } break;
    case kCGPathElementAddQuadCurveToPoint: {
        oc__point_2f forigin = { ctx->origin.x * fupem / fppem, ctx->origin.y * fupem / fppem };
        oc__point_2f fcontrol = { element->points[0].x * fupem / fppem, element->points[0].y * fupem / fppem };
        oc__point_2f fto = { element->points[1].x * fupem / fppem, element->points[1].y * fupem / fppem };

        oc__point_2f cubic[2];
        cubic[0].x = forigin.x + 2.0f * (fcontrol.x - forigin.x) / 3.0f;
        cubic[0].y = forigin.y + 2.0f * (fcontrol.y - forigin.y) / 3.0f;
        cubic[1].x = fto.x + 2.0f * (fcontrol.x - fto.x) / 3.0f;
        cubic[1].y = fto.y + 2.0f * (fcontrol.y - fto.y) / 3.0f;

        oc_point points[3] = {
            { cubic[0].x, cubic[0].y },
            { cubic[1].x, cubic[1].y },
            { fto.x, fto.y }
        };

        ctx->funcs->cubic_to(points[0], points[1], points[2], ctx->ctx);
        ctx->origin = element->points[1];
    }; break;
    case kCGPathElementAddCurveToPoint: {
        oc_point points[3] = {
            { element->points[0].x * fupem / fppem, element->points[0].y * fupem / fppem },
            { element->points[1].x * fupem / fppem, element->points[1].y * fupem / fppem },
            { element->points[2].x * fupem / fppem, element->points[2].y * fupem / fppem },
        };

        ctx->funcs->cubic_to(points[0], points[1], points[2], ctx->ctx);
        ctx->origin = element->points[2];
    } break;
    case kCGPathElementCloseSubpath:
        if (ctx->origin.x != ctx->start.x || ctx->origin.y != ctx->start.y) {
            oc_point point = { ctx->start.x * fupem / fppem, ctx->start.y * fupem / fppem };
            ctx->funcs->line_to(point, ctx->ctx);
        }

        ctx->funcs->end_figure(ctx->ctx);
        break;
    }
}

bool oc_get_outline(const oc_face* face, uint16_t index, const oc_outline_funcs* funcs, void* user) {
    CTFontRef ct_font;
    CGPathRef outline;
    oc__outline_context context = { 0 };

    if (!(face && funcs)) {
        return false;
    }

    ct_font = (CTFontRef)face->impl;
    outline = CTFontCreatePathForGlyph(ct_font, index, NULL);

    if (outline == NULL) {
        return false;
    }

    context.funcs = funcs;
    context.ctx = user;
    context.fsize = CTFontGetSize(ct_font);
    context.funits_per_em = CTFontGetUnitsPerEm(ct_font);

    CGPathApply(outline, &context, oc__path_applier);
    CGPathRelease(outline);

    return true;
}

oc_error oc_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t buffer_size) {
    oc_error err = oc_error_ok;

    CTFontRef ct_font;
    CFIndex count;

    oc_bbox cbox;
    oc_bbox pbox;

    CGColorSpaceRef linear_gray;
    CGContextRef context;
    CGRect rect;
    CGPoint pos;

    oc_extent extent = { 0 };
    size_t length;

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
    oc_get_glyph_cbox(face, index, OC_LOAD_DEFAULT, &cbox);

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

    length = (size_t)extent.rows * (size_t)extent.cols;
    if (buffer_size < length) {
        err = oc_error_insufficient_buffer;
        goto exit;
    }

    linear_gray = CGColorSpaceCreateWithName(kCGColorSpaceLinearGray);
    if (linear_gray == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    memset(buffer, 0, length);

    context = CGBitmapContextCreate(
        buffer,
        extent.cols,
        extent.rows,
        8,
        extent.cols,
        linear_gray,
        kCGImageAlphaOnly);
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
    CTFontDescriptorRef ct_font;
    CTFontRef ct_face;
    CFStringRef ct_family;
    oc_font font;
} oc__font_impl;

static inline void oc__free_font_impl(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    CFRelease(impl->ct_family);
    CFRelease(impl->ct_face);
    free(impl);
}

oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error err = oc_error_ok;
    oc_collection collection = { 0 };

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }

exit:
    if (ocollection)
        *ocollection = collection;
    return err;
}

void oc_free_collection(oc_collection* collection) {
    if (collection) {
        while (collection->nfonts--) {
            oc__free_font_impl(collection->fonts[collection->nfonts]);
        }

        free(collection->fonts);

        if (collection->impl)
            CFRelease(collection->impl);
        memset(collection, 0, sizeof(*collection));
    }
}

static const struct {
    int ot;
    double ct;
} oc__weight_map[] = {
    { 0, -0.8 },
    { 100, -0.8 },
    { 200, -0.6 },
    { 300, -0.4 },
    // todo: check what these are equal to
    // { 350, FC_WEIGHT_DEMILIGHT },
    // { 380, FC_WEIGHT_BOOK },
    { 400, 0.0 },
    { 500, 0.23 },
    { 600, 0.3 },
    { 700, 0.4 },
    { 800, 0.56 },
    { 900, 0.62 },
    { 1000, 1.0 }, // idk if this correct
};

// todo: fix these lerp functioms wtf is this
static double oc__lerp(double x, double x1, double x2, int y1, int y2) {
    double dx = x2 - x1;
    int dy = y2 - y1;
    assert(dx > 0 && dy >= 0 && x1 <= x && x <= x2);
    return y1 + (dy * (x - x1) + dx / 2.0) / dx;
}

double oc__convert(double ct_weight) {
    int i;

    for (i = 1; ct_weight > oc__weight_map[i].ct; i++)
        ;

    if (ct_weight == oc__weight_map[i].ct) {
        return oc__weight_map[i].ot;
    }

    return oc__lerp(
        ct_weight,
        oc__weight_map[i - 1].ct,
        oc__weight_map[i].ct,
        oc__weight_map[i - 1].ot,
        oc__weight_map[i].ot);
}

static oc__font_impl* oc__init_font_impl(CTFontDescriptorRef ct_font) {
    oc__font_impl* impl = NULL;

    CFDictionaryRef ct_traits = NULL;
    CFStringRef ct_family;
    CTFontRef ct_face;

    const char* family;

    CFNumberRef weight_obj;
    CFNumberRef symbolic_obj;

    double ct_weight;
    uint32_t ct_symbolic;

    assert(ct_font != NULL);

    ct_traits = CTFontDescriptorCopyAttribute(ct_font, kCTFontTraitsAttribute);
    if (ct_traits == NULL) {
        goto exit;
    }

    weight_obj = CFDictionaryGetValue(ct_traits, kCTFontWeightTrait);
    assert(weight_obj != NULL);

    symbolic_obj = CFDictionaryGetValue(ct_traits, kCTFontSymbolicTrait);
    assert(symbolic_obj != NULL);

    CFNumberGetValue(weight_obj, kCFNumberDoubleType, &ct_weight);
    CFNumberGetValue(symbolic_obj, kCFNumberSInt32Type, &ct_symbolic);

    ct_family = CTFontDescriptorCopyAttribute(ct_font, kCTFontFamilyNameAttribute);
    assert(ct_family != NULL);

    family = CFStringGetCStringPtr(ct_family, kCFStringEncodingUTF8);
    // todo: test is it always utf8 and null terminated
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

    impl->ct_font = ct_font;
    impl->ct_family = ct_family;
    impl->ct_face = ct_face;
    impl->font.family = family;
    impl->font.weight = oc__convert(ct_weight) + 0.5;
    impl->font.slant = oc_slant_roman;

    // tood: implement valid one
    //       we need crossplatform solution
    if (ct_symbolic & kCTFontItalicTrait) {
        impl->font.slant = oc_slant_italic;
    }

exit:
    if (ct_traits)
        CFRelease(ct_traits);
    return impl;
}

oc_error oc_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;

    CTFontCollectionRef ct_collection;
    CFArrayRef ct_fonts;

    CFIndex font_count;

    oc_font** fonts = NULL;
    uint32_t nfonts = 0;

    oc_collection tmp_collection;

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

    for (CFIndex i = 0; i < font_count; i++) {
        CTFontDescriptorRef ct_font = CFArrayGetValueAtIndex(ct_fonts, i);
        oc__font_impl* impl = oc__init_font_impl(ct_font);
        if (impl == NULL) {
            err = oc_error_out_of_memory;
            goto exit;
        }

        fonts[nfonts++] = &impl->font;
    }
done:
    tmp_collection.impl = (oc_collection_impl*)ct_fonts;
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

    ct_fonts = (CFArrayRef)collection->impl;
    fonts = collection->fonts;
    nfonts = collection->nfonts;

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
    CTFontRef ct_face;

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


// todo: ensure that every backends set oface to 0 on failure
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;

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

    return oc__init_face(impl->ct_font, desired_size, dpi, oface);
}

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    CFURLRef url;

    CFStringRef path;
    CFIndex path_len;

    size_t copy_len;

    if (!font) {
        return 0;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    url = CTFontDescriptorCopyAttribute(impl->ct_font, kCTFontURLAttribute);

    // todo: check if this is a shared object
    if (url == NULL) {
        return 0;
    }

    path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
    CFRelease(url);

    if (path == NULL) {
        return 0;
    }

    // todo: check if path is already utf8
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
#include <assert.h>
#include <initguid.h>

#include <d2d1.h>
#include <dwrite.h>

struct oc_face_impl {
    IDWriteFontFace* dw_face;
    IDWriteFactory* dw_factory;
};

typedef struct {
    const void* data;
    size_t size;
} oc__memory_view;

typedef struct {
    const IDWriteFontFileStreamVtbl* lpVtbl;
    LONG ref_count;
    oc__memory_view memory_view;
} OC__IDWriteFontFileStream;

typedef struct {
    const IDWriteFontFileLoaderVtbl* lpVtbl;
    LONG ref_count;
} OC__IDWriteFontFileLoader;

typedef struct {
    const ID2D1SimplifiedGeometrySinkVtbl* lpVtbl;
    const oc_outline_funcs* funcs;
    D2D1_POINT_2F start;
    D2D1_POINT_2F origin;
    void* ctx;
    LONG ref_count;
} OC__ID2D1SimplifiedGeometrySink;

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
    const void** fragment_start,
    UINT64 offset,
    UINT64 fragment_size,
    void** fragment_context) {

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

    *fragment_start = this->memory_view.data + offset;
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

static HRESULT STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_Close(ID2D1SimplifiedGeometrySink* This) {
    (void)This;
    return S_OK;
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_EndFigure(ID2D1SimplifiedGeometrySink* This, D2D1_FIGURE_END figureEnd) {
    (void)figureEnd;
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    if (this->origin.x != this->start.x || this->origin.y != this->start.y) {
        oc_point point = { this->start.x, -this->start.y };
        this->funcs->line_to(point, this->ctx);
    }

    this->funcs->end_figure(this->ctx);
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_AddBeziers(ID2D1SimplifiedGeometrySink* This, const D2D1_BEZIER_SEGMENT* beziers, UINT beziersCount) {
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    oc_point points[3];
    for (UINT32 i = 0; i < beziersCount; i++) {
        points[0].x = beziers[i].point1.x;
        points[0].y = -beziers[i].point1.y;

        points[1].x = beziers[i].point2.x;
        points[1].y = -beziers[i].point2.y;

        points[2].x = beziers[i].point3.x;
        points[2].y = -beziers[i].point3.y;

        this->funcs->cubic_to(points[0], points[1], points[2], this->ctx);
    }

    assert(beziersCount > 0);
    this->origin = beziers[beziersCount - 1].point3;
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_AddLines(ID2D1SimplifiedGeometrySink* This, const D2D1_POINT_2F* points, UINT pointsCount) {
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    oc_point point;
    for (UINT32 i = 0; i < pointsCount; i++) {
        point.x = points[i].x;
        point.y = -points[i].y;
        this->funcs->line_to(point, this->ctx);
    }

    assert(pointsCount > 0);
    this->origin = points[pointsCount - 1];
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_BeginFigure(ID2D1SimplifiedGeometrySink* This, D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) {
    (void)figureBegin;
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    oc_point point = { startPoint.x, -startPoint.y };
    this->funcs->start_figure(point, this->ctx);
    this->start = startPoint;
    this->origin = startPoint;
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_SetSegmentFlags(ID2D1SimplifiedGeometrySink* This, D2D1_PATH_SEGMENT vertexFlags) {
    (void)This;
    (void)vertexFlags;
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_SetFillMode(ID2D1SimplifiedGeometrySink* This, D2D1_FILL_MODE fillMode) {
    (void)This;
    (void)fillMode;
};

static ULONG STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_Release(IUnknown* This) {
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    assert(refs != -1);
    return refs;
}

static ULONG STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_AddRef(IUnknown* This) {
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader)) {
        OC__ID2D1SimplifiedGeometrySink_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const ID2D1SimplifiedGeometrySinkVtbl OC__ID2D1SimplifiedGeometrySinkVtbl = {
    { OC__ID2D1SimplifiedGeometrySink_QueryInterface,
        OC__ID2D1SimplifiedGeometrySink_AddRef,
        OC__ID2D1SimplifiedGeometrySink_Release },
    OC__ID2D1SimplifiedGeometrySink_SetFillMode,
    OC__ID2D1SimplifiedGeometrySink_SetSegmentFlags,
    OC__ID2D1SimplifiedGeometrySink_BeginFigure,
    OC__ID2D1SimplifiedGeometrySink_AddLines,
    OC__ID2D1SimplifiedGeometrySink_AddBeziers,
    OC__ID2D1SimplifiedGeometrySink_EndFigure,
    OC__ID2D1SimplifiedGeometrySink_Close,
};

OC__IDWriteFontFileLoader oc__file_loader = { &OC__IDWriteFontFileLoaderVtbl, 0 };
IDWriteFontFileLoader* oc__dw_file_loader = (IDWriteFontFileLoader*)&oc__file_loader;

oc_error oc_init_library(oc_library* olibrary) {
    HRESULT err;
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

    olibrary->internals = dw_factory;
    return oc_error_ok;
}

void oc_free_library(oc_library* library) {
    IDWriteFactory* dw_factory;

    if (!library) {
        return;
    }

    dw_factory = library->internals;

    dw_factory->lpVtbl->UnregisterFontFileLoader(dw_factory, oc__dw_file_loader);
    dw_factory->lpVtbl->Release(dw_factory);

    memset(library, 0, sizeof(oc_library));
}

static oc_error oc__init_face(IDWriteFactory* dw_factory, IDWriteFontFace* dw_face, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    DWRITE_FONT_METRICS metrics;
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t ppem;
    oc_face face;

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
    HRESULT err;
    WINBOOL is_supported_fonttype;
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFace* dw_face;
    UINT32 face_num;
    oc_open_params params = oc__open_params_defaults(uparams);

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

oc_error oc_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    int32_t err;
    int size;
    wchar_t* dw_path;
    IDWriteFactory* dw_factory;
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

    dw_factory = library->internals;
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

oc_error oc_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    int32_t err;
    IDWriteFontFile* font_file;
    IDWriteFactory* dw_factory;
    oc__memory_view key;

    if (!(library && data && oface)) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    dw_factory = library->internals;

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

void oc_free_face(oc_face* face) {
    face->impl->dw_face->lpVtbl->Release(face->impl->dw_face);
    free(face->impl);
    memset(face, 0, sizeof(*face));
}

uint16_t oc_get_char_index(const oc_face* face, uint32_t charcode) {
    HRESULT err;
    IDWriteFontFace* dw_face;
    UINT16 index;

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

oc_error oc_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t ppem;

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

oc_error oc_get_sfnt_table(const oc_face* face, oc_tag tag, uint32_t offset, void* data, uint32_t* size) {
    HRESULT err;
    IDWriteFontFace* dw_face;

    const void* table_data;
    UINT32 table_size;

    void* context;
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
        memcpy(data, table_data + offset, length);
    }

    dw_face->lpVtbl->ReleaseFontTable(dw_face, context);
    return oc_error_ok;
}

void oc_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    HRESULT err;
    DWRITE_GLYPH_METRICS dw_metrics;
    IDWriteFontFace* dw_face;
    oc_16p16 scale;
    UINT16 count;
    oc_glyph_metrics metrics = { 0 };

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

void oc_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    HRESULT err;
    DWRITE_GLYPH_METRICS metrics;
    IDWriteFontFace* dw_face;
    UINT16 count;
    oc_16p16 scale;
    oc_bbox cbox = { 0 };

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

bool oc_get_outline(const oc_face* face, uint16_t index, const oc_outline_funcs* funcs, void* user) {
    HRESULT err;
    ULONG refs;
    OC__ID2D1SimplifiedGeometrySink geometry_sink = { 0 };

    if (!(face && funcs)) {
        return false;
    }

    geometry_sink.lpVtbl = &OC__ID2D1SimplifiedGeometrySinkVtbl;
    geometry_sink.funcs = funcs;
    geometry_sink.ref_count = 1;
    geometry_sink.ctx = user;

    err = face->impl->dw_face->lpVtbl->GetGlyphRunOutline(
        face->impl->dw_face,
        face->upem,
        &index,
        NULL,
        NULL,
        1,
        FALSE,
        FALSE,
        (IDWriteGeometrySink*)&geometry_sink);

    if (err != S_OK) {
        return false;
    }

    refs = geometry_sink.lpVtbl->Base.Release((IUnknown*)&geometry_sink);

    (void)refs;
    assert(refs == 0);

    return true;
}

// todo: check this rendering thingy as smth is a bit off with dwrite
//       it seems dwrite does hard edges, idk if we can change that
oc_error oc_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t buffer_size) {
    oc_error err = oc_error_ok;
    HRESULT dw_err = S_OK;

    IDWriteFontFace* dw_face;
    IDWriteFactory* dw_factory;

    oc_bbox cbox;
    oc_bbox pbox;

    DWRITE_MATRIX transform;
    UINT16 count;
    RECT bounds;

    IDWriteGlyphRunAnalysis* analysis = NULL;
    DWRITE_GLYPH_RUN glyph_run = { 0 };
    oc_extent extent = { 0 };

    uint8_t* bitmap = NULL;
    size_t length;

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
    oc_get_glyph_cbox(face, index, OC_LOAD_DEFAULT, &cbox);

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

    length = (size_t)extent.rows * (size_t)extent.cols;
    if (buffer_size < length) {
        oc__exit(oc_error_insufficient_buffer);
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

    bitmap = malloc(length * 3);
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
        length * 3);

    if (err != S_OK) {
        oc__exit(oc__unexpected(err));
    }

    for (uint32_t i = 0; i < length; i++) {
        uint8_t r = bitmap[i * 3 + 0];
        uint8_t g = bitmap[i * 3 + 1];
        uint8_t b = bitmap[i * 3 + 2];

        buffer[i] = (r + b + g) / 3.0f;
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
#include <assert.h>
#include <dwrite.h>

struct oc_collection_impl {
    IDWriteFactory* dw_factory;
    char** families;
    UINT32 nfamilies;
};

typedef struct {
    IDWriteFactory* dw_factory;
    IDWriteFont* dw_font;
    oc_font font;
} oc__font_impl;

static inline void oc__free_font(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    impl->dw_font->lpVtbl->Release(impl->dw_font);
    free(impl);
}

oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_collection collection = { 0 };
    oc_error err = oc_error_ok;

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    collection.impl = calloc(1, sizeof(oc_collection_impl));
    if (collection.impl == NULL) {
        return oc_error_out_of_memory;
    }

    collection.impl->dw_factory = library->internals;
exit:
    if (ocollection)
        *ocollection = collection;
    return err;
}

void oc_free_collection(oc_collection* collection) {
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

static oc_font* oc__init_font(IDWriteFactory* dw_factory, IDWriteFont* dw_font, const char* family) {
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE style;

    oc__font_impl* impl;

    weight = dw_font->lpVtbl->GetWeight(dw_font);
    style = dw_font->lpVtbl->GetStyle(dw_font);

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        return NULL;
    }

    impl->dw_factory = dw_factory;
    impl->dw_font = dw_font;
    impl->font.family = family;
    impl->font.weight = (uint16_t)weight;
    impl->font.slant = oc__slant_map[style];

    return &impl->font;
}

oc_error oc_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;
    HRESULT hr;

    IDWriteFactory* dw_factory;
    IDWriteFontCollection* dw_collection = NULL;

    UINT32 family_count;
    UINT32 font_count;

    union {
        char* str;
        UINT32 len;
    }* families = NULL;
    UINT32 nfamilies = 0;

    WCHAR* wide_buf = NULL;
    UINT32 wide_buf_len;

    oc_font** fonts = NULL;
    uint32_t nfonts = 0;

    oc_collection tmp_collection;
    oc_collection_impl tmp_impl;

    if (!collection) {
        oc__exit(oc_error_invalid_param);
    }

    dw_factory = collection->impl->dw_factory;
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
        IDWriteFontFamily* family;
        IDWriteLocalizedStrings* names;
        UINT32 length;

        hr = dw_collection->lpVtbl->GetFontFamily(dw_collection, i, &family);
        assert(hr == S_OK);

        hr = family->lpVtbl->GetFamilyNames(family, &names);
        assert(hr == S_OK);

        // todo: test this locale idx feature an other backends
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
        IDWriteFontFamily* font_family;
        IDWriteLocalizedStrings* names;

        UINT32 wide_length = families[i].len;
        UINT32 font_index;

        char* family;
        int length;

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
            oc_font* font;

            hr = font_family->lpVtbl->GetFont(font_family, font_index, &dw_font);
            assert(hr == S_OK);

            font = oc__init_font(dw_factory, dw_font, family);
            if (font == NULL) {
                font_family->lpVtbl->Release(font_family);
                oc__exit(oc_error_out_of_memory);
            }

            fonts[nfonts++] = font;
        }

        font_family->lpVtbl->Release(font_family);
    }
done:
    tmp_impl.dw_factory = dw_factory;
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
    IDWriteFont* dw_font;

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

oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;
    IDWriteFontFace* face;
    HRESULT result;

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    result = impl->dw_font->lpVtbl->CreateFontFace(impl->dw_font, &face);

    switch (result) {
    case S_OK:
        break;
    default:
        return oc__unexpected(result);
    }

    if (desired_size == 0) {
        desired_size = 12 << 6;
    } else if (desired_size < 1 << 6) {
        desired_size = 1 << 6;
    }

    if (dpi == 0) {
        dpi = 72;
    }
    
    return oc__init_face(impl->dw_factory, face, desired_size, dpi, oface);
}

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    HRESULT result; 

    IDWriteFontFace* face;
    IDWriteFontFile* file;

    UINT32 nfiles;

    const void* key;
    UINT32 key_size;

    IDWriteFontFileLoader* loader;
    IDWriteLocalFontFileLoader* local_loader;

    WCHAR* wide_path;
    UINT32 wide_len;

    int path_len;
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
