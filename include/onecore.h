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

// OC_PUBLIC oc_16p16
// oc_div_16p16(oc_16p16 a, oc_16p16 b);

// OC_PUBLIC oc_16p16
// oc_mul_16p16(oc_16p16 a, oc_16p16 b);

#define oc_div_16p16(a, b) ((int32_t)FT_DivFix((int32_t)(a), (int32_t)(b)))
#define oc_mul_16p16(a, b) ((int32_t)FT_MulFix((int32_t)(a), (int32_t)(b)))

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* INCLUDE_ONECORE_H */

/******************************************************************************************************/
/*                                                                                                    */
/*                                           IMPLEMENTATION                                           */
/*                                                                                                    */
/******************************************************************************************************/

#if defined(ONECORE_FREETYPE_IMPLEMENTATION) && !defined(ONECORE_IMPLEMENTATION)
#define ONECORE_IMPLEMENTATION
#endif

#ifdef ONECORE_IMPLEMENTATION
#include <pthread.h>
#include <assert.h>

#define OC_ASSERT(x) assert(x)

#define oc_div_16p16(a, b) ((int32_t)FT_DivFix((int32_t)(a), (int32_t)(b)))
#define oc_mul_16p16(a, b) ((int32_t)FT_MulFix((int32_t)(a), (int32_t)(b)))

typedef pthread_mutex_t oc_mutex_impl_t;

#define oc_mutex_impl_init(m) pthread_mutex_init(m, NULL)
#define oc_mutex_impl_lock(m) pthread_mutex_lock(m)
#define oc_mutex_impl_unlock(m)	pthread_mutex_unlock(m)
#define oc_mutex_impl_destroy(m) pthread_mutex_destroy(m)

#define oc_unexpected(e) oc_error_unexpected

