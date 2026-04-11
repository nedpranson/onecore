#define ONECORE_IMPLEMENTATION
#define ONECORE_FREETYPE_LOADER_IMPLEMENTATION
#include "onecore.h"

extern oc_error oc__init_face(CTFontDescriptorRef descriptor, oc_26p6 desired_size, uint16_t dpi, oc_face* oface);

/* ONECORE_CORETEXT_FINDER_IMPLEMENTATION */
#import <CoreText/CoreText.h>

typedef struct {
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    const oc_library*         oc_library;
#endif
    CTFontDescriptorRef ct_font;
    CTFontRef           ct_face;
    CFStringRef         ct_family;
    oc_font             font;
} oc__font_impl;

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
struct oc_collection_impl {
    const oc_library* oc_library;
    CFArrayRef ct_fonts;
};
#endif

static inline void oc__free_font_impl(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    CFRelease(impl->ct_family);
    CFRelease(impl->ct_face);
    free(impl);
}

oc_error ocf_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_error      err = oc_error_ok;
    oc_collection collection = { 0 };

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    collection.impl = calloc(1, sizeof(oc_collection_impl));
    if (!collection.impl) {
        err = oc_error_out_of_memory;
        goto exit;
    }
    collection.impl->oc_library = library;
#endif
exit:
    if (ocollection)
        *ocollection = collection;
    return err;
}

void ocf_free_collection(oc_collection* collection) {
    if (collection) {
        while (collection->nfonts--) {
            oc__free_font_impl(collection->fonts[collection->nfonts]);
        }

        free(collection->fonts);

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
        if (collection->impl->ct_fonts) {
            CFRelease(collection->impl->ct_fonts);
        }
        free(collection->impl);
#else
        if (collection->impl)
            CFRelease(collection->impl);
#endif
        memset(collection, 0, sizeof(*collection));
    }
}

static oc__font_impl* oc__init_font_impl(const oc_library* oc_library, CTFontDescriptorRef ct_font) {
    oc__font_impl* impl = NULL;

    CFDictionaryRef ct_traits = NULL;
    CFStringRef     ct_family;
    CTFontRef       ct_face;

    const char* family;

    CFNumberRef symbolic_obj;
    CFNumberRef weight_obj;

    int      weight;
    uint32_t ct_symbolic;

    assert(ct_font != NULL);
    (void)oc_library;

    // todo (stage 2): do some assumptions based on this assumption
    // is_immortal = CFGetRetainCount(obj) == 0x7FFFFFFFFFFFFFFF

    // Cheers to AI it has found private api to 'CTFontCSSWeightAttribute'
    weight_obj = CTFontDescriptorCopyAttribute(ct_font, CFSTR("CTFontCSSWeightAttribute"));
    // Notify developer on GitHub if this assertion ever fails:
    // https://github.com/nedpranson/onecore/issues
    assert(weight_obj != NULL);

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

    // family seems to always be utf8 and null terminated
    family = CFStringGetCStringPtr(ct_family, kCFStringEncodingUTF8);
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

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    impl->oc_library = oc_library;
#endif
    impl->ct_font = ct_font;
    impl->ct_family = ct_family;
    impl->ct_face = ct_face;
    impl->font.family = family;
    impl->font.weight = (uint16_t)weight;
    impl->font.slant = oc_slant_roman;

    // todo (stage 2): implement valid one
    // we need crossplatform solution
    if (ct_symbolic & kCTFontItalicTrait) {
        impl->font.slant = oc_slant_italic;
    }

exit:
    if (ct_traits)
        CFRelease(ct_traits);
    return impl;
}

// todo: clean!
oc_error ocf_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;

    CTFontCollectionRef ct_collection;
    CFArrayRef          ct_fonts;

    CFIndex font_count;

    oc_font** fonts = NULL;
    uint32_t  nfonts = 0;

    oc_collection tmp_collection;
    const oc_library* oc_library = NULL;

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    CFArrayRef ct_fonts2;
#endif

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

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    oc_library = collection->impl->oc_library;
#endif

    for (CFIndex i = 0; i < font_count; i++) {
        CTFontDescriptorRef ct_font = CFArrayGetValueAtIndex(ct_fonts, i);
        oc__font_impl*      impl = oc__init_font_impl(oc_library, ct_font);
        if (impl == NULL) {
            err = oc_error_out_of_memory;
            goto exit;
        }

        fonts[nfonts++] = &impl->font;
    }
done:
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    ct_fonts2 = ct_fonts;
#else
    tmp_collection.impl = (oc_collection_impl*)ct_fonts;
#endif
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    ct_fonts = collection->impl->ct_fonts;
    collection->impl->ct_fonts = ct_fonts2;
#else
    ct_fonts = (CFArrayRef)collection->impl;
#endif
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
    CTFontRef      ct_face;

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

