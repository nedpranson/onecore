#include "_string.h"
#define ONECORE_SHARED_IMPLEMENTATION
#include "../onecore.h"

/* ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION */
#include <assert.h>
#include <fontconfig/fontconfig.h>
#include <stdlib.h>
#include <string.h>

struct oc_collection_impl {
    const oc_library* oc_library;
    FcConfig*         fc_config;
};

typedef struct {
    const oc_library* oc_library;
    FcPattern*        fc_pattern;
    oc_font           font;
} oc__font_impl;

typedef enum {
    oc__status_ok,
    oc__status_memory,
    oc__status_skip,
} oc__status;

static inline void oc__free_font(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    // impl->fc_pattern should not be freed as it is owned by FcConfig
    free(impl);
}

oc_error ocf_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error            err = oc_error_ok;
    FcConfig*           fc_config;
    oc_collection_impl* impl;
    oc_collection       collection = { 0 };

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

void ocf_free_collection(oc_collection* collection) {
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

oc_error ocf_load_fonts(oc_collection* collection) {
    oc_error          err = oc_error_ok;
    const oc_library* oc_library;

    FcConfig*  fc_config;
    FcFontSet* fc_fonts;

    int font_count;

    oc_font** fonts = NULL;
    uint32_t  nfonts = 0;

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
        oc_font*   font;

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
    FcCharSet*     charset;
    FcResult       result;

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
    FcResult   result;

    FcChar8* file;
    int      index;

    oc_open_params params;

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);
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

    return ocl_open_face(impl->oc_library, (char*)file, &params, oface);
}

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    FcResult       result;

    FcChar8* file;
    size_t   file_len;

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
