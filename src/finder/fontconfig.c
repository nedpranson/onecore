#define ONECORE_SHARED_IMPLEMENTATION
#include "../onecore.h"

/* ONECORE_FONTCONFIG_FINDER_IMPLEMENTATION */
#include <fontconfig/fontconfig.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    FcPattern* fc_pattern;
    oc_font font;
} oc__font_impl;

typedef enum {
    oc__status_ok,
    oc__status_memory,
    oc__status_skip,
} oc__status;

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
        // todo: test what fontconfig does when cache is corrupted
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
done:
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
