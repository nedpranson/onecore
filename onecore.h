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
    uint16_t ppem; // todo: put this into some oc_scale struct
    oc_16p16 scale; // todo: put this into some oc_scale struct
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
typedef struct oc_font oc_font;

typedef struct {
    void* internals;
    // oc_library_impl* impl;
} oc_library;

typedef struct {
    oc_face_impl* impl;
    oc_font_metrics metrics;
} oc_face;

typedef struct {
    oc_collection_impl* impl;
    oc_font** fonts;
    size_t elements;
    //size_t capacity;
} oc_collection;

typedef struct {
    const void* data;
    size_t size;
} oc_table;

typedef struct {
    const char* family;
    uint8_t weight;
    // flags for bold | italic
} oc_discovery_params;

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

// todo: we need better naming as now we have two seperate project in one lib:
// * discovery
// * loader/parser
OC_PUBLIC const char*
oc_get_family(const oc_font* font);

OC_PUBLIC const char*
oc_get_path(const oc_font* font);

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
oc_set_size(oc_face* face, oc_26p6 desired_size, short dpi);

OC_PUBLIC uint16_t
oc_get_char_index(const oc_face* face, uint32_t charcode);

OC_PUBLIC void
oc_get_glyph_metrics(
    const oc_face* face,
    uint16_t index,
    oc_load_flags flags,
    oc_glyph_metrics* ometrics);

// todo: rename to oc_get_glyph_cbox
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
OC_PUBLIC oc_error
oc_render_glyph(
    const oc_face* face,
    uint16_t index,
    oc_extent* oextent,
    uint8_t* buffer,
    size_t buffer_size);

OC_PUBLIC bool
oc_get_outline(
    const oc_face* face,
    uint16_t index,
    const oc_outline_funcs* funcs,
    void* user);

// todo: copy variant would be nice which we would not need to free
OC_PUBLIC oc_error
oc_get_sfnt_table(
    const oc_face* face,
    oc_tag tag,
    oc_table* otable,
    void** ocontext);

OC_PUBLIC void
oc_free_table(const oc_face* face, void* context);

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

// these defines should work fine
// but should we even allow this?
// if someone would be using them it would just defeat the purpose of this lib
// ONECORE_FORCE_FREETYPE
// ONECORE_FORCE_FONTCONFIG

#ifdef ONECORE_IMPLEMENTATION

#if defined(_MSC_VER) || defined(__MINGW32__)
#define ONECORE_DIRECTWRITE_IMPLEMENTATION
#elif defined(__APPLE__)
#define ONECORE_CORETEXT_IMPLEMENTATION
#else
#define ONECORE_FREETYPE_IMPLEMENTATION
#endif

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

// todo: make default dpi to 72
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

    if (params.dpi <= 0) {
        params.dpi = 72;
    }

    return params;
}

#ifdef ONECORE_FREETYPE_IMPLEMENTATION
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
#endif /* ONECORE_FREETYPE_IMPLEMENTATION */

#ifdef ONECORE_CORETEXT_IMPLEMENTATION
#include <CoreText/CoreText.h>

oc_error oc_init_library(oc_library* olibrary) {
    return olibrary == NULL ? oc_error_invalid_param : oc_error_ok;
}

void oc_free_library(oc_library* library) {
    memset(library, 0, sizeof(*library));
}

static oc_error oc__open_face_from_descriptors(CFArrayRef descriptors, const oc_open_params* uparams, oc_face* oface) {
    oc_face face;

    CTFontRef ct_font;
    CTFontDescriptorRef descriptor;

    CFIndex count = CFArrayGetCount(descriptors);
    oc_open_params params = oc__open_params_defaults(uparams);

    oc_16p16 scaled;
    CGFloat size;

    oc_16p16 ppem;
    uint16_t upem;

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

    scaled = (params.desired_size * params.dpi + 36) / 72;
    ct_font = CTFontCreateWithFontDescriptor(descriptor, scaled / 64.0, NULL);

    if (ct_font == NULL) {
        return oc_error_out_of_memory;
    }

    ppem = (scaled + 32) >> 6;
    if (ppem <= 0 || ppem > UINT16_MAX) {
        // todo: add this test case
        return oc_error_invalid_param;
    }

    size = CTFontGetSize(ct_font);
    upem = CTFontGetUnitsPerEm(ct_font);

    face.impl = (oc_face_impl*)ct_font;
    face.metrics.upem = upem;
    face.metrics.ppem = (uint16_t)ppem;
    face.metrics.scale = oc_div_16p16(scaled, upem);
    face.metrics.ascent = CTFontGetAscent(ct_font) * upem / size;
    face.metrics.descent = CTFontGetDescent(ct_font) * upem / size;
    face.metrics.leading = CTFontGetLeading(ct_font) * upem / size;
    face.metrics.underline_position = CTFontGetUnderlinePosition(ct_font) * upem / size;
    face.metrics.underline_thickness = CTFontGetUnderlineThickness(ct_font) * upem / size;

    *oface = face;
    return oc_error_ok;
}