#ifdef ONECORE_CORETEXT_LOADER_IMPLEMENTATION
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;
    oc_error       err;
    oc_face        face = { 0 };

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

    err = oc__init_face(impl->ct_font, desired_size, dpi, &face);
    *oface = face;

    return err;
}
#else
// todo (stage 2): handle memory only fonts!
// when we will implement correct reconstruction function
// we could use FT_StreamRec to stream data and emulate it
// just build these ocf__offset_table ocf__table_record
// give to freetype and forget
// any other data we track on which tag we are and just give it back
// typedef struct {
//     int32_t  sfnt_version;
//     uint16_t num_tables;
//     uint16_t search_range;
//     uint16_t entry_selector;
//     uint16_t range_shift;
// } ocf__offset_table;
//
// typedef struct {
//     uint32_t tag;
//     uint32_t checksum;
//     uint32_t offset;
//     uint32_t length;
// } ocf__table_record;
//
//
// typedef struct {
//     size_t size;
//     void*  data;
// } ocf__memory_view;
//
// static uint32_t ocf__checksum(const uint32_t* table, uint32_t padded_size) {
//     uint32_t sum = 0;
//     padded_size >>= 2;
//
//     while (padded_size--) {
//         sum += CFSwapInt32HostToBig(*table++);
//     }
//     return sum;
// }
//
// // https://gist.github.com/netmaid/dd524da6a8b893c3f9fdf8cd52d1816b
// // todo (stage 2): test endian
// // this impl is slow did not test why, but my guess is calculating checksum without simd is expensive
// // and calloc is expensive as we need to zero out memory faster would be to just set those padded pixels to 0 when needed
// static ocf__memory_view ocf__extract_font_data(CGFontRef cg_font) {
//     CFArrayRef tags;
//     CFIndex ntables;
//
//     size_t size;
//     void*  data;
//
//     ocf__memory_view view = { 0 };
//
//     bool cff = false;
//
//     assert(cg_font != NULL);
//
//     tags = CGFontCopyTableTags(cg_font);
//     if (!tags) {
//         return view;
//     }
//
//     ntables = CFArrayGetCount(tags);
//     size = sizeof(ocf__offset_table) + sizeof(ocf__table_record) * ntables;
//
//     assert(UINT16_MAX > ntables);
//
//     for (CFIndex i = 0; i < ntables; i++) {
//         uint32_t tag;
//         CFDataRef table;
//
//         tag = (uint32_t)(uintptr_t)CFArrayGetValueAtIndex(tags, i);
//         if (tag == 'CFF ') {
//             cff = true;
//         }
//
//         table = CGFontCopyTableForTag(cg_font, tag);
//         assert(table != NULL);
//
//         size += (CFDataGetLength(table) + 3) & ~3;
//         CFRelease(table);
//     }
//
//     data = calloc(1, size);
//     if (data) {
//         uint16_t search_range = 1;
//         uint16_t entry_selector = 0;
//
//         ocf__offset_table* table = data;
//         ocf__table_record* records = data + sizeof(ocf__offset_table);
//
//         void* offset = data + sizeof(ocf__offset_table) + sizeof(ocf__table_record) * ntables;
//
//         while (search_range * 2 <= ntables) {
//             search_range *= 2;
//             entry_selector++;
//         }
//
//         table->sfnt_version = cff ? 'OTTO' : CFSwapInt32HostToBig(0x10000);
//         table->num_tables = CFSwapInt16HostToBig((uint16_t)ntables);
//         table->search_range = CFSwapInt16HostToBig(search_range * 16);
//         table->entry_selector = CFSwapInt16HostToBig(entry_selector);
//
//         if (search_range * 16 >= 256) {
//             table->range_shift = CFSwapInt16HostToBig(ntables * 16 - search_range);
//         } else {
//             table->range_shift = CFSwapInt16HostToBig(ntables * 16 - search_range * 16);
//         }
//
//         for (CFIndex i = 0; i < ntables; i++) {
//             uint32_t tag = (uint32_t)(uintptr_t)CFArrayGetValueAtIndex(tags, i);
//
//             CFDataRef table = CGFontCopyTableForTag(cg_font, tag);
//             CFIndex   table_size;
//
//             uint32_t padded_size;
//
//             assert(table != NULL);
//
//             table_size = CFDataGetLength(table);
//             padded_size = (uint32_t)((table_size + 3) & ~3);
//
//             memcpy(offset, CFDataGetBytePtr(table), table_size); // rly slow!
//
//             records[i].tag = CFSwapInt32HostToBig(tag);
//             records[i].checksum = ocf__checksum((uint32_t*)offset, padded_size);
//             records[i].offset = CFSwapInt32HostToBig((uint32_t)(uintptr_t)(offset - data));
//             records[i].length = (uint32_t)table_size;
//
//             offset += padded_size;
//             CFRelease(table);
//         }
//     }
//
//     view.size = size;
//     view.data = data;
//
//     CFRelease(tags);
//     return view;
// }

static uint32_t ocf__font_index(CTFontRef font) {
    CFNumberRef n = CTFontCopyAttribute(font, CFSTR("NSCTFontIndexAttribute"));
    if (!n) {
        return 0;
    }

    long idx;

    CFNumberGetValue(n, kCFNumberNSIntegerType, &idx);
    CFRelease(n);

    return (uint32_t)idx;
}

// todo: zero init face on failure
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;
    CFURLRef       url;

    CFStringRef path;

    uint32_t index;
    char buf[256];

    oc_open_params params;

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    index = ocf__font_index(impl->ct_face);

    url = CTFontDescriptorCopyAttribute(impl->ct_font, kCTFontURLAttribute);
    if (url == NULL) {
        return oc__unexpected(1);
    }

    path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
    CFRelease(url);

    if (path == NULL) {
        return oc_error_out_of_memory;
    }

    CFStringGetCString(path, buf, sizeof(buf), kCFStringEncodingUTF8);
    CFRelease(path);

    // todo: freetype indexes have more depth bla bla bla!
    params.face_index = (uint32_t)index;
    params.desired_size = desired_size;
    params.dpi = dpi;

    return ocl_open_face(impl->oc_library, buf, &params, oface);
}
#endif

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    CFURLRef       url;

    CFStringRef path;
    CFIndex     path_len;

    size_t copy_len;

    if (!font) {
        return 0;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    url = CTFontDescriptorCopyAttribute(impl->ct_font, kCTFontURLAttribute);

    if (url == NULL) {
        return 0;
    }

    path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
    CFRelease(url);

    if (path == NULL) {
        return 0;
    }

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
