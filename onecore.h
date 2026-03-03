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
// todo: implement this!
#define oc_unexpected(e) oc_error_unexpected

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

static inline oc_open_params oc__open_params_defaults(const oc_open_params* pparams) {
    if (pparams == NULL)
        return (oc_open_params) {
            .face_index = 0,
            .desired_size = 12 << 6,
            .dpi = 96,
        };

    oc_open_params params = *pparams;

    if (params.desired_size <= 0) {
        params.desired_size = 12 << 6;
    }

    if (params.dpi <= 0) {
        params.dpi = 96;
    }

    return params;
}
#endif /* ONECORE_IMPLEMENTATION */

#ifdef ONECORE_FREETYPE_IMPLEMENTATION
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

struct oc_face_impl {
    FT_Face ft_face;
    oc__mutex_impl_t lock;
};

oc_error oc_init_library(oc_library* plibrary) {
    if (plibrary == NULL) {
        return oc_error_invalid_param;
    }

    FT_Library library;
    FT_Error err = FT_Init_FreeType(&library);
    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    default:
        return oc_unexpected(err);
    }

    plibrary->internals = library;
    return oc_error_ok;
}

inline void oc_free_library(oc_library library) {
    FT_Done_FreeType(((FT_Library)(library).internals));
}

static oc_error oc__init_face(FT_Face ft_face, const oc_open_params* pparams, oc_face* pface) {
    FT_Error err = FT_Set_Char_Size(ft_face, 0, pparams->desired_size, pparams->dpi, pparams->dpi);
    if (err != FT_Err_Ok) {
        return oc_unexpected(err);
    }

    oc_face_impl* impl = (oc_face_impl*)malloc(sizeof(oc_face_impl));
    if (impl == NULL) {
        return oc_error_out_of_memory;
    }

    impl->ft_face = ft_face;
    oc__mutex_impl_init(&impl->lock);

    pface->impl = impl;
    pface->metrics.upem = ft_face->units_per_EM;
    pface->metrics.ppem = ft_face->size->metrics.y_ppem;
    pface->metrics.scale = ft_face->size->metrics.y_scale;
    pface->metrics.ascent = ft_face->ascender;
    pface->metrics.descent = -ft_face->descender;
    pface->metrics.leading = ft_face->height - ft_face->ascender + ft_face->descender;
    // reverting ajusted underline position by freetype
    pface->metrics.underline_position = ft_face->underline_position + (ft_face->underline_thickness >> 1);
    pface->metrics.underline_thickness = ft_face->underline_thickness;

    return oc_error_ok;
}

oc_error oc_open_face(oc_library library, const char* path, const oc_open_params* pparams, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (path == NULL) {
        return oc_error_invalid_param;
    }

    FT_Error err;
    FT_Face face;

    FT_Open_Args open_args = { 0 };
    open_args.flags = FT_OPEN_PATHNAME;
    open_args.pathname = (char*)path;

    oc_open_params params = oc__open_params_defaults(pparams);

    // using FT_Open_Face as FT_New_Face fails if file extention does not match file type
    err = FT_Open_Face(((FT_Library)(library).internals), &open_args, params.face_index, &face);
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
        return oc_unexpected(err);
    }

    oc_error oc_err = oc__init_face(face, &params, pface);
    if (oc_err != oc_error_ok) {
        FT_Done_Face(face);
    }

    return oc_err;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, const oc_open_params* pparams, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    FT_Face face;
    FT_Error err;

    oc_open_params params = oc__open_params_defaults(pparams);
    err = FT_New_Memory_Face(((FT_Library)(library).internals), (const FT_Byte*)data, size, params.face_index, &face);
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
        return oc_unexpected(err);
    }

    oc_error oc_err = oc__init_face(face, &params, pface);
    if (oc_err != oc_error_ok) {
        FT_Done_Face(face);
    }

    return oc_err;
}

void oc_free_face(oc_face face) {
    FT_Done_Face(face.impl->ft_face);
    oc__mutex_impl_destroy(&face.impl->lock);

    free(face.impl);
}

inline uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    return FT_Get_Char_Index(face.impl->ft_face, charcode);
}

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable, void** pcontext) {
    FT_Error err;

    if (ptable == NULL || pcontext == NULL) {
        return oc_error_invalid_param;
    }

    // todo: add offset option
    FT_ULong size = 0;
    err = FT_Load_Sfnt_Table(face.impl->ft_face, tag, 0, NULL, &size);
    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Table_Missing:
        return oc_error_table_missing;
    default:
        return oc_unexpected(err);
    }

    uint8_t* buffer = (uint8_t*)malloc(size);
    if (buffer == NULL) {
        return oc_error_out_of_memory;
    }

    err = FT_Load_Sfnt_Table(face.impl->ft_face, tag, 0, buffer, &size);
    assert(err == oc_error_ok);

    oc_table table;
    table.data = buffer;
    table.size = size;

    *ptable = table;
    *pcontext = buffer;

    return oc_error_ok;
}

inline void oc_free_table(oc_face face, void* context) {
    (void)face;
    free(context);
}