oc_error oc_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    CFStringRef ct_path;
    CFURLRef url_path;

    CFArrayRef descriptors;
    oc_error err;

    if (!(library && path && oface)) {
        return oc_error_invalid_param;
    }

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
    CFRelease(face->impl);
    memset(face, 0, sizeof(*face));
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

oc_error oc_set_size(oc_face* face, oc_26p6 desired_size, short dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t ppem;

    CTFontRef ct_font;
    CTFontRef ct_font_copy;

    if (!face) {
        return oc_error_invalid_param;
    }

    // todo: think if oc_error_invl_pix_size should be returned
    if (desired_size < 1 << 6 || dpi < 0) {
        return oc_error_invalid_param;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    scaled = (desired_size * dpi + 36) / 72;
    scale = oc_div_16p16(scaled, face->metrics.upem);

    ct_font = (CTFontRef)face->impl;
    ct_font_copy = CTFontCreateCopyWithAttributes(ct_font, scaled / 64.0, NULL, NULL);

    if (ct_font_copy == NULL) {
        return oc_error_out_of_memory;
    }

    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        // todo: add this test case
        CFRelease(ct_font_copy);
        return oc_error_invalid_param;
    }

    face->metrics.ppem = (uint16_t)ppem;
    face->metrics.scale = scale;
    face->impl = (oc_face_impl*)ct_font_copy;

    CFRelease(ct_font);
    return oc_error_ok;
}

