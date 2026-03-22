#define ONECORE_IMPLEMENTATION
#include "../onecore.h"

/* ONECORE_CORETEXT_FINDER_IMPLEMENTATION */
#include <CoreText/CoreText.h>

typedef struct {
    CTFontDescriptorRef ct_font;
    CFStringRef ct_family;
    oc_font font;
} oc__font_impl;

static inline void oc__free_font_impl(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    CFRelease(impl->ct_family);
    free(impl);
}

oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error err = oc_error_ok;
    oc_collection collection = { 0 };

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }

exit:
    if (ocollection) *ocollection = collection;
    return err;
}

void oc_free_collection(oc_collection* collection) {
    if (collection) {
        while (collection->nfonts--) {
            oc__free_font_impl(collection->fonts[collection->nfonts]);
        }

        free(collection->fonts);

        if (collection->impl) CFRelease(collection->impl);
        memset(collection, 0, sizeof(*collection));
    }
}

static const struct {
  int ot;
  double ct;
} oc__weight_map[] = {
    {   0, -0.8 },
    { 100, -0.8 },
    { 200, -0.6 },
    { 300, -0.4 },
    // todo: check what these are equal to
    // { 350, FC_WEIGHT_DEMILIGHT },
    // { 380, FC_WEIGHT_BOOK },
    { 400, 0.0 },
    { 500, 0.23 },
    { 600, 0.3 },
    { 700, 0.4 },
    { 800, 0.56 },
    { 900, 0.62 },
    {1000, 1.0 }, // idk if this correct
};

// todo: fix these lerp functioms wtf is this
static double oc__lerp(double x, double x1, double x2, int y1, int y2) {
    double dx = x2 - x1;
    int dy = y2 - y1;
    assert (dx > 0 && dy >= 0 && x1 <= x && x <= x2);
    return y1 + (dy*(x-x1) + dx / 2.0) / dx;
}

double oc__convert(double ct_weight) {
    int i;

    for (i = 1; ct_weight > oc__weight_map[i].ct; i++);

    if (ct_weight == oc__weight_map[i].ct) {
        return oc__weight_map[i].ot;
    }

    return oc__lerp(
        ct_weight,
        oc__weight_map[i-1].ct,
        oc__weight_map[i].ct,
        oc__weight_map[i-1].ot,
        oc__weight_map[i].ot);
}

static oc__font_impl* oc__init_font_impl(CTFontDescriptorRef ct_font) {
    oc__font_impl* impl = NULL;

    CFDictionaryRef ct_traits;
    CFStringRef ct_family;

    const char* family;

    CFNumberRef weight_obj;
    double ct_weight;

    assert(ct_font != NULL);

    ct_traits = CTFontDescriptorCopyAttribute(ct_font, kCTFontTraitsAttribute);
    if (ct_traits == NULL) {
        goto exit;
    }

    weight_obj = CFDictionaryGetValue(ct_traits, kCTFontWeightTrait);
    assert(weight_obj != NULL);

    CFNumberGetValue(weight_obj, kCFNumberDoubleType, &ct_weight);
    CFRelease(ct_traits);

    ct_family = CTFontDescriptorCopyAttribute(ct_font, kCTFontFamilyNameAttribute);
    assert(ct_family != NULL);

    family = CFStringGetCStringPtr(ct_family, kCFStringEncodingUTF8);
    // todo: test is it always utf8 and null terminated
    assert(family != NULL);

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        CFRelease(ct_family);
        goto exit;
    }

    impl->ct_font = ct_font;
    impl->ct_family = ct_family;
    impl->font.family = family;
    impl->font.weight = oc__convert(ct_weight) + 0.5;

exit:
    return impl;
}

oc_error oc_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;

    CTFontCollectionRef ct_collection;
    CFArrayRef ct_fonts;

    CFIndex font_count;

    oc_font** fonts = NULL;
    uint32_t nfonts = 0;
    
    oc_collection tmp_collection;

    if (!collection) {
        err = oc_error_invalid_param;
        goto exit;
    }

    ct_collection = CTFontCollectionCreateFromAvailableFonts(NULL);
    if (ct_collection == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    ct_fonts = CTFontCollectionCreateMatchingFontDescriptors(ct_collection);
    CFRelease(ct_collection);

    if (ct_fonts == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    font_count = CFArrayGetCount(ct_fonts);
    if (font_count == 0) {
        goto done;
    }

    fonts = malloc(font_count * sizeof(*fonts));
    if (fonts == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    for (CFIndex i = 0; i < font_count; i++) {
        CTFontDescriptorRef ct_font = CFArrayGetValueAtIndex(ct_fonts, i);
        oc__font_impl* impl = oc__init_font_impl(ct_font);
        if (impl == NULL) {
            err = oc_error_out_of_memory;
            goto exit;
        }

        fonts[nfonts++] = &impl->font;
    }
done:
    tmp_collection.impl = (oc_collection_impl*)ct_fonts;
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

    ct_fonts = (CFArrayRef)collection->impl;
    fonts = collection->fonts;
    nfonts = collection->nfonts;

    *collection = tmp_collection;
exit:
    while (nfonts--) oc__free_font_impl(fonts[nfonts]);
    free(fonts);
    if (ct_fonts) CFRelease(ct_fonts);

    return err;
}
