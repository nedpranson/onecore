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


struct oc_collection_impl {
    FcConfig* fc_config;
    FcFontSet* fc_font_set;
};

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


oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error err = oc_error_ok;
    oc_collection_impl* impl;
    oc_collection collection = { 0 };

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        goto exit;
    }

    impl->fc_config = FcInitLoadConfig();
    impl->fc_font_set = NULL;

    if (impl->fc_config == NULL) {
        err = oc_error_out_of_memory;
        free(impl);
        goto exit;
    }

    collection.impl = impl;
    collection.fonts = NULL;
    collection.elements = 0;
exit:
    if (ocollection) *ocollection = collection;
    return err;
}

void oc_free_collection(oc_collection* collection) {
    oc_collection_impl* impl;
    if (!collection) {
        return;
    }

    impl = collection->impl;

    FcConfigDestroy(impl->fc_config);
    free(impl);
    memset(collection, 0, sizeof(*collection));
}


oc_error oc_load_fonts(oc_collection* collection) {
    FcConfig* fc_config;
    FcFontSet* fc_font_set;

    if (!collection) {
        return oc_error_invalid_param;
    }

    fc_config = collection->impl->fc_config;
    if (!FcConfigBuildFonts(fc_config)) {
        return oc_error_out_of_memory;
    }

    // todo: check what it does if there is no system fonts
    fc_font_set = FcConfigGetFonts(fc_config, FcSetSystem);
    assert(fc_font_set != NULL);

    collection->elements = fc_font_set->nfont;
    collection->fonts = (oc_font**)fc_font_set->fonts;

    return oc_error_ok;
}

int oc_get_weight(const oc_font* font) {
    int fc_weight;
    int weight;

    if (!font) {
        return 0;
    }

    FcPatternGetInteger((FcPattern*)font, FC_WEIGHT, 0, &fc_weight);
    weight = FcWeightToOpenType(fc_weight);

    return weight;
}

// todo: abstract oc_get_family and oc_get_path under one method

// static const char* oc__font_get_string(const oc_font* font, const char* object) {
//     const FcPattern* fc_pattern;
//
//     FcChar8* string;
//     FcResult result;
//
//     if (!font) {
//         return NULL;
//     }
//
//     fc_pattern = (const FcPattern*)font;
//     result = FcPatternGetString(fc_pattern, object, 0, &string);
//
//     if (result != FcResultMatch) {
//         return NULL;
//     }
//
//     return (const char*)string;
// }

// const char* oc_get_family(const oc_font* font) {
//     return oc__font_get_string(font, FC_FAMILY);
// }
//
// const char* oc_get_path(const oc_font* font) {
//     return oc__font_get_string(font, FC_FILE);
// }

// todo: this is not the place to write fontconfig impls
// oc_error oc_discover_fonts(const oc_library* library, const oc_discovery_params* uparams) {
//     oc_error err = oc_error_ok;
//
//     FcConfig* config = NULL;
//     FcPattern* pattern = NULL;
//     FcFontSet* set = NULL;
//
//     FcResult result;
//
//     (void)uparams;
//
//     if (!library) {
//         err = oc_error_invalid_param;
//         goto exit;
//     }
//
//     config = FcInitLoadConfig();
//     if (config == NULL) {
//         err = oc_error_out_of_memory;
//         goto exit;
//     }
//
//     if (!FcConfigBuildFonts(config)) {
//         err = oc_error_unexpected;
//         goto exit;
//     }
//
//     pattern = FcPatternCreate();
//     if (pattern == NULL) {
//         err = oc_error_out_of_memory;
//         goto exit;
//     }
//
//     FcPatternAddInteger(pattern, FC_WEIGHT, FC_WEIGHT_REGULAR);
//     FcPatternAddInteger(pattern, FC_SLANT, FC_SLANT_ROMAN);
//
//     FcConfigSubstitute(config, pattern, FcMatchPattern);
//
//     set = FcFontSort(config, pattern, false, NULL, &result);
//     switch (result) {
//     case FcResultMatch:
//         break;
//     case FcResultOutOfMemory:
//         err = oc_error_out_of_memory;
//         goto exit;
//     default:
//         err = oc_error_unexpected;
//         goto exit;
//     }
//
//     for (int i = 0; i < set->nfont; i++) {
//         FcPattern* font = set->fonts[i];
//         FcChar8* file;
//         FcChar8* family;
//         int dpi = 0;
//
//         FcPatternGetString(font, FC_FILE, 0, &file);
//         FcPatternGetString(font, FC_FAMILY, 0, &family);
//         FcPatternGetInteger(font, FC_DPI, 0, &dpi);
//         printf("%s: %s, %d\n", file, family, dpi);
//     }
// exit:
//     if (set) FcFontSetDestroy(set);
//     if (pattern) FcPatternDestroy(pattern);
//     if (config) FcConfigDestroy(config);
//
//     return err;
// }

