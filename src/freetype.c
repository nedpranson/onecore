#define ONECORE_IMPLEMENTATION
#include "onecore.h"

/* ONECORE_FREETYPE_IMPLEMENTATION */
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
        return oc__unexpected(err);
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
        return oc__unexpected(err);
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
        return oc__unexpected(err);
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
        return oc__unexpected(err);
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
        return oc__unexpected(err);
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
            return oc__unexpected(err);
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
        return oc__unexpected(err);
    }

    err = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1);
    glyph_bitmap = (FT_BitmapGlyph)glyph;

    if (err != FT_Err_Ok) {
        FT_Done_Glyph(glyph);
        switch (err) {
        case FT_Err_Out_Of_Memory:
            return oc_error_out_of_memory;
        default:
            return oc__unexpected(err);
        }
    }

    assert(glyph_bitmap->bitmap.rows == bitmap.rows);
    assert(glyph_bitmap->bitmap.width == bitmap.width);
    assert(glyph_bitmap->bitmap.pitch == bitmap.pitch);

    memcpy(buffer, glyph_bitmap->bitmap.buffer, bitmap.rows * bitmap.width);

    FT_Done_Glyph(glyph);
    return oc_error_ok;
}