// todo: add option for verticals and maybe load both hori and vert bearings, advances
void oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_glyph_metrics* pmetrics) {
    if (pmetrics == NULL) {
        return;
    }

    FT_Int32 ft_load_flags = FT_LOAD_NO_AUTOHINT | FT_LOAD_BITMAP_METRICS_ONLY | FT_LOAD_NO_HINTING;
    if (flags & OC_LOAD_NO_SCALE) {
        ft_load_flags |= FT_LOAD_NO_SCALE;
    }

    //if (flags & OC_LOAD_NO_HINTING) {
        //ft_load_flags |= FT_LOAD_NO_HINTING;
    //}

    oc__mutex_impl_lock(&face.impl->lock);
    FT_Error err = FT_Load_Glyph(face.impl->ft_face, glyph_index, ft_load_flags);
    if (err != FT_Err_Ok) {
        oc__mutex_impl_unlock(&face.impl->lock);
        memset(pmetrics, 0, sizeof(oc_glyph_metrics));
        return;
    }

    FT_GlyphSlot slot = face.impl->ft_face->glyph;
    FT_Glyph_Metrics glyph_metrics = slot->metrics;
    oc__mutex_impl_unlock(&face.impl->lock);

    pmetrics->width = glyph_metrics.width;
    pmetrics->height = glyph_metrics.height;
    pmetrics->bearing_x = glyph_metrics.horiBearingX;
    pmetrics->bearing_y = glyph_metrics.horiBearingY;
    pmetrics->advance = glyph_metrics.horiAdvance;
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

void oc_get_glyph_bbox(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_bbox* pbbox) {
    if (pbbox == NULL) {
        return;
    }

    FT_Error err;
    FT_BBox bbox;

    FT_Int32 ft_load_flags = FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING;
    if (flags & OC_LOAD_NO_SCALE) {
        ft_load_flags |= FT_LOAD_NO_SCALE;
    }

    //if (flags & OC_LOAD_NO_HINTING) {
        //ft_load_flags |= FT_LOAD_NO_HINTING;
    //}

    oc__mutex_impl_lock(&face.impl->lock);
    err = FT_Load_Glyph(face.impl->ft_face, glyph_index, ft_load_flags);
    if (err != FT_Err_Ok) {
        memset(pbbox, 0, sizeof(oc_bbox));
        oc__mutex_impl_unlock(&face.impl->lock);
    }

    FT_Outline_Get_CBox(&face.impl->ft_face->glyph->outline, &bbox);
    oc__mutex_impl_unlock(&face.impl->lock);

    pbbox->min_x = bbox.xMin;
    pbbox->min_y = bbox.yMin;
    pbbox->max_x = bbox.xMax;
    pbbox->max_y = bbox.yMax;
}

bool oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    FT_Error err;
    if (outline_funcs == NULL) {
        return false;
    }

    oc__mutex_impl_lock(&face.impl->lock);
    err = FT_Load_Glyph(face.impl->ft_face, glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);
    if (err != FT_Err_Ok) {
        oc__mutex_impl_unlock(&face.impl->lock);
        return false;
    }

    FT_GlyphSlot slot = face.impl->ft_face->glyph;
    FT_Outline glyph_outline = slot->outline;

    if (slot->format != FT_GLYPH_FORMAT_OUTLINE && slot->format != FT_GLYPH_FORMAT_COMPOSITE) {
        oc__mutex_impl_unlock(&face.impl->lock);
        return false;
    }
    oc__mutex_impl_unlock(&face.impl->lock);

    oc__outline_context ctx = { 0 };
    ctx.funcs = outline_funcs;
    ctx.ctx = context;

    // shift is set to one as we want all point to be multiplied by 2
    // to restore conic 'to' position to its original floating point value
    static const FT_Outline_Funcs decompose_funcs = {
        oc__move_to,
        oc__line_to,
        oc__conic_to,
        oc__cubic_to,
        1,
        0,
    };

    err = FT_Outline_Decompose(&glyph_outline, &decompose_funcs, &ctx);
    if (err != FT_Err_Ok) {
        return false;
    }

    if (ctx.figure_started) {
        ctx.funcs->end_figure(ctx.ctx);
    }

    return true;
}