oc_error oc_get_sfnt_table(const oc_face* face, oc_tag tag, oc_table* otable, void** ocontext) {
    CTFontRef ct_font;
    CFDataRef ct_data;

    oc_error err = oc_error_ok;
    oc_table table = { 0 };

    if (!(face && otable && ocontext)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    ct_font = (CTFontRef)face->impl;
    ct_data = CTFontCopyTable(ct_font, tag, kCTFontTableOptionNoOptions);

    if (ct_data == NULL) {
        err = oc_error_table_missing; // or oom
        goto exit;
    }

    table.data = CFDataGetBytePtr(ct_data);
    table.size = CFDataGetLength(ct_data);

    *ocontext = (void*)ct_data;
exit:
    if (otable) *otable = table;
    return err;
}

void oc_free_table(const oc_face* face, void* context) {
    (void)face;
    CFRelease(context);
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

    upem = face->metrics.upem;
    size = CTFontGetSize(ct_font);

    metrics.width = rect.size.width * upem / size;
    metrics.height = rect.size.height * upem / size;
    metrics.bearing_x = rect.origin.x * upem / size;
    metrics.bearing_y = (rect.size.height + rect.origin.y) * upem / size;
    metrics.advance = advance.width * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->metrics.scale;

    metrics.width = oc_mul_16p16(metrics.width, scale);
    metrics.height = oc_mul_16p16(metrics.height, scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, scale);
    metrics.advance = oc_mul_16p16(metrics.advance, scale);

exit:
    if (ometrics) *ometrics = metrics;
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

    upem = face->metrics.upem;
    size = CTFontGetSize(ct_font);

    cbox.min_x = CGRectGetMinX(rect) * upem / size;
    cbox.min_y = CGRectGetMinY(rect) * upem / size;
    cbox.max_x = CGRectGetMaxX(rect) * upem / size;
    cbox.max_y = CGRectGetMaxY(rect) * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->metrics.scale;

    cbox.min_x = oc_mul_16p16(cbox.min_x, scale);
    cbox.min_y = oc_mul_16p16(cbox.min_y, scale);
    cbox.max_x = oc_mul_16p16(cbox.max_x, scale);
    cbox.max_y = oc_mul_16p16(cbox.max_y, scale);

exit:
    if (ocbox) *ocbox = cbox;
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

    if (buffer_size < extent.rows * extent.cols) {
        err = oc_error_insufficient_buffer;
        goto exit;
    }

    linear_gray = CGColorSpaceCreateWithName(kCGColorSpaceLinearGray);
    if (linear_gray == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    memset(buffer, 0, extent.rows * extent.cols);

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
    if (oextent) *oextent = extent;
    return err;
}
#endif /* ONECORE_CORETEXT_IMPLEMENTATION */

#ifdef ONECORE_DIRECTWRITE_IMPLEMENTATION
#include <assert.h>
#include <initguid.h>

#include <d2d1.h>
#include <dwrite.h>

#define oc__exit(e) \
    do {            \
        err = (e);  \
        goto exit;  \
    } while (0)

struct oc_face_impl {
    IDWriteFontFace* dw_face;
    IDWriteFactory* dw_factory;
};

typedef struct {
    char* family;
    char* path;
} oc__font__cache;

// struct oc_font {
//     IDWriteFont* dw_font;
//     oc__font__cache cache;
// };

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

oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_collection collection = { 0 };
    oc_error err = oc_error_ok;

    if (!(library && ocollection)) {
        goto exit;
    }

    collection.impl = library->internals;
    collection.fonts = NULL;
    collection.elements = 0;
exit:
    if (ocollection) *ocollection = collection;
    return err;
}

void oc_free_collection(oc_collection* collection) {
    if (collection) {
        memset(collection, 0, sizeof(*collection));
    }
}


// const init_capacity = @as(comptime_int, @max(1, std.atomic.cache_line / @sizeOf(T)));
//
// /// Called when memory growth is necessary. Returns a capacity larger than
// /// minimum that grows super-linearly.
// fn growCapacity(current: usize, minimum: usize) usize {
//     var new = current;
//     while (true) {
//         new +|= new / 2 + init_capacity;
//         if (new >= minimum)
//             return new;
//     }
// }

// // 64 is size of cache line
// static const size_t oc__init_size = 64 / sizeof(oc_font*);
//
// static size_t oc__grow_capacity(size_t current, size_t minimum) {
//     while (current < minimum) {
//         size_t increment = (current >> 1) + oc__init_size;
//         size_t next = current + increment;
//
//         // on overflow every size_t bit will be set to 1
//         size_t mask = -(next < current);
//         current = next | mask;
//     }
//     return current;
// }
//
// // todo: fix some overflow errors
// static bool oc__collection_reserve(oc_collection* collection, size_t elements) {
//     size_t new_capacity = collection->elements + elements;
//     size_t capacity = collection->capacity;
//
//     if (capacity < new_capacity) {
//         oc_font** new_memory;
//         oc_font** old_memory = collection->fonts;
//
//         new_capacity = oc__grow_capacity(capacity, new_capacity);
//         new_memory = malloc(new_capacity * sizeof(oc_font*));
//
//         if (new_memory == NULL) {
//             return false;
//         }
//
//         memcpy(new_memory, old_memory, collection->elements * sizeof(oc_font*));
//
//         collection->fonts = new_memory;
//         collection->capacity = new_capacity;
//
//         free(old_memory);
//     }
//     return true;
// }

// impl needs to have dw_factory and dw_collection
oc_error oc_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;
    HRESULT hr;

    IDWriteFactory* dw_factory;
    IDWriteFontCollection* dw_collection = NULL;

    UINT32 family_count;
    UINT32 index;

    oc_font** fonts = NULL;
    size_t font_count = 0;

    oc_collection collection_copy;

    // on failure collection must stay the same
    // this function should have no side effects on failure!

    if (!collection) {
        oc__exit(oc_error_invalid_param);
    }

    dw_factory = (IDWriteFactory*)collection->impl;
    hr = dw_factory->lpVtbl->GetSystemFontCollection(
        dw_factory,
        &dw_collection,
        TRUE);

    switch (hr) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(hr));
    }

    family_count = dw_collection->lpVtbl->GetFontFamilyCount(dw_collection);
    for (size_t i = 0; i < family_count; i++) {
        IDWriteFontFamily* family;

        hr = dw_collection->lpVtbl->GetFontFamily(dw_collection, i, &family);
        assert(hr == S_OK);

        font_count += family->lpVtbl->GetFontCount(family);
        family->lpVtbl->Release(family);
    }

    fonts = malloc(font_count * sizeof(*fonts));
    if (fonts == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    index = 0;
    for (size_t i = 0; i < family_count; i++) {
        IDWriteFontFamily* family;
        UINT32 font_index;

        hr = dw_collection->lpVtbl->GetFontFamily(dw_collection, i, &family);
        assert(hr == S_OK);

        font_index = family->lpVtbl->GetFontCount(family);
        while (font_index--) {
            IDWriteFont* dw_font;

            hr = family->lpVtbl->GetFont(family, font_index, &dw_font);
            assert(hr == S_OK);

            fonts[index++] = (oc_font*)dw_font;
        }
        // do we need to store family??
        family->lpVtbl->Release(family);
    }

    collection_copy.impl = collection->impl;
    collection_copy.elements = font_count;
    collection_copy.fonts = fonts;

    fonts = collection->fonts;
    font_count = collection->elements;

    *collection = collection_copy;
exit:
    while (font_count--) {
        IDWriteFont* dw_font = (IDWriteFont*)fonts[font_count];
        dw_font->lpVtbl->Release(dw_font);
    }
    free(fonts);
    if (dw_collection) dw_collection->lpVtbl->Release(dw_collection);
    return err;
}


