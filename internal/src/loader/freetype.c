#include "internal/src/onecore.h"
#define ONECORE_IMPLEMENTATION
#define OC__OVERRIDE_LIBRARY_IMPL
#include "onecore.h"

/* ONECORE_FREETYPE_LOADER_IMPLEMENTATION */
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

typedef struct {
    const oc_outline_funcs* funcs;
    void*                   ctx;

    FT_Vector x2origin;
    bool      figure_started;
} oc__outline_context;

static int oc__move_to(const FT_Vector* to, void* user) {
    oc__outline_context* ctx = (oc__outline_context*)user;
    oc_point             point = { (int32_t)(to->x >> 1), (int32_t)(to->y >> 1) };

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
    oc_point             point = { (int32_t)(x2to->x >> 1), (int32_t)(x2to->y >> 1) };

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

    // comes extremely close to dwrites internal implemintation
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

bool ocl_get_outline(const oc_face* face, uint16_t index, oc_load_flags flags, const oc_outline_funcs* funcs, void* user) {
    FT_Error            err;
    FT_Face             ft_face;
    oc__mutex_impl_t*   lock;
    FT_GlyphSlot        glyph;
    FT_Outline          outline;
    FT_Int32            ft_load_flags = FT_LOAD_NO_BITMAP;
    oc__outline_context context = { 0 };

    if (!(face && funcs)) {
        goto exit;
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
    err = FT_Load_Glyph(ft_face, index, ft_load_flags);
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
    } else for (uint32_t y = 0; y < extent.rows; y++) {
        memcpy(buffer + y * pitch, ((FT_BitmapGlyph)ft_glyph)->bitmap.buffer + y * extent.cols, extent.cols);
    }
exit:
    if (ft_glyph)
        FT_Done_Glyph(ft_glyph);
    if (oextent)
        *oextent = extent;

    return err;
}