oc_error oc_render_glyph(oc_face face, uint16_t glyph_index, oc_size* psize, unsigned char* buffer, size_t buffer_size) {
    FT_Error err;
    if (psize == NULL) {
        return oc_error_invalid_param;
    }

    oc__mutex_impl_lock(&face.impl->lock);
    err = FT_Load_Glyph(face.impl->ft_face, glyph_index, FT_LOAD_BITMAP_METRICS_ONLY | FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT);
    if (err != FT_Err_Ok) {
        oc__mutex_impl_unlock(&face.impl->lock);
        switch (err) {
        case FT_Err_Out_Of_Memory:
            return oc_error_out_of_memory;
        case FT_Err_Invalid_Argument:
            return oc_error_invalid_param;
        default:
            return oc_unexpected(err);
        }
    }

    FT_Bitmap bitmap = face.impl->ft_face->glyph->bitmap;
    if ((int)bitmap.width != bitmap.pitch) {
        oc__mutex_impl_unlock(&face.impl->lock);
        // todo: implement diffrent types
        return oc_error_unexpected;
    }

    psize->rows = bitmap.rows;
    psize->cols = bitmap.width;

    if (buffer == NULL) {
        oc__mutex_impl_unlock(&face.impl->lock);
        return oc_error_ok;
    }

    if (bitmap.rows == 0 || bitmap.width == 0) {
        oc__mutex_impl_unlock(&face.impl->lock);
        return oc_error_ok;
    }

    if (buffer_size < bitmap.rows * bitmap.width) {
        oc__mutex_impl_unlock(&face.impl->lock);
        return oc_error_insufficient_buffer;
    }

    FT_Glyph glyph;
    FT_BitmapGlyph glyph_bitmap;

    err = FT_Get_Glyph(face.impl->ft_face->glyph, &glyph);
    oc__mutex_impl_unlock(&face.impl->lock);

    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    default:
        return oc_unexpected(err);
    }

    err = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1);
    glyph_bitmap = (FT_BitmapGlyph)glyph;

    if (err != FT_Err_Ok) {
        FT_Done_Glyph(glyph);
        switch (err) {
        case FT_Err_Out_Of_Memory:
            return oc_error_out_of_memory;
        default:
            return oc_unexpected(err);
        }
    }

    assert(glyph_bitmap->bitmap.rows == bitmap.rows);
    assert(glyph_bitmap->bitmap.width == bitmap.width);
    assert(glyph_bitmap->bitmap.pitch == bitmap.pitch);

    memcpy(buffer, glyph_bitmap->bitmap.buffer, bitmap.rows * bitmap.width);

    FT_Done_Glyph(glyph);
    return oc_error_ok;
}
#endif /* ONECORE_FREETYPE_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_IMPLEMENTATION
#include <CoreText/CoreText.h>

inline oc_error oc_init_library(oc_library* plibrary) {
    return plibrary == NULL ? oc_error_invalid_param : oc_error_ok;
}

inline void oc_free_library(oc_library library) {
    (void)library;
}

static oc_error oc__open_face_from_descriptors(CFArrayRef cf_descriptors_ref, const oc_open_params* pparams, oc_face* pface) {
    CFIndex count = CFArrayGetCount(cf_descriptors_ref);
    if (count == 0) {
        return oc_error_failed_to_open;
    }

    oc_open_params params = oc__open_params_defaults(pparams);
    if (params.face_index >= count) {
        return oc_error_invalid_param;
    }

    CTFontDescriptorRef ctf_descriptor_ref = (CTFontDescriptorRef)CFArrayGetValueAtIndex(cf_descriptors_ref, params.face_index);
    if (ctf_descriptor_ref == NULL) {
        return oc_error_out_of_memory;
    }

    oc_16p16 scaled = (params.desired_size * params.dpi + 36) / 72;
    CTFontRef ct_font_ref = CTFontCreateWithFontDescriptor(ctf_descriptor_ref, scaled / 64.0, NULL);
    if (ct_font_ref == NULL) {
        return oc_error_out_of_memory;
    }

    oc_16p16 scale = oc_div_16p16(scaled, CTFontGetUnitsPerEm(ct_font_ref));
    int32_t ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        // todo: add this test case
        return oc_error_invalid_param;
    }

    CGFloat size = CTFontGetSize(ct_font_ref);
    uint16_t upem = CTFontGetUnitsPerEm(ct_font_ref);

    pface->impl = (oc_face_impl*)ct_font_ref;
    pface->metrics.upem = upem;
    pface->metrics.ppem = ppem;
    pface->metrics.scale = scale;
    pface->metrics.ascent = (CTFontGetAscent(ct_font_ref) * upem / size);
    pface->metrics.descent = (CTFontGetDescent(ct_font_ref) * upem / size);
    pface->metrics.leading = (CTFontGetLeading(ct_font_ref) * upem / size);
    pface->metrics.underline_position = (CTFontGetUnderlinePosition(ct_font_ref) * upem / size);
    pface->metrics.underline_thickness = (CTFontGetUnderlineThickness(ct_font_ref) * upem / size);

    return oc_error_ok;
}