// todo: family_name needs to be shared
// const char* oc_get_family(const oc_font* font) {
//     IDWriteFontFamily* dw_family;
//     IDWriteLocalizedStrings* dw_family_names;
//     HRESULT hr;
//
//     if (!font) {
//         return NULL;
//     }
//
//     if (font->cache.family) {
//         return font->cache.family;
//     }
//
//     hr = font->dw_font->lpVtbl->GetFontFamily(font->dw_font, &dw_family);
//     assert(hr == S_OK);
//
//     hr = dw_family->lpVtbl->GetFamilyNames(dw_family, &dw_family_names);
//
//     assert(hr == S_OK);
//     assert(dw_family_names->lpVtbl->GetCount(dw_family_names) > 0);
//
//
//     UINT32 len;
//     dw_family_names->lpVtbl->GetStringLength(dw_family_names, 0, &len);
//
//     dw_family_names->lpVtbl->Release(dw_family_names);
//     dw_family->lpVtbl->Release(dw_family);
//
//     return NULL;
// }

// const char* oc_get_path(const oc_font* font) {
//     // returns NULL for now
//     return font->cache.path;
// }

static oc_error oc__open_face_from_font_file(IDWriteFactory* dw_factory, IDWriteFontFile* font_file, const oc_open_params* uparams, oc_face* oface) {
    HRESULT err;
    WINBOOL is_supported_fonttype;
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFace* dw_face;
    UINT32 face_num;
    oc_face face;
    DWRITE_FONT_METRICS metrics;
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t ppem;
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

    // todo: we should handle simulations
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

    face.impl = malloc(sizeof(face));
    if (face.impl == NULL) {
        dw_face->lpVtbl->Release(dw_face);
        return oc_error_out_of_memory;
    }

    dw_face->lpVtbl->GetMetrics(dw_face, &metrics);

    // todo: and make that point has to be atleast 1.0

    // https://github.com/freetype/freetype/blob/85c8efe0afa5ad0df35114e317a065f544943c52/include/freetype/internal/ftobjs.h#L665
    scaled = (params.desired_size * params.dpi + 36) / 72;
    scale = oc_div_16p16(scaled, metrics.designUnitsPerEm);

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L3368
    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        // todo: add this test case
        return oc_error_invalid_param;
    }

    face.impl->dw_face = dw_face;
    face.impl->dw_factory = dw_factory;
    face.metrics.upem = metrics.designUnitsPerEm;
    face.metrics.ppem = (uint16_t)ppem;
    face.metrics.scale = scale;
    face.metrics.ascent = metrics.ascent;
    face.metrics.descent = metrics.descent;
    face.metrics.leading = metrics.lineGap;
    face.metrics.underline_position = metrics.underlinePosition;
    face.metrics.underline_thickness = metrics.underlineThickness;

    *oface = face;
    return oc_error_ok;
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

