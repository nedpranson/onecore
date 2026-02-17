#include "shared.h"
#ifdef ONECORE_FREETYPE

#include <ft2build.h>
#include <pthread.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

struct face_internals {
    FT_Face face;
    mutex_t lock;
};

#define FT(x) _Generic((x),                  \
    oc_library: ((FT_Library)(x).internals), \
    oc_face: ((struct face_internals*)(x).internals)->face)

#define FACE_LOCK(x) mutex_lock(&((struct face_internals*)x.internals)->lock)
#define FACE_UNLOCK(x) mutex_unlock(&((struct face_internals*)x.internals)->lock)

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
        return unexpected(err);
    }

    plibrary->internals = library;
    return oc_error_ok;
}

inline void oc_free_library(oc_library library) {
    FT_Done_FreeType(FT(library));
}

static oc_error init_face(FT_Face ft_face, const oc_face_params* pparams, oc_face* pface) {
    FT_Error err = FT_Set_Char_Size(ft_face, 0, pparams->desired_size * 64.0f, pparams->dpi, pparams->dpi);
    if (err != FT_Err_Ok) {
        return unexpected(err);
    }

    struct face_internals* internals = malloc(sizeof(struct face_internals));
    if (internals == NULL) {
        return oc_error_out_of_memory;
    }

    // todo: think
    // as dwrite and freetype needs say 2 args
    // we can add void* oc_face::reserved

    internals->face = ft_face;
    mutex_init(&internals->lock);

    printf("y_ppem: %d, x_ppem: %d\n", ft_face->size->metrics.y_ppem, ft_face->size->metrics.x_ppem);

    pface->internals = internals;
    pface->metrics.ppem = ft_face->size->metrics.y_ppem;
    pface->metrics.upem = ft_face->units_per_EM;
    pface->metrics.ascent = ft_face->ascender;
    pface->metrics.descent = -ft_face->descender;
    pface->metrics.leading = ft_face->height - ft_face->ascender + ft_face->descender;
    // reverting ajusted underline position by freetype
    pface->metrics.underline_position = ft_face->underline_position + (ft_face->underline_thickness >> 1);
    pface->metrics.underline_thickness = ft_face->underline_thickness;

    return oc_error_ok;
}

oc_error oc_open_face(oc_library library, const char* path, const oc_face_params* pparams, oc_face* pface) {
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

    oc_face_params params = fill_face_params(pparams);

    // using FT_Open_Face as FT_New_Face fails if file extention does not match file type
    err = FT_Open_Face(FT(library), &open_args, params.face_index, &face);
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
        return unexpected(err);
    }

    oc_error oc_err = init_face(face, &params, pface);
    if (oc_err != oc_error_ok) {
        FT_Done_Face(face);
    }

    return oc_err;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, const oc_face_params* pparams, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    FT_Face face;
    FT_Error err;

    oc_face_params params = fill_face_params(pparams);
    err = FT_New_Memory_Face(FT(library), data, size, params.face_index, &face);
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
        return unexpected(err);
    }

    oc_error oc_err = init_face(face, &params, pface);
    if (oc_err != oc_error_ok) {
        FT_Done_Face(face);
    }

    return oc_err;
}

void oc_free_face(oc_face face) {
    FT_Done_Face(FT(face));

    mutex_destroy(&((struct face_internals*)face.internals)->lock);
    free(face.internals);
}

inline uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    return FT_Get_Char_Index(FT(face), charcode);
}

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable, void** pcontext) {
    oc_table table;
    FT_Error err;

    if (ptable == NULL || pcontext == NULL) {
        return oc_error_invalid_param;
    }

    // if other abis allow we can add offset option
    table.size = 0;
    err = FT_Load_Sfnt_Table(FT(face), tag, 0, NULL, &table.size);
    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Table_Missing:
        return oc_error_table_missing;
    default:
        return unexpected(err);
    }

    uint8_t* buffer = malloc(table.size);
    if (buffer == NULL) {
        return oc_error_out_of_memory;
    }

    err = FT_Load_Sfnt_Table(FT(face), tag, 0, buffer, &table.size);
    assert(err == oc_error_ok);

    table.data = buffer;

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

    // disable hinting bla bla bla!
    FT_Int32 ft_load_flags = FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT | FT_LOAD_BITMAP_METRICS_ONLY;
    if (flags & OC_LOAD_NO_SCALE) {
        ft_load_flags |= FT_LOAD_NO_SCALE;
    }

    FACE_LOCK(face);
    FT_Error err = FT_Load_Glyph(FT(face), glyph_index, ft_load_flags);
    if (err != FT_Err_Ok) {
        FACE_UNLOCK(face);
        memset(pmetrics, 0, sizeof(oc_glyph_metrics));
        return;
    }

    FT_GlyphSlot slot = FT(face)->glyph;
    FT_Glyph_Metrics glyph_metrics = slot->metrics;
    FACE_UNLOCK(face);

    uint8_t shift = (flags & OC_LOAD_NO_SCALE) ? 0 : 6;

    if (face.metrics.ppem == 21) {
        printf("float: %f\n", glyph_metrics.horiAdvance / 64.0f);
    }

    pmetrics->width = glyph_metrics.width >> shift;
    pmetrics->height = glyph_metrics.height >> shift;
    pmetrics->bearing_x = glyph_metrics.horiBearingX >> shift;
    pmetrics->bearing_y = glyph_metrics.horiBearingY >> shift;
    pmetrics->advance = glyph_metrics.horiAdvance >> shift;
}