oc_error oc_open_face(oc_library library, const char* path, const oc_open_params* pparams, oc_face* pface) {
    (void)library;

    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (path == NULL) {
        return oc_error_invalid_param;
    }

    CFStringRef cf_path_ref = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    if (cf_path_ref == NULL) {
        // macos error handling sucks
        return oc_error_failed_to_open;
    }

    CFURLRef cf_url_ref = CFURLCreateWithFileSystemPath(NULL, cf_path_ref, kCFURLPOSIXPathStyle, false);
    CFRelease(cf_path_ref);

    if (cf_url_ref == NULL) {
        return oc_error_failed_to_open;
    }

    CFArrayRef cf_descriptors_ref = CTFontManagerCreateFontDescriptorsFromURL(cf_url_ref);
    CFRelease(cf_url_ref);

    if (cf_descriptors_ref == NULL) {
        return oc_error_failed_to_open;
    }

    oc_error err = oc__open_face_from_descriptors(cf_descriptors_ref, pparams, pface);
    CFRelease(cf_descriptors_ref);

    return err;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, const oc_open_params* pparams, oc_face* pface) {
    (void)library;

    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    CFDataRef cf_data_ref = CFDataCreateWithBytesNoCopy(NULL, data, size, kCFAllocatorNull);
    if (cf_data_ref == NULL) {
        return oc_error_out_of_memory;
    }

    CFArrayRef cf_descriptors_ref = CTFontManagerCreateFontDescriptorsFromData(cf_data_ref);
    CFRelease(cf_data_ref);

    if (cf_descriptors_ref == NULL) {
        return oc_error_failed_to_open;
    }

    oc_error err = oc__open_face_from_descriptors(cf_descriptors_ref, pparams, pface);
    CFRelease(cf_descriptors_ref);

    return err;
}

void oc_free_face(oc_face face) {
    CFRelease(face.impl);
}

uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    CTFontRef ct_font = (CTFontRef)face.impl;

    if (charcode > 0x10FFFF) {
        return 0;
    }

    // check out CFStringGetSurrogatePairForLongCharacter

    // CTFontGetGlyphsForCharacters writes cg_glyph[1] when the length is 2 (i.e. when encoding a surrogate pair)
    // in this case it will always be set to 0, but we still need to pass 2 elements
    // we reuse the second element to store the utf16 character sequence length
    CGGlyph cg_glyph[2];
    UniChar uni_char[2];

    if (charcode <= 0xFFFF) {
        uni_char[0] = charcode;
        cg_glyph[1] = 1;
    } else {
        uint32_t norm = charcode - 0x10000;
        uni_char[0] = (norm >> 10) + 0xD800;
        uni_char[1] = (norm & 0x3FF) + 0xDC00;
        cg_glyph[1] = 2;
    }

    // cg_glyph[0] will always be set by Core Text no matter the status
    // thus we can ignore returned value
    CTFontGetGlyphsForCharacters(
        ct_font,
        uni_char,
        cg_glyph,
        cg_glyph[1]);

    return cg_glyph[0];
}

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable, void** pcontext) {
    if (ptable == NULL || pcontext == NULL) {
        return oc_error_invalid_param;
    }


    CTFontRef ct_font = (CTFontRef)face.impl;
    CFDataRef cf_data_ref = CTFontCopyTable(ct_font, tag, kCTFontTableOptionNoOptions);
    if (cf_data_ref == NULL) {
        // todo: fix this it can be oom error
        return oc_error_table_missing;
    }

    oc_table table;
    table.data = CFDataGetBytePtr(cf_data_ref);
    table.size = CFDataGetLength(cf_data_ref);

    *ptable = table;
    *pcontext = (void*)cf_data_ref;

    return oc_error_ok;
}

inline void oc_free_table(oc_face face, void* context) {
    (void)face;
    CFRelease(context);
}

void oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_glyph_metrics* pmetrics) {
    if (pmetrics == NULL) {
        return;
    }

    oc_glyph_metrics metrics;
    CTFontRef ct_font = (CTFontRef)face.impl;

    CFIndex glyph_count = CTFontGetGlyphCount(ct_font);
    if (glyph_index >= glyph_count) {
        memset(pmetrics, 0, sizeof(oc_glyph_metrics));
        return;
    }

    CGSize advance;
    CTFontGetAdvancesForGlyphs(ct_font, kCTFontOrientationHorizontal, &glyph_index, &advance, 1);

    CGRect bbox = CTFontGetBoundingRectsForGlyphs(ct_font, kCTFontOrientationHorizontal, &glyph_index, NULL, 1);

    CGFloat size = CTFontGetSize(ct_font);
    uint16_t upem = face.metrics.upem;

    metrics.width = bbox.size.width * upem / size;
    metrics.height = bbox.size.height * upem / size;
    metrics.bearing_x = bbox.origin.x * upem / size;
    metrics.bearing_y = (bbox.size.height + bbox.origin.y) * upem / size;
    metrics.advance = advance.width * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        goto done;
        return;
    }

    metrics.width = oc_mul_16p16(metrics.width, face.metrics.scale);
    metrics.height = oc_mul_16p16(metrics.height, face.metrics.scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, face.metrics.scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, face.metrics.scale);
    metrics.advance = oc_mul_16p16(metrics.advance, face.metrics.scale);

done:
    *pmetrics = metrics;
}