// race!!!!!! to face.metrics->ppem and face->metrics.scale should we allow it?
oc_error oc_set_size(oc_face* face, oc_26p6 desired_size, short dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t ppem;

    if (!face) {
        return oc_error_invalid_param;
    }

    // todo: think if oc_error_invl_pix_size should be returned
    if (desired_size < 1 << 6 || dpi < 0) {
        return oc_error_invalid_param;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    scaled = (desired_size * dpi + 36) / 72;
    scale = oc_div_16p16(scaled, face->metrics.upem);
    ppem = (scaled + 32) >> 6;

    if (ppem > UINT16_MAX) {
        // todo: add this test case
        return oc_error_invalid_param;
    }

    face->metrics.ppem = (uint16_t)ppem;
    face->metrics.scale = scale;

    return oc_error_ok;
}

oc_error oc_get_sfnt_table(const oc_face* face, oc_tag tag, oc_table* otable, void** ocontext) {
    HRESULT dw_err;
    const void* table_data;
    UINT32 table_size;
    void* context;
    WINBOOL exists;
    oc_table table = { 0 };
    oc_error err = oc_error_ok;

    if (!(face && otable && ocontext)) {
        oc__exit(oc_error_invalid_param);
    }

    dw_err = face->impl->dw_face->lpVtbl->TryGetFontTable(
        face->impl->dw_face,
        _byteswap_ulong(tag), // swapping bytes because windows table tags are little-endian
        &table_data,
        &table_size,
        &context,
        &exists);

    switch (dw_err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(err));
    }

    if (!exists) {
        oc__exit(oc_error_table_missing);
    }

    table.data = table_data;
    table.size = table_size;

    *ocontext = context;
exit:
    if (otable) *otable = table;
    return err;
}

void oc_free_table(const oc_face* face, void* context) {
    IDWriteFontFace* dw_face;

    if (!face) return;

    dw_face = face->impl->dw_face;
    dw_face->lpVtbl->ReleaseFontTable(dw_face, context);
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

    scale = face->metrics.scale;

    metrics.width = oc_mul_16p16(metrics.width, scale);
    metrics.height = oc_mul_16p16(metrics.height, scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, scale);
    metrics.advance = oc_mul_16p16(metrics.advance, scale);

    // if (flags & OC_LOAD_NO_HINTING) {
    // goto done;
    //}

    // fit_metrics(&metrics);

exit:
    if (ometrics) *ometrics = metrics;
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

    scale = face->metrics.scale;

    cbox.min_x = oc_mul_16p16(cbox.min_x, scale);
    cbox.min_y = oc_mul_16p16(cbox.min_y, scale);
    cbox.max_x = oc_mul_16p16(cbox.max_x, scale);
    cbox.max_y = oc_mul_16p16(cbox.max_y, scale);

exit:
    if (ocbox) *ocbox = cbox;
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
        face->metrics.upem,
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

    uint8_t* bitmap = NULL;
    oc_extent extent = { 0 };

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

    if (buffer_size < extent.rows * extent.cols) {
        oc__exit(oc_error_insufficient_buffer);
    }

    transform.m11 = 1.0f;
    transform.m12 = 0.0f;
    transform.m21 = 0.0f;
    transform.m22 = 1.0f;
    transform.dx = (cbox.min_x & 63) / 64.0f;
    transform.dy = -(cbox.min_y & 63) / 64.0f;

    glyph_run.fontFace = dw_face;
    glyph_run.fontEmSize = oc_mul_16p16(face->metrics.upem, face->metrics.scale) / 64.0f;
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

    bitmap = malloc(extent.rows * extent.cols * 3);
    if (bitmap == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    bounds.left = 0;
    bounds.bottom = 0;

    bounds.top = -(int32_t)extent.rows;
    bounds.right = extent.cols;

    err = analysis->lpVtbl->CreateAlphaTexture(
        analysis,
        DWRITE_TEXTURE_CLEARTYPE_3x1,
        &bounds,
        bitmap,
        extent.rows * extent.cols * 3);

    if (err != S_OK) {
        oc__exit(oc__unexpected(err));
    }

    for (uint32_t i = 0; i < extent.rows * extent.cols; i++) {
        uint8_t r = bitmap[i * 3 + 0];
        uint8_t g = bitmap[i * 3 + 1];
        uint8_t b = bitmap[i * 3 + 2];

        buffer[i] = (r + b + g) / 3.0f;
    }
exit:
    if (bitmap) free(bitmap);
    if (analysis) analysis->lpVtbl->Release(analysis);
    if (oextent) *oextent = extent;
    return err;
}
#endif /* ONECORE_DIRECTWRITE_IMPLEMENTATION */

#endif /* ONECORE_IMPLEMENTATION */
