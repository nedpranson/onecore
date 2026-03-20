#include "freetype/fttypes.h"
#include <stdint.h>
#define ONECORE_IMPLEMENTATION
#include "onecore.h"

/* ONECORE_FREETYPE_IMPLEMENTATION */
#include <assert.h>
// todo: make font config optional
#include <fontconfig/fontconfig.h>
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

#define oc__exit(e) \
    do {            \
        err = (e);  \
        goto exit;  \
    } while (0)

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

typedef struct {
    FcPattern* fc_pattern;
    oc_font font;
} oc__font_impl;

typedef enum {
    oc__status_ok,
    oc__status_memory,
    oc__status_skip,
} oc__status;

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

static inline void oc__free_font(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    free(impl);
}

oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error err = oc_error_ok;
    FcConfig* fc_config;
    oc_collection collection = { 0 };

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    fc_config = FcInitLoadConfig();
    if (fc_config == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    collection.impl = (oc_collection_impl*)fc_config;
exit:
    if (ocollection) *ocollection = collection;
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

        memset(collection, 0, sizeof(*collection));
    }
}

static oc__status oc__init_font(FcPattern* fc_pattern, oc_font** ofont) {
    FcResult result;
    FcValue weight_value;

    int weight;
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

    // todo: check if weight can be negative
    weight = FcWeightToOpenType(weight);
    assert(weight >= 0 && weight <= UINT16_MAX);

    result = FcPatternGetString(fc_pattern, FC_FAMILY, 0, &family);
    assert(result == FcResultMatch);
    assert(family != NULL);

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        return oc__status_memory;
    }

    impl->fc_pattern = fc_pattern;
    impl->font.family = (char*)family;
    impl->font.weight = (uint16_t)weight;

    *ofont = &impl->font;
    return oc__status_ok;
}

oc_error oc_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;

    FcConfig* fc_config;
    FcFontSet* fc_fonts;

    int font_count;

    oc_font** fonts = NULL;
    uint32_t nfonts = 0;

    oc_collection tmp_collection;

    if (!collection) {
        oc__exit(oc_error_invalid_param);
    }

    fc_config = (FcConfig*)collection->impl;
    if (!FcConfigBuildFonts(fc_config)) {
        oc__exit(oc_error_invalid_param);
    }

    // todo: check what it does if there is no system fonts
    fc_fonts = FcConfigGetFonts(fc_config, FcSetSystem);
    assert(fc_fonts != NULL); // todo: look source code check if this can return NULL

    font_count = fc_fonts->nfont;
    fonts = malloc(font_count * sizeof(*fonts));

    if (fonts == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    for (int i = 0; i < font_count; i++) {
        oc__status status;

        FcPattern* pattern;
        oc_font* font;

        pattern = fc_fonts->fonts[i];
        status = oc__init_font(pattern, &font);

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

    tmp_collection.impl = (oc_collection_impl*)fc_config;
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

    fonts = collection->fonts;
    nfonts = collection->nfonts;

    *collection = tmp_collection;
exit:
    while (nfonts--) oc__free_font(fonts[nfonts]);
    free(fonts);

    return err;
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

    face.metrics.upem = ft_face->units_per_EM;
    face.metrics.ppem = ft_face->size->metrics.y_ppem;
    face.metrics.scale = ft_face->size->metrics.y_scale;
    face.metrics.ascent = ft_face->ascender;
    face.metrics.descent = -ft_face->descender;
    face.metrics.leading = ft_face->height - ft_face->ascender + ft_face->descender;
    // reverting ajusted underline position by freetype
    face.metrics.underline_position = ft_face->underline_position + (ft_face->underline_thickness >> 1);
    face.metrics.underline_thickness = ft_face->underline_thickness;

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

    face->metrics.ppem = ft_face->size->metrics.y_ppem;
    face->metrics.scale = ft_face->size->metrics.y_scale;

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
    length = *size;

    assert(length == 0 || length >= offset);

    err = FT_Load_Sfnt_Table(ft_face, (FT_ULong)tag, (FT_ULong)offset, (FT_Byte*)data, &length);
    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Table_Missing:
        return oc_error_table_missing;
    default:
        return oc__unexpected(err);
    }

    assert(UINT32_MAX >= length);

    *size = length;
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
exit:
    if (ometrics) *ometrics = metrics;
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
    if (ocbox) *ocbox = cbox;
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
    FT_Bitmap ft_bitmap;
    FT_Error ft_err;
    FT_Glyph ft_glyph = NULL;
    oc_error err = oc_error_ok;
    oc_extent extent = { 0 };

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

    if (buffer_size < extent.rows * extent.cols) {
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

    memcpy(buffer, ((FT_BitmapGlyph)ft_glyph)->bitmap.buffer, extent.rows * extent.cols);

exit:
    if (ft_glyph) FT_Done_Glyph(ft_glyph);
    if (oextent) *oextent = extent;

    return err;
}