void oc_get_glyph_bbox(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_bbox* pbbox) {
    if (pbbox == NULL) {
        return;
    }

    oc_bbox bbox;
    CGRect rect;
    CTFontRef ct_font = (CTFontRef)face.impl;

    CTFontGetBoundingRectsForGlyphs(
        ct_font,
        kCTFontOrientationHorizontal,
        &glyph_index,
        &rect,
        1);

    CGFloat size = CTFontGetSize(ct_font);
    uint16_t upem = face.metrics.upem;

    bbox.min_x = CGRectGetMinX(rect) * upem / size;
    bbox.min_y = CGRectGetMinY(rect) * upem / size;
    bbox.max_x = CGRectGetMaxX(rect) * upem / size;
    bbox.max_y = CGRectGetMaxY(rect) * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        goto done;
    }

    oc_16p16 scale = face.metrics.scale;
    bbox.min_x = oc_mul_16p16(bbox.min_x, scale);
    bbox.min_y = oc_mul_16p16(bbox.min_y, scale);
    bbox.max_x = oc_mul_16p16(bbox.max_x, scale);
    bbox.max_y = oc_mul_16p16(bbox.max_y, scale);

done:
    *pbbox = bbox;
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

bool oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    if (outline_funcs == NULL) {
        return false;
    }

    CTFontRef ct_font = (CTFontRef)face.impl;
    CGPathRef path = CTFontCreatePathForGlyph(ct_font, glyph_index, NULL);
    if (path == NULL) {
        return false;
    }

    oc__outline_context ctx = { 0 };
    ctx.funcs = outline_funcs;
    ctx.ctx = context;
    ctx.fsize = CTFontGetSize(ct_font);
    ctx.funits_per_em = CTFontGetUnitsPerEm(ct_font);

    CGPathApply(path, &ctx, oc__path_applier);
    CGPathRelease(path);

    return true;
}

// smth is off with mac!!!!
oc_error oc_render_glyph(oc_face face, uint16_t glyph_index, oc_size* psize, unsigned char* buffer, size_t buffer_size) {
    if (psize == NULL) {
        return oc_error_invalid_param;
    }

    CTFontRef ct_font = (CTFontRef)face.impl;
    CFIndex glyph_count = CTFontGetGlyphCount(ct_font);
    if (glyph_index >= glyph_count) {
        return oc_error_invalid_param;
    }

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L414
    oc_bbox cbox;
    oc_bbox pbox;
    oc_get_glyph_bbox(face, glyph_index, OC_LOAD_DEFAULT, &cbox);

    pbox.min_x = cbox.min_x >> 6;
    pbox.min_y = cbox.min_y >> 6;
    pbox.max_x = cbox.max_x >> 6;
    pbox.max_y = cbox.max_y >> 6;

    // take fractional part and ceil it
    pbox.max_x += ((cbox.max_x & 63) + 63) >> 6;
    pbox.max_y += ((cbox.max_y & 63) + 63) >> 6;

    uint32_t rows = pbox.max_y - pbox.min_y;
    uint32_t cols = pbox.max_x- pbox.min_x;

    psize->rows = rows;
    psize->cols = cols;

    if (buffer == NULL) {
        return oc_error_ok;
    }

    if (rows == 0 || cols == 0) {
        return oc_error_ok;
    }

    if (buffer_size < rows * cols) {
        return oc_error_insufficient_buffer;
    }

    CGColorSpaceRef linear_gray = CGColorSpaceCreateWithName(kCGColorSpaceLinearGray);
    if (linear_gray == NULL) {
        return oc_error_out_of_memory;
    }

    memset(buffer, 0, rows * cols);

    CGContextRef ctx = CGBitmapContextCreate(
        buffer,
        cols,
        rows,
        8,
        cols,
        linear_gray,
        kCGImageAlphaOnly);
    CGColorSpaceRelease(linear_gray);
    if (ctx == NULL) {
        return oc_error_out_of_memory;
    }

    CGRect fill_rect;
    fill_rect.origin.x = 0;
    fill_rect.origin.y = 0;
    fill_rect.size.height = rows;
    fill_rect.size.width = cols;

    // https://github.com/ghostty-org/ghostty/blob/main/src/font/face/coretext.zig#L478

    CGContextSetGrayFillColor(ctx, 0.0, 0.0);
    CGContextFillRect(ctx, fill_rect);

    CGContextSetAllowsFontSmoothing(ctx, false);
    CGContextSetShouldSmoothFonts(ctx, false);

    CGContextSetAllowsFontSubpixelPositioning(ctx, true);
    CGContextSetShouldSubpixelPositionFonts(ctx, true);

    CGContextSetAllowsFontSubpixelQuantization(ctx, false);
    CGContextSetShouldSubpixelQuantizeFonts(ctx, false);

    CGContextSetAllowsAntialiasing(ctx, true);
    CGContextSetShouldAntialias(ctx, true);

    CGContextSetGrayFillColor(ctx, 1.0, 1.0);
    CGContextSetGrayStrokeColor(ctx, 1.0, 1.0);

    CGContextTranslateCTM(ctx, (cbox.min_x & 63) / 64.0, (cbox.min_y & 63) / 64.0);

    CGPoint pos = {
        -cbox.min_x / 64.0,
        -cbox.min_y / 64.0,
    };

    CTFontDrawGlyphs(
        ct_font,
        &glyph_index,
        &pos,
        1,
        ctx);
    CGContextRelease(ctx);

    return oc_error_ok;
}
#endif /* ONECORE_CORETEXT_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_IMPLEMENTATION
#include <assert.h>
#include <initguid.h>