static inline oc_open_params oc_open_params_defaults(const oc_open_params* pparams) {
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

// ONECORE_???_FREETYPE_IMPLEMENTATION
// ONECORE_???_CORETEXT_IMPLEMENTATION
// ONECORE_???_DIRECTWRITE_IMPLEMENTATION
//
// ONECORE_???_FONTCONFIG_IMPLEMENTATION
// ONECORE_???_CORETEXT_IMPLEMENTATION
// ONECORE_???_DIRECTWRITE_IMPLEMENTATION

#ifdef ONECORE_FREETYPE_IMPLEMENTATION
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

struct oc_face_impl {
    FT_Face ft_face;
    oc_mutex_impl_t lock;
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

static oc_error init_face(FT_Face ft_face, const oc_open_params* pparams, oc_face* pface) {
    FT_Error err = FT_Set_Char_Size(ft_face, 0, pparams->desired_size, pparams->dpi, pparams->dpi);
    if (err != FT_Err_Ok) {
        return oc_unexpected(err);
    }

    oc_face_impl* impl = (oc_face_impl*)malloc(sizeof(oc_face_impl));
    if (impl == NULL) {
        return oc_error_out_of_memory;
    }

    // todo: think
    // as dwrite and freetype needs say 2 args
    // we can add void* oc_face::reserved

    impl->ft_face = ft_face;
    oc_mutex_impl_init(&impl->lock);

    // printf("y_ppem: %d, x_ppem: %d\n", ft_face->size->metrics.y_ppem, ft_face->size->metrics.x_ppem);
    // printf("my_sclae_y: %ld\n", FT_DivFix(((long)(pparams->desired_size * 64.0f) * pparams->dpi + 32) / 72, ft_face->units_per_EM));

    // printf("scale_x: %ld\n", ft_face->size->metrics.y_scale);
    // printf("mul: %ld\n", FT_MulFix(677, ft_face->size->metrics.y_scale));

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

    oc_open_params params = oc_open_params_defaults(pparams);

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

    oc_error oc_err = init_face(face, &params, pface);
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

    oc_open_params params = oc_open_params_defaults(pparams);
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

    oc_error oc_err = init_face(face, &params, pface);
    if (oc_err != oc_error_ok) {
        FT_Done_Face(face);
    }

    return oc_err;
}

void oc_free_face(oc_face face) {
    FT_Done_Face(face.impl->ft_face);
    oc_mutex_impl_destroy(&face.impl->lock);

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

    // if other abis allow we can add offset option
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
    OC_ASSERT(err == oc_error_ok);

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

    oc_mutex_impl_lock(&face.impl->lock);
    FT_Error err = FT_Load_Glyph(face.impl->ft_face, glyph_index, ft_load_flags);
    if (err != FT_Err_Ok) {
        oc_mutex_impl_unlock(&face.impl->lock);
        memset(pmetrics, 0, sizeof(oc_glyph_metrics));
        return;
    }

    FT_GlyphSlot slot = face.impl->ft_face->glyph;
    FT_Glyph_Metrics glyph_metrics = slot->metrics;
    oc_mutex_impl_unlock(&face.impl->lock);

    pmetrics->width = glyph_metrics.width;
    pmetrics->height = glyph_metrics.height;
    pmetrics->bearing_x = glyph_metrics.horiBearingX;
    pmetrics->bearing_y = glyph_metrics.horiBearingY;
    pmetrics->advance = glyph_metrics.horiAdvance;
}

typedef struct oc_outline_context {
    const oc_outline_funcs* funcs;
    void* ctx;

    FT_Vector x2origin;
    bool figure_started;
} oc_outline_context;

static int oc_move_to(const FT_Vector* to, void* user) {
    oc_outline_context* ctx = (oc_outline_context*)user;
    oc_point point = { (int32_t)(to->x >> 1), (int32_t)(to->y >> 1) };

    if (ctx->figure_started) {
        ctx->funcs->end_figure(ctx->ctx);
    }

    ctx->funcs->start_figure(point, ctx->ctx);
    ctx->x2origin = *to;
    ctx->figure_started = true;

    return 0;
}

static int oc_line_to(const FT_Vector* x2to, void* user) {
    oc_outline_context* ctx = (oc_outline_context*)user;
    oc_point point = { (int32_t)(x2to->x >> 1), (int32_t)(x2to->y >> 1) };

    ctx->funcs->line_to(point, ctx->ctx);
    ctx->x2origin = *x2to;

    return 0;
}

typedef struct point_2f {
    float x;
    float y;
} point_2f;

static int oc_conic_to(const FT_Vector* x2control, const FT_Vector* x2to, void* user) {
    oc_outline_context* ctx = (oc_outline_context*)user;

    point_2f forigin = { (float)ctx->x2origin.x * 0.5f, (float)ctx->x2origin.y * 0.5f };
    point_2f fto = { (float)x2to->x * 0.5f, (float)x2to->y * 0.5f };

    // comes extremely closes to dwrites internal implemintation
    // but is not 100% perfect
    point_2f cubic[2];
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

static int oc_cubic_to(const FT_Vector* x2c1, const FT_Vector* x2c2, const FT_Vector* x2to, void* user) {
    oc_outline_context* ctx = (oc_outline_context*)user;

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

    oc_mutex_impl_lock(&face.impl->lock);
    err = FT_Load_Glyph(face.impl->ft_face, glyph_index, ft_load_flags);
    if (err != FT_Err_Ok) {
        memset(pbbox, 0, sizeof(oc_bbox));
        oc_mutex_impl_unlock(&face.impl->lock);
    }

    FT_Outline_Get_CBox(&face.impl->ft_face->glyph->outline, &bbox);
    oc_mutex_impl_unlock(&face.impl->lock);

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

    oc_mutex_impl_lock(&face.impl->lock);
    err = FT_Load_Glyph(face.impl->ft_face, glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);
    if (err != FT_Err_Ok) {
        oc_mutex_impl_unlock(&face.impl->lock);
        return false;
    }

    FT_GlyphSlot slot = face.impl->ft_face->glyph;
    FT_Outline glyph_outline = slot->outline;

    if (slot->format != FT_GLYPH_FORMAT_OUTLINE && slot->format != FT_GLYPH_FORMAT_COMPOSITE) {
        oc_mutex_impl_unlock(&face.impl->lock);
        return false;
    }
    oc_mutex_impl_unlock(&face.impl->lock);

    oc_outline_context ctx = { 0 };
    ctx.funcs = outline_funcs;
    ctx.ctx = context;

    // shift is set to one as we want all point to be multiplied by 2
    // to restore conic 'to' position to its original floating point value
    static const FT_Outline_Funcs decompose_funcs = {
        oc_move_to,
        oc_line_to,
        oc_conic_to,
        oc_cubic_to,
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

    oc_mutex_impl_lock(&face.impl->lock);
    err = FT_Load_Glyph(face.impl->ft_face, glyph_index, FT_LOAD_BITMAP_METRICS_ONLY | FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT);
    if (err != FT_Err_Ok) {
        oc_mutex_impl_unlock(&face.impl->lock);
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
        oc_mutex_impl_unlock(&face.impl->lock);
        // todo: implement diffrent types
        return oc_error_unexpected;
    }

    psize->rows = bitmap.rows;
    psize->cols = bitmap.width;

    if (buffer == NULL) {
        oc_mutex_impl_unlock(&face.impl->lock);
        return oc_error_ok;
    }

    if (bitmap.rows == 0 || bitmap.width == 0) {
        oc_mutex_impl_unlock(&face.impl->lock);
        return oc_error_ok;
    }

    if (buffer_size < bitmap.rows * bitmap.width) {
        oc_mutex_impl_unlock(&face.impl->lock);
        return oc_error_insufficient_buffer;
    }

    FT_Glyph glyph;
    FT_BitmapGlyph glyph_bitmap;

    err = FT_Get_Glyph(face.impl->ft_face->glyph, &glyph);
    oc_mutex_impl_unlock(&face.impl->lock);

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

    OC_ASSERT(glyph_bitmap->bitmap.rows == bitmap.rows);
    OC_ASSERT(glyph_bitmap->bitmap.width == bitmap.width);
    OC_ASSERT(glyph_bitmap->bitmap.pitch == bitmap.pitch);

    memcpy(buffer, glyph_bitmap->bitmap.buffer, bitmap.rows * bitmap.width);

    FT_Done_Glyph(glyph);
    return oc_error_ok;
}
#endif /* ONECORE_FREETYPE_IMPLEMENTATION */