static oc_error oc__init_face(FT_Face ft_face, const oc_open_params* params, oc_face* oface) {
    FT_Error err;
    oc_face face;

    err = FT_Set_Char_Size(ft_face, 0, params->desired_size, params->dpi, params->dpi);
    if (err != FT_Err_Ok) {
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


oc_error oc_set_size(oc_face* face, oc_26p6 desired_size, short dpi) {
    FT_Error err;
    FT_Face ft_face;

    if (!face) {
        return oc_error_invalid_param;
    }

    // todo: think if oc_error_invl_pix_size should be returned
    if (desired_size < 1 << 6) {
        return oc_error_invalid_param;
    }

    ft_face = face->impl->ft_face;
    err = FT_Set_Char_Size(ft_face, 0, desired_size, dpi, dpi);

    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Invalid_Pixel_Size:
        return oc_error_invalid_param;
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

oc_error oc_get_sfnt_table(const oc_face* face, oc_tag tag, oc_table* otable, void** ocontext) {
    FT_Error err;
    FT_Face ft_face;
    oc_table table;
    uint8_t* buffer;
    FT_ULong size = 0;

    if (!(face && otable && ocontext)) {
        return oc_error_invalid_param;
    }

    ft_face = face->impl->ft_face;

    // todo: add offset option
    err = FT_Load_Sfnt_Table(ft_face, tag, 0, NULL, &size);
    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Table_Missing:
        return oc_error_table_missing;
    default:
        return oc__unexpected(err);
    }

    buffer = malloc(size);
    if (buffer == NULL) {
        return oc_error_out_of_memory;
    }

    err = FT_Load_Sfnt_Table(ft_face, tag, 0, buffer, &size);
    assert(err == oc_error_ok);

    table.data = buffer;
    table.size = size;

    *otable = table;
    *ocontext = buffer;

    return oc_error_ok;
}

void oc_free_table(const oc_face* face, void* context) {
    (void)face;
    free(context);
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
    FT_Error ft_err = FT_Err_Ok;
    FT_Glyph ft_glyph = NULL;
    oc_error err = oc_error_ok;
    oc_extent extent = { 0 };

    if (!(face && oextent)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    ft_face = face->impl->ft_face;
    lock = &face->impl->lock;

    oc__mutex_impl_lock(lock);
    ft_err = FT_Load_Glyph(ft_face, index, FT_LOAD_BITMAP_METRICS_ONLY | FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT);
    if (ft_err != FT_Err_Ok) {
        oc__mutex_impl_unlock(lock);
        goto exit;
    }

    ft_bitmap = ft_face->glyph->bitmap;
    if (ft_bitmap.width != (FT_UInt)ft_bitmap.pitch) {
        // // todo: implement diffrent types
        err = oc_error_unexpected;
        goto exit_critical;
    }

    extent.rows = ft_bitmap.rows;
    extent.cols = ft_bitmap.width;

    if (buffer == NULL) {
        goto exit_critical;
    }

    if (extent.rows == 0 || extent.cols == 0) {
        goto exit_critical;
    }

    if (buffer_size < extent.rows * extent.cols) {
        err = oc_error_insufficient_buffer;
        goto exit_critical;
    }

    ft_err = FT_Get_Glyph(ft_face->glyph, &ft_glyph);
    oc__mutex_impl_unlock(lock);

    if (ft_err != FT_Err_Ok) {
        goto exit;
    }

    ft_err = FT_Glyph_To_Bitmap(&ft_glyph, FT_RENDER_MODE_NORMAL, NULL, 1);
    if (ft_err != FT_Err_Ok) {
        goto exit;
    }

    assert(((FT_BitmapGlyph)ft_glyph)->bitmap.rows == extent.rows);
    assert(((FT_BitmapGlyph)ft_glyph)->bitmap.width == extent.cols);
    assert((FT_UInt)((FT_BitmapGlyph)ft_glyph)->bitmap.pitch == extent.cols);

    memcpy(buffer, ((FT_BitmapGlyph)ft_glyph)->bitmap.buffer, extent.rows * extent.cols);

    // todo: clean this up
exit:
    if (ft_glyph) FT_Done_Glyph(ft_glyph);
    if (oextent) *oextent = extent;

    if (ft_err != FT_Err_Ok) {
        switch (ft_err) {
        case FT_Err_Out_Of_Memory:
            return oc_error_out_of_memory;
        case FT_Err_Invalid_Argument:
            return oc_error_invalid_param;
        default:
            return oc__unexpected(ft_err);
        }
    }

    return err;
// fragile section!!!
exit_critical:
    oc__mutex_impl_unlock(lock);
    goto exit;
}
