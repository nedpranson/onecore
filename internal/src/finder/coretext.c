#include "freetype/freetype.h"
#include <stdint.h>
#define ONECORE_IMPLEMENTATION
#define ONECORE_FREETYPE_LOADER_IMPLEMENTATION
#include "onecore.h"

extern oc_error oc__init_face(CTFontDescriptorRef descriptor, oc_26p6 desired_size, uint16_t dpi, oc_face* oface);

/* ONECORE_CORETEXT_FINDER_IMPLEMENTATION */
#include <CoreText/CoreText.h>

typedef struct {
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    const oc_library* oc_library;
#endif
    CTFontDescriptorRef ct_font;
    CTFontRef           ct_face;
    CFStringRef         ct_family;
    oc_font             font;
} oc__font_impl;

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
struct oc_collection_impl {
    const oc_library* oc_library;
    CFArrayRef        ct_fonts;
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

    oc_collection     tmp_collection;
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
// todo: clean this shi up
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    ct_fonts2 = ct_fonts;
#else
    tmp_collection.impl = (oc_collection_impl*)ct_fonts;
#endif
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    ct_fonts2 = collection->impl->ct_fonts;
#else
    ct_fonts = (CFArrayRef)collection->impl;
#endif
    fonts = collection->fonts;
    nfonts = collection->nfonts;
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    tmp_collection.impl = collection->impl;
    collection->impl->ct_fonts = ct_fonts;
    ct_fonts = ct_fonts2;
#endif
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
#elif defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION)
// todo (stage 2): handle memory only fonts!
// when we will implement correct reconstruction function
// we could use FT_StreamRec to stream data and emulate it
// just build these ocf__offset_table ocf__table_record
// give to freetype and forget
// any other data we track on which tag we are and just give it back
typedef struct {
    int32_t  sfnt_version;
    uint16_t num_tables;
    uint16_t search_range;
    uint16_t entry_selector;
    uint16_t range_shift;
} ocf__offset_table;

typedef struct {
    uint32_t tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
} ocf__table_record;

static void* ocf__make_head(CGFontRef cg_font, CFArrayRef tags, uint32_t* size) {
    CFIndex ntags;

    uint32_t file_size;
    uint32_t head_size;

    bool  cff = false;
    void* head_data;

    ocf__offset_table* table;
    ocf__table_record* records;

    assert(cg_font != NULL);
    assert(tags != NULL);

    ntags = CFArrayGetCount(tags);
    assert(0 < ntags && ntags <= UINT16_MAX);

    head_size = sizeof(ocf__offset_table) + sizeof(ocf__table_record) * ntags;
    head_data = malloc(head_size);

    if (!head_data) {
        return NULL;
    }

    table = head_data;
    records = head_data + sizeof(ocf__offset_table);

    file_size = head_size;
    for (CFIndex i = 0; i < ntags; i++) {
        uint32_t tag = (uint32_t)(uintptr_t)CFArrayGetValueAtIndex(tags, i);

        CFDataRef table = CGFontCopyTableForTag(cg_font, tag);
        CFIndex   table_length = CFDataGetLength(table);

        ocf__table_record* record = records + i;

        if (tag == 'CFF ') {
            cff = true;
        }

        record->tag = CFSwapInt32HostToBig(tag);
        record->checksum = 0;
        record->offset = CFSwapInt32HostToBig(file_size);
        record->length = CFSwapInt32HostToBig((uint32_t)table_length);

        file_size += (table_length + 3) & ~3;
        CFRelease(table);
    }

    table->sfnt_version = cff ? 'OTTO' : CFSwapInt32HostToBig(0x10000);
    table->num_tables = CFSwapInt16HostToBig((uint16_t)ntags);
    table->search_range = 0;
    table->entry_selector = 0;
    table->range_shift = 0;

    *size = file_size;
    return head_data;
}

typedef struct {
    CGFontRef font;
    void*     head;
} ocf__context;

static void ocf__stream_close(FT_Stream stream) {
    const ocf__context* context = stream->descriptor.pointer;

    free(context->head);
    CFRelease(context->font);
}

static unsigned long ocf__stream_read(
    FT_Stream      stream,
    unsigned long  offset,
    unsigned char* buffer,
    unsigned long  count) {
    const ocf__context* context;

    const ocf__offset_table* table;
    const ocf__table_record* records;

    uint16_t ntags;

    const void* ptr;
    uint32_t    len;

    uint32_t head_size;

    CFDataRef ct_table = NULL;

    assert(stream != NULL);
    assert(stream->size >= offset && stream->size - offset >= count);

    if (count == 0) {
        return 0;
    }

    context = stream->descriptor.pointer;

    table = context->head;
    records = context->head + sizeof(ocf__offset_table);

    ntags = CFSwapInt16BigToHost(table->num_tables);
    head_size = sizeof(*table) + sizeof(*records) * ntags;

    if (head_size >= offset) {
        ptr = context->head + offset;
        len = head_size - offset;
    } else {
        uint16_t lo = 0;
        uint16_t hi = ntags;

        while (lo < hi) {
            uint16_t mid = lo + ((hi - lo) >> 1);
            if (CFSwapInt32BigToHost(records[mid].offset) < offset) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        assert(lo != ntags);

        ct_table = CGFontCopyTableForTag(context->font, CFSwapInt32BigToHost(records[lo].tag));

        ptr = CFDataGetBytePtr(ct_table);
        len = CFDataGetLength(ct_table);
    }

    memcpy(buffer, ptr, OC__MIN(count, len));
    if (ct_table)
        CFRelease(ct_table);

    return OC__MIN(count, len);
}

oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc__font_impl* impl;

    CGFontRef  cg_font;
    CFArrayRef tags;

    oc_face  face = { 0 };
    oc_error err = oc_error_ok;

    void*    file_head;
    uint32_t file_size;

    ocf__context* context;

    FT_Open_Args args = { 0 };
    FT_Stream    stream;

    FT_Face  ft_face;
    FT_Error ft_err;

    oc_open_params params;

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    assert(impl->ct_face != NULL); // todo: add these types asserts everywhere

    cg_font = CTFontCopyGraphicsFont(impl->ct_face, NULL);
    if (!cg_font) {
        oc__exit(oc_error_out_of_memory);
    }

    tags = CGFontCopyTableTags(cg_font);
    if (!tags) {
        CFRelease(cg_font);
        oc__exit(oc_error_out_of_memory);
    }

    file_head = ocf__make_head(cg_font, tags, &file_size);
    CFRelease(tags);

    if (!file_head) {
        CFRelease(cg_font);
        oc__exit(oc_error_out_of_memory);
    }

    context = malloc(sizeof(*context));
    if (!context) {
        CFRelease(cg_font);
        oc__exit(oc_error_out_of_memory);
    }

    context->font = cg_font;
    context->head = file_head;

    stream = calloc(1, sizeof(*stream));
    if (!stream) {
        free(file_head);
        CFRelease(cg_font);
        oc__exit(oc_error_out_of_memory);
    }

    stream->descriptor.pointer = context;
    stream->read = &ocf__stream_read;
    stream->close = &ocf__stream_close;
    stream->size = file_size;

    args.flags = FT_OPEN_STREAM;
    args.stream = stream;

    ft_err = FT_Open_Face((FT_Library)impl->oc_library, &args, 0, &ft_face);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        oc__exit(oc_error_out_of_memory);
    case FT_Err_Invalid_File_Format:
    case FT_Err_Unknown_File_Format:
    case FT_Err_Invalid_Stream_Operation:
        oc__exit(oc_error_failed_to_open);
    default:
        oc__exit(oc__unexpected(ft_err));
    }

    params.face_index = 0;
    params.desired_size = desired_size;
    params.dpi = dpi;

    params = oc__open_params_defaults(&params);
    err = oc__init_face(ft_face, &params, &face);

    if (err != oc_error_ok) {
        FT_Done_Face(ft_face);
    }
exit:
    *oface = face;
    return err;
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
