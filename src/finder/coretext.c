#include <CoreFoundation/CFBase.h>
#include <CoreText/CTFontTraits.h>
#include <stdint.h>
#define ONECORE_SHARED_IMPLEMENTATION
#include "../onecore.h"

#include <CoreText/CoreText.h>
extern oc_error oc__init_face(CTFontDescriptorRef  descriptor, oc_26p6 desired_size, uint16_t dpi, oc_face* oface);

/* ONECORE_CORETEXT_FINDER_IMPLEMENTATION */
#include <CoreText/CoreText.h>

typedef struct {
    CTFontDescriptorRef ct_font;
    CTFontRef ct_face;
    CFStringRef ct_family;
    oc_font font;
} oc__font_impl;

static inline void oc__free_font_impl(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    CFRelease(impl->ct_family);
    CFRelease(impl->ct_face);
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
    if (ocollection)
        *ocollection = collection;
    return err;
}

void oc_free_collection(oc_collection* collection) {
    if (collection) {
        while (collection->nfonts--) {
            oc__free_font_impl(collection->fonts[collection->nfonts]);
        }

        free(collection->fonts);

        if (collection->impl)
            CFRelease(collection->impl);
        memset(collection, 0, sizeof(*collection));
    }
}

static oc__font_impl* oc__init_font_impl(CTFontDescriptorRef ct_font) {
    oc__font_impl* impl = NULL;

    CFDictionaryRef ct_traits = NULL;
    CFStringRef ct_family;
    CTFontRef ct_face;

    const char* family;

    CFNumberRef symbolic_obj;
    CFNumberRef weight_obj;

    int weight;
    uint32_t ct_symbolic;

    assert(ct_font != NULL);

    weight_obj = CTFontDescriptorCopyAttribute(ct_font, CFSTR("CTFontCSSWeightAttribute"));
    if (weight_obj == NULL) {
        goto exit;
    }

    CFNumberGetValue(weight_obj, kCFNumberIntType, &weight);
    CFRelease(weight_obj);

    ct_traits = CTFontDescriptorCopyAttribute(ct_font, kCTFontTraitsAttribute);
    if (ct_traits == NULL) {
        goto exit;
    }

    symbolic_obj = CFDictionaryGetValue(ct_traits, kCTFontSymbolicTrait);
    assert(symbolic_obj != NULL);

    CFNumberGetValue(symbolic_obj, kCFNumberSInt32Type, &ct_symbolic);

    ct_family = CTFontDescriptorCopyAttribute(ct_font, kCTFontFamilyNameAttribute);
    assert(ct_family != NULL);

    family = CFStringGetCStringPtr(ct_family, kCFStringEncodingUTF8);
    // todo: test is it always utf8 and null terminated
    assert(family != NULL);

    ct_face = CTFontCreateWithFontDescriptor(ct_font, 0.0, NULL);
    if (ct_face == NULL) {
        CFRelease(ct_family);
        goto exit;
    }

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        CFRelease(ct_family);
        CFRelease(ct_face);
        goto exit;
    }

    impl->ct_font = ct_font;
    impl->ct_family = ct_family;
    impl->ct_face = ct_face;
    impl->font.family = family;
    impl->font.weight = (uint16_t)weight;
    impl->font.slant = oc_slant_roman;

    // tood: implement valid one
    //       we need crossplatform solution
    if (ct_symbolic & kCTFontItalicTrait) {
        impl->font.slant = oc_slant_italic;
    }

exit:
    if (ct_traits)
        CFRelease(ct_traits);
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
    while (nfonts--)
        oc__free_font_impl(fonts[nfonts]);
    free(fonts);
    if (ct_fonts)
        CFRelease(ct_fonts);

    return err;
}

bool ocf_has_character(const oc_font* font, uint32_t charcode) {
    oc__font_impl* impl;
    CTFontRef ct_face;

    CGGlyph glyphs[2];
    UniChar chars[2];

    if (!font || charcode > 0x10FFFF) {
        return false;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    ct_face = impl->ct_face;

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
        ct_face,
        chars,
        glyphs,
        glyphs[1]);

    return glyphs[0];
}


// todo: ensure that every backends set oface to 0 on failure
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);

    if (desired_size == 0) {
        desired_size = 12 << 6;
    } else if (desired_size < 1 << 6) {
        desired_size = 1 << 6;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    return oc__init_face(impl->ct_font, desired_size, dpi, oface);
}

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    CFURLRef url;

    CFStringRef path;
    CFIndex path_len;

    size_t copy_len;

    if (!font) {
        return 0;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    url = CTFontDescriptorCopyAttribute(impl->ct_font, kCTFontURLAttribute);

    // todo: check if this is a shared object
    if (url == NULL) {
        return 0;
    }

    path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
    CFRelease(url);

    if (path == NULL) {
        return 0;
    }

    // todo: check if path is already utf8
    path_len = CFStringGetBytes(
        path,
        CFRangeMake(0, CFStringGetLength(path)),
        kCFStringEncodingUTF8,
        0,
        false,
        NULL,
        0,
        NULL);

    copy_len = len < (size_t)path_len ? len : (size_t)path_len;
    if (copy_len == 0) {
        CFRelease(path);
        return (size_t)path_len;
    }

    CFStringGetBytes(
        path,
        CFRangeMake(0, CFStringGetLength(path)),
        kCFStringEncodingUTF8,
        0,
        false,
        (UInt8*)buf,
        copy_len,
        NULL);

    CFRelease(path);
    return copy_len;
}