typedef struct outline_context {
    const oc_outline_funcs* funcs;
    void* ctx;

    FT_Vector x2origin;
    bool figure_started;
} outline_context;

static int move_to(const FT_Vector* to, void* user) {
    outline_context* ctx = (outline_context*)user;
    oc_point point = { to->x >> 1, to->y >> 1 };

    if (ctx->figure_started) {
        ctx->funcs->end_figure(ctx->ctx);
    }

    ctx->funcs->start_figure(point, ctx->ctx);
    ctx->x2origin = *to;
    ctx->figure_started = true;

    return 0;
}

static int line_to(const FT_Vector* x2to, void* user) {
    outline_context* ctx = (outline_context*)user;
    oc_point point = { x2to->x >> 1, x2to->y >> 1 };

    // need to cancel line to somehow

    // printf("curr: (%ld %ld)\n", ctx->x2origin.x >> 1, ctx->x2origin.y >> 1);

    ctx->funcs->line_to(point, ctx->ctx);
    ctx->x2origin = *x2to;

    return 0;
}

typedef struct point_2f {
    float x;
    float y;
} point_2f;

static int conic_to(const FT_Vector* x2control, const FT_Vector* x2to, void* user) {
    outline_context* ctx = (outline_context*)user;

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
        { cubic[0].x, cubic[0].y },
        { cubic[1].x, cubic[1].y },
        { x2to->x >> 1, x2to->y >> 1 }
    };

    ctx->funcs->cubic_to(points[0], points[1], points[2], ctx->ctx);
    ctx->x2origin = *x2to;

    return 0;
}

static int cubic_to(const FT_Vector* x2c1, const FT_Vector* x2c2, const FT_Vector* x2to, void* user) {
    outline_context* ctx = (outline_context*)user;

    oc_point points[3] = {
        { x2c1->x >> 1, x2c1->y >> 1 },
        { x2c2->x >> 1, x2c2->y >> 1 },
        { x2to->x >> 1, x2to->y >> 1 }
    };

    ctx->funcs->cubic_to(points[0], points[1], points[2], ctx->ctx);
    ctx->x2origin = *x2to;

    return 0;
}

bool oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    FT_Error err;
    if (outline_funcs == NULL) {
        return false;
    }

    FACE_LOCK(face);

    err = FT_Load_Glyph(FT(face), glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);
    if (err != FT_Err_Ok) {
        FACE_UNLOCK(face);
        return false;
    }

    FT_GlyphSlot slot = FT(face)->glyph;
    FT_Outline glyph_outline = slot->outline;

    if (slot->format != FT_GLYPH_FORMAT_OUTLINE && slot->format != FT_GLYPH_FORMAT_COMPOSITE) {
        FACE_UNLOCK(face);
        return false;
    }

    FACE_UNLOCK(face);

    outline_context ctx = { 0 };
    ctx.funcs = outline_funcs;
    ctx.ctx = context;

    // shift is set to one as we want all point to be multiplied by 2
    // to restore conic 'to' position to its original floating point value
    static const FT_Outline_Funcs decompose_funcs = {
        move_to,
        line_to,
        conic_to,
        cubic_to,
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

oc_error oc_render_glyph(oc_face face, uint16_t glyph_index, oc_bbox* pbbox, unsigned char* buffer, size_t buffer_size) {
    FT_Error err;
    if (pbbox == NULL) {
        return oc_error_invalid_param;
    }

    FACE_LOCK(face);

    err = FT_Load_Glyph(FT(face), glyph_index, FT_LOAD_BITMAP_METRICS_ONLY);
    if (err != FT_Err_Ok) {
        FACE_UNLOCK(face);
        switch (err) {
        case FT_Err_Out_Of_Memory:
            return oc_error_out_of_memory;
        case FT_Err_Invalid_Argument:
            return oc_error_invalid_param;
        default:
            return unexpected(err);
        }
    }

    FT_Bitmap bitmap = FT(face)->glyph->bitmap;
    if ((int)bitmap.width != bitmap.pitch) {
        FACE_UNLOCK(face);
        // todo: implement diffrent types
        return oc_error_unexpected;
    }

    pbbox->rows = bitmap.rows;
    pbbox->cols = bitmap.width;

    if (buffer == NULL) {
        FACE_UNLOCK(face);
        return oc_error_ok;
    }

    if (bitmap.rows == 0 || bitmap.width == 0) {
        FACE_UNLOCK(face);
        return oc_error_ok;
    }

    if (buffer_size < bitmap.rows * bitmap.width) {
        FACE_UNLOCK(face);
        return oc_error_insufficient_buffer;
    }

    FT_Glyph glyph;
    FT_BitmapGlyph glyph_bitmap;

    err = FT_Get_Glyph(FT(face)->glyph, &glyph);
    FACE_UNLOCK(face);

    switch (err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    err = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1);
    glyph_bitmap = (FT_BitmapGlyph)glyph;

    if (err != FT_Err_Ok) {
        FT_Done_Glyph(glyph);
        switch (err) {
        case FT_Err_Out_Of_Memory:
            return oc_error_out_of_memory;
        default:
            return unexpected(err);
        }
    }

    assert(glyph_bitmap->bitmap.rows == bitmap.rows);
    assert(glyph_bitmap->bitmap.width == bitmap.width);
    assert(glyph_bitmap->bitmap.pitch == bitmap.pitch);

    memcpy(buffer, glyph_bitmap->bitmap.buffer, bitmap.rows * bitmap.width);

    FT_Done_Glyph(glyph);
    return oc_error_ok;
}

#endif // ONECORE_FREETYPE
