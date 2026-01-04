#include "shared.h"
#ifdef ONECORE_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H

oc_error oc_init_library(oc_library* plibrary) {
    if (plibrary == NULL) {
        return oc_error_invalid_param;
    }

    FT_Error err = FT_Init_FreeType((FT_Library*)&plibrary->handle);
    switch (err) {
    case FT_Err_Ok:
        return oc_error_ok;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }
}

void oc_free_library(oc_library library) {
    FT_Done_FreeType(library.handle);
}

oc_error oc_open_face(oc_library library, const char* path, long face_index, oc_face* pface) {
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

    // using FT_Open_Face as FT_New_Face fails if file extention does not match file type
    err = FT_Open_Face(library.handle, &open_args, face_index, &face);
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

    err = FT_Set_Char_Size(face, 0, 12 * 64.0, ONECORE_DEFAULT_DPI, ONECORE_DEFAULT_DPI);
    if (err != FT_Err_Ok) {
        FT_Done_Face(face);
        return unexpected(err);
    }

    pface->handle = face;
    return oc_error_ok;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, long face_index, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    FT_Face face;
    FT_Error err;

    err = FT_New_Memory_Face(library.handle, data, size, face_index, &face);
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

    err = FT_Set_Char_Size(face, 0, 12 * 64, ONECORE_DEFAULT_DPI, ONECORE_DEFAULT_DPI);
    if (err != FT_Err_Ok) {
        FT_Done_Face(face);
        return unexpected(err);
    }

    pface->handle = face;
    return oc_error_ok;
}

void oc_free_face(oc_face face) {
    FT_Done_Face(face.handle);
}

inline uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    return FT_Get_Char_Index(face.handle, charcode);
}

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable) {
    oc_table table;
    FT_Error err;

    if (ptable == NULL) {
        return oc_error_invalid_param;
    }

    // if other abis allow we can add offset option
    table.size = 0;
    err = FT_Load_Sfnt_Table(face.handle, tag, 0, NULL, &table.size);
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

    err = FT_Load_Sfnt_Table(face.handle, tag, 0, buffer, &table.size);
    assert(err == oc_error_ok);

    table.data = buffer;
    table.__handle = buffer;

    *ptable = table;

    return oc_error_ok;
}

inline void oc_free_table(oc_face face, oc_table table) {
    (void)face;
    free(table.__handle);
}

void oc_get_metrics(oc_face face, oc_metrics* pmetrics) {
    FT_Face ft_face = face.handle;
    pmetrics->units_per_em = ft_face->units_per_EM;
    pmetrics->ascent = ft_face->ascender;
    pmetrics->descent = -ft_face->descender;
    pmetrics->leading = ft_face->height - ft_face->ascender + ft_face->descender;
    // reverting ajusted underline position by freetype
    pmetrics->underline_position = ft_face->underline_position + (ft_face->underline_thickness >> 1);
    pmetrics->underline_thickness = ft_face->underline_thickness;
}

// todo: add option for verticals and maybe load both hori and vert bearings, advances
bool oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_glyph_metrics* pglyph_metrics) {
    if (pglyph_metrics == NULL) {
        return false;
    }

    // start: not thread safe here!!!!
    FT_Face ft_face = face.handle;
    FT_Error err = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_BITMAP_METRICS_ONLY);
    if (err != FT_Err_Ok) {
        return false;
    }

    FT_GlyphSlot slot = ft_face->glyph;
    FT_Glyph_Metrics glyph_metrics = slot->metrics;
    // end: not thread safe here!!!!

    pglyph_metrics->width = glyph_metrics.width;
    pglyph_metrics->height = glyph_metrics.height;
    pglyph_metrics->bearing_x = glyph_metrics.horiBearingX;
    pglyph_metrics->bearing_y = glyph_metrics.horiBearingY;
    pglyph_metrics->advance = glyph_metrics.horiAdvance;

    return true;
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

    // start: not thread safe here!!!!
    FT_Face ft_face = face.handle;
    err = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);
    if (err != FT_Err_Ok) {
        return false;
    }

    FT_GlyphSlot slot = ft_face->glyph;
    FT_Outline glyph_outline = slot->outline;
    // todo: check if glyph format has outline bla bla bla
    // end: not thread safe here!!!!

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

#endif // ONECORE_FREETYPE