#include <d2d1.h>
#include <dwrite.h>

struct oc_face_impl {
    IDWriteFontFace* dw_face;
    IDWriteFactory* dw_factory;
};

// #define DW(x) _Generic((x),                       \
//     oc_library: ((IDWriteFactory*)(x).internals), \
//     oc_face: ((struct face_internals*)(x).internals)->face)

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

static const IDWriteFontFileLoaderVtbl OC__IDWriteFontFileLoaderVtbl  = {
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

oc_error oc_init_library(oc_library* plibrary) {
    if (plibrary == NULL) {
        return oc_error_invalid_param;
    }

    IDWriteFactory* dw_factory;
    HRESULT err;

    err = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_ISOLATED,
        &IID_IDWriteFactory,
        (IUnknown**)&dw_factory);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc_unexpected(err);
    }

    err = dw_factory->lpVtbl->RegisterFontFileLoader(dw_factory, oc__dw_file_loader);
    if (err != S_OK) {
        // DWRITE_E_ALREADYREGISTERED;
        dw_factory->lpVtbl->Release(dw_factory);
        return oc_unexpected(err);
    }

    plibrary->internals = dw_factory;
    return oc_error_ok;
}

void oc_free_library(oc_library library) {
    IDWriteFactory* dw_factory = (IDWriteFactory*)library.internals;

    dw_factory->lpVtbl->UnregisterFontFileLoader(dw_factory, oc__dw_file_loader);
    dw_factory->lpVtbl->Release(dw_factory);
}

static oc_error oc__open_face_from_font_file(oc_library library, IDWriteFontFile* font_file, const oc_open_params* pparams, oc_face* pface) {
    HRESULT err;
    WINBOOL is_supported_fonttype;
    IDWriteFactory* dw_factory;

    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;

    UINT32 face_num;

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
        return oc_unexpected(err);
    }

    if (!is_supported_fonttype) {
        return oc_error_failed_to_open;
    }

    IDWriteFontFace* dw_font_face;
    oc_open_params params = oc__open_params_defaults(pparams);

    dw_factory = (IDWriteFactory*)library.internals;

    // todo: we should handle simulations
    err = dw_factory->lpVtbl->CreateFontFace(
        dw_factory,
        face_type,
        1,
        &font_file,
        params.face_index,
        DWRITE_FONT_SIMULATIONS_NONE,
        &dw_font_face);

    switch (err) {
    case S_OK:
        break;
    case E_INVALIDARG:
        return oc_error_invalid_param;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc_unexpected(err);
    }

    oc_face_impl* impl = malloc(sizeof(oc_face_impl));
    if (impl == NULL) {
        dw_font_face->lpVtbl->Release(dw_font_face);
        return oc_error_out_of_memory;
    }

    impl->dw_face = dw_font_face;
    impl->dw_factory = dw_factory;

    DWRITE_FONT_METRICS metrics;
    dw_font_face->lpVtbl->GetMetrics(dw_font_face, &metrics);

    // todo: and make that point has to atleast 1.0

    // https://github.com/freetype/freetype/blob/85c8efe0afa5ad0df35114e317a065f544943c52/include/freetype/internal/ftobjs.h#L665
    oc_16p16 scaled = (params.desired_size * params.dpi + 36) / 72;
    oc_16p16 scale = oc_div_16p16(scaled, metrics.designUnitsPerEm);

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L3368
    int32_t ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        // todo: add this test case
        return oc_error_invalid_param;
    }

    pface->impl = impl;
    pface->metrics.upem = metrics.designUnitsPerEm;
    pface->metrics.ppem = (uint16_t)ppem;
    pface->metrics.scale = scale;
    pface->metrics.ascent = metrics.ascent;
    pface->metrics.descent = metrics.descent;
    pface->metrics.leading = metrics.lineGap;
    pface->metrics.underline_position = metrics.underlinePosition;
    pface->metrics.underline_thickness = metrics.underlineThickness;

    return oc_error_ok;
}

oc_error oc_open_face(oc_library library, const char* path, const oc_open_params* pparams, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (path == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0) - 1;
    if (wlen == -1) {
        return oc_error_failed_to_open;
    }

    if (wlen == 0) {
        return oc_error_failed_to_open;
    }

    wchar_t* wpath = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    if (wpath == NULL) {
        return oc_error_out_of_memory;
    }

    int ok = MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wlen + 1);

    (void)ok;
    assert(ok != 0);

    IDWriteFontFile* font_file;
    IDWriteFactory* dw_factory = library.internals;

    err = dw_factory->lpVtbl->CreateFontFileReference(
        dw_factory,
        wpath,
        NULL,
        &font_file);
    free(wpath);

    switch (err) {
    case S_OK:
        break;
    case DWRITE_E_FILENOTFOUND:
        return oc_error_failed_to_open;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc_unexpected(err);
    }

    oc_error result = oc__open_face_from_font_file(library, font_file, pparams, pface);
    font_file->lpVtbl->Release(font_file);

    return result;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, const oc_open_params* pparams, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err;
    IDWriteFontFile* font_file;
    IDWriteFactory* dw_factory = library.internals;

    oc__memory_view key = { data, size };

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
        return oc_unexpected(err);
    }

    oc_error result = oc__open_face_from_font_file(library, font_file, pparams, pface);
    font_file->lpVtbl->Release(font_file);

    return result;
}

void oc_free_face(oc_face face) {
    face.impl->dw_face->lpVtbl->Release(face.impl->dw_face);
    free(face.impl);
}

uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    UINT16 index;
    IDWriteFontFace* dw_face = face.impl->dw_face;

    HRESULT err = dw_face->lpVtbl->GetGlyphIndices(
        dw_face,
        &charcode,
        1,
        &index);
    (void)err;

    assert(err == S_OK);
    return index;
}

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable, void** pcontext) {
    if (ptable == NULL || pcontext == NULL) {
        return oc_error_invalid_param;
    }

    const void* table_data;
    UINT32 table_size;

    void* context;
    WINBOOL exists;

    HRESULT err = face.impl->dw_face->lpVtbl->TryGetFontTable(
        face.impl->dw_face,
        _byteswap_ulong(tag), // swapping bytes because windows table tags are little-endian
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
        return oc_unexpected(err);
    }

    if (exists == FALSE) {
        return oc_error_table_missing;
    }

    oc_table table;
    table.data = table_data;
    table.size = table_size;

    *ptable = table;
    *pcontext = context;

    return oc_error_ok;
}

inline void oc_free_table(oc_face face, void* context) {
    face.impl->dw_face->lpVtbl->ReleaseFontTable(face.impl->dw_face, context);
}

// static void fit_metrics(oc_glyph_metrics* pmetrics) {
//     oc_26p6 right = OC_26P6_CEIL(OC_26P6_ADD(pmetrics->bearing_x, pmetrics->width));
//     oc_26p6 bottom = OC_26P6_FLOOR(OC_26P6_SUB(pmetrics->bearing_y, pmetrics->height));
//
//     pmetrics->bearing_x = OC_26P6_FLOOR(pmetrics->bearing_x);
//     pmetrics->bearing_y = OC_26P6_CEIL(pmetrics->bearing_y);
//
//     pmetrics->width = OC_26P6_SUB(right, pmetrics->bearing_x);
//     pmetrics->height = OC_26P6_SUB(pmetrics->bearing_y, bottom);
//
//     pmetrics->advance = OC_26P6_ROUND(pmetrics->advance);
// }

void oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_glyph_metrics* pmetrics) {
    if (pmetrics == NULL) {
        return;
    }

    oc_glyph_metrics metrics;
    DWRITE_GLYPH_METRICS dw_metrics;
    IDWriteFontFace* dw_face = face.impl->dw_face;

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    UINT16 glyph_count = dw_face->lpVtbl->GetGlyphCount(dw_face);
    if (glyph_index >= glyph_count) {
        memset(pmetrics, 0, sizeof(oc_glyph_metrics));
        return;
    }

    HRESULT err = dw_face->lpVtbl->GetDesignGlyphMetrics(
        dw_face,
        &glyph_index,
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
        goto done;
    }

    metrics.width = oc_mul_16p16(metrics.width, face.metrics.scale);
    metrics.height = oc_mul_16p16(metrics.height, face.metrics.scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, face.metrics.scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, face.metrics.scale);
    metrics.advance = oc_mul_16p16(metrics.advance, face.metrics.scale);

    //if (flags & OC_LOAD_NO_HINTING) {
        //goto done;
    //}

    //fit_metrics(&metrics);

done:
    *pmetrics = metrics;
}

void oc_get_glyph_bbox(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_bbox* pbbox) {
    if (pbbox == NULL) {
        return;
    }

    oc_bbox bbox;
    DWRITE_GLYPH_METRICS metrics;
    IDWriteFontFace* dw_face = face.impl->dw_face;

    UINT16 glyph_count = dw_face->lpVtbl->GetGlyphCount(dw_face);
    if (glyph_index >= glyph_count) {
        memset(pbbox, 0, sizeof(oc_bbox));
        return;
    }

    HRESULT err = dw_face->lpVtbl->GetDesignGlyphMetrics(
        dw_face,
        &glyph_index,
        1,
        &metrics,
        FALSE);
    (void)err;
    assert(err == S_OK);

    bbox.min_x = metrics.leftSideBearing;
    bbox.min_y = metrics.verticalOriginY + metrics.bottomSideBearing - (INT32)metrics.advanceHeight;
    bbox.max_x = metrics.advanceWidth - metrics.rightSideBearing;
    bbox.max_y = metrics.verticalOriginY - metrics.topSideBearing;

    if (flags & OC_LOAD_NO_SCALE) {
        goto done;
    }

    bbox.min_x = oc_mul_16p16(bbox.min_x, face.metrics.scale);
    bbox.min_y = oc_mul_16p16(bbox.min_y, face.metrics.scale);
    bbox.max_x = oc_mul_16p16(bbox.max_x, face.metrics.scale);
    bbox.max_y = oc_mul_16p16(bbox.max_y, face.metrics.scale);

done:
    *pbbox = bbox;
}

bool oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    if (outline_funcs == NULL) {
        return false;
    }

    OC__ID2D1SimplifiedGeometrySink geometry_sink = { 0 };
    geometry_sink.lpVtbl = &OC__ID2D1SimplifiedGeometrySinkVtbl;
    geometry_sink.funcs = outline_funcs;
    geometry_sink.ref_count = 1;
    geometry_sink.ctx = context;

    HRESULT err = face.impl->dw_face->lpVtbl->GetGlyphRunOutline(
        face.impl->dw_face,
        face.metrics.upem,
        &glyph_index,
        NULL,
        NULL,
        1,
        FALSE,
        FALSE,
        (IDWriteGeometrySink*)&geometry_sink);

    if (err != S_OK) {
        return false;
    }

    ULONG refs = geometry_sink.lpVtbl->Base.Release((IUnknown*)&geometry_sink);

    (void)refs;
    assert(refs == 0);

    return true;
}

// todo: check this rendering thingy as smth is a bit off with dwrite
//       it seems dwrite does hard edges, idk if we can change that
oc_error oc_render_glyph(oc_face face, uint16_t glyph_index, oc_size* psize, unsigned char* buffer, size_t buffer_size) {
    HRESULT err;
    IDWriteFontFace* dw_face = face.impl->dw_face;
    IDWriteFactory* dw_factory = face.impl->dw_factory;

    if (psize == NULL) {
        return oc_error_invalid_param;
    }

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    UINT16 glyph_count = dw_face->lpVtbl->GetGlyphCount(dw_face);
    if (glyph_index >= glyph_count) {
        return oc_error_invalid_param;
    }

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L414
    oc_bbox cbox;
    oc_bbox pbox;
    oc_get_glyph_bbox(face, glyph_index, OC_LOAD_DEFAULT, &cbox);

    pbox.min_x = cbox.min_x >> 6;
    pbox.min_y = cbox.min_y >> 6;
    pbox.max_x = cbox.max_x >> 6;
    pbox.max_y = cbox.max_y >> 6;

    // take fractional part and ceil it
    pbox.max_x += ((cbox.max_x & 63) + 63) >> 6;
    pbox.max_y += ((cbox.max_y & 63) + 63) >> 6;

    uint32_t rows = pbox.max_y - pbox.min_y;
    uint32_t cols = pbox.max_x- pbox.min_x;

    // todo: add smth like this to our oc_render_glyph
    DWRITE_MATRIX transform = {
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        (cbox.min_x & 63) / 64.0f,
        -(cbox.min_y & 63) / 64.0f,
    };

    // is this correct?
    oc_26p6 em_size = oc_mul_16p16(face.metrics.upem, face.metrics.scale);

    DWRITE_GLYPH_RUN glyph_run = { 0 };
    glyph_run.fontFace = dw_face;
    glyph_run.fontEmSize = em_size / 64.0f;
    glyph_run.glyphCount = 1;
    glyph_run.glyphIndices = &glyph_index;

    IDWriteGlyphRunAnalysis* analysis;
    err = dw_factory->lpVtbl->CreateGlyphRunAnalysis(
        dw_factory,
        &glyph_run,
        1.0f,
        &transform,
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
        DWRITE_MEASURING_MODE_NATURAL,
        -cbox.min_x / 64.0f,
        cbox.min_y / 64.0f,
        &analysis);
    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc_unexpected(err);
    }

    psize->rows = rows;
    psize->cols = cols;

    if (buffer == NULL) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_ok;
    }

    if (rows == 0 || cols == 0) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_ok;
    }

    if (buffer_size < rows * cols) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_insufficient_buffer;
    }

    unsigned char* buffer_3x = malloc(rows * cols * 3);
    if (buffer_3x == NULL) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_out_of_memory;
    }

    RECT bounds = { 0 };
    bounds.top = -(int32_t)rows;
    bounds.right = cols;

    err = analysis->lpVtbl->CreateAlphaTexture(
        analysis,
        DWRITE_TEXTURE_CLEARTYPE_3x1,
        &bounds,
        buffer_3x,
        rows * cols * 3);
    analysis->lpVtbl->Release(analysis);

    if (err != S_OK) {
        free(buffer_3x);
        return oc_unexpected(err);
    }

    for (uint32_t i = 0; i < rows * cols; i++) {
        uint8_t r = buffer_3x[i * 3 + 0];
        uint8_t g = buffer_3x[i * 3 + 1];
        uint8_t b = buffer_3x[i * 3 + 2];

        buffer[i] = (r + b + g) / 3.0f;
    }

    free(buffer_3x);
    return oc_error_ok;
}
#endif /* ONECORE_DIRECTWRITE_IMPLEMENTATION */
