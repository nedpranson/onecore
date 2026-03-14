#include <CoreFoundation/CFArray.h>
#include <CoreText/CTFontDescriptor.h>
#define ONECORE_IMPLEMENTATION
#include "onecore.h"

/* ONECORE_CORETEXT_IMPLEMENTATION */
#include <CoreText/CoreText.h>

oc_error oc_init_library(oc_library* olibrary) {
    return olibrary == NULL ? oc_error_invalid_param : oc_error_ok;
}

void oc_free_library(oc_library* library) {
    if (library) memset(library, 0, sizeof(*library));
}

typedef struct {
    CTFontDescriptorRef ct_font;
    oc_font font;
} oc__font_impl;

static inline void oc__free_font_impl(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    free((char*)impl->font.path);
    free((char*)impl->font.family);
    free(impl);
}

oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_collection collection = { 0 };
    oc_error err = oc_error_ok;

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    collection.impl = NULL;
    collection.fonts = NULL;
    collection.elements = 0;
exit:
    if (ocollection) *ocollection = collection;
    return err;
}

void oc_free_collection(oc_collection* collection) {
    if (collection) {
        for (size_t i = 0; i < collection->elements; i++) {
            oc__free_font_impl(collection->fonts[i]);
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

static inline char* oc__copy_string(CFStringRef string) {
    assert(string != NULL);

    CFIndex length = CFStringGetLength(string);
    CFRange range = CFRangeMake(0, length);

    CFIndex size = 0;
    UInt8* out;

    assert(length > 0);

    CFStringGetBytes(
        string,
        range,
        kCFStringEncodingUTF8,
        0,
        false,
        NULL,
        0,
        &size);

    out = malloc(size + 1);
    if (out == NULL) {
        return NULL;
    }

    CFStringGetBytes(
        string,
        range,
        kCFStringEncodingUTF8,
        0,
        false,
        out,
        size,
        NULL);

    out[size] = '\0';
    return (char*)out;
}

static oc__font_impl* oc__init_font_impl(CTFontDescriptorRef ct_font) {
    oc__font_impl* impl;

    CFDictionaryRef ct_traits;
    CFURLRef ct_url;
    CFStringRef ct_path;
    CFStringRef ct_family;

    char* path = NULL;
    char* family = NULL;

    CFNumberRef weight_obj;
    double ct_weight;

    assert(ct_font != NULL);

    ct_url = CTFontDescriptorCopyAttribute(ct_font, kCTFontURLAttribute);
    if (ct_url == NULL) {
        goto exit;
    }

    ct_path = CFURLCopyFileSystemPath(ct_url, kCFURLPOSIXPathStyle);
    CFRelease(ct_url);

    if (ct_path == NULL) {
        goto exit;
    }

    path = oc__copy_string(ct_path);
    CFRelease(ct_path);

    if (path == NULL) {
        goto exit;
    }

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

    family = oc__copy_string(ct_family); 
    CFRelease(ct_family);

    if (family == NULL) {
        goto exit;
    }

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        goto exit;
    }

    impl->ct_font = ct_font;
    impl->font.path = path;
    impl->font.family = family;
    impl->font.weight = oc__convert(ct_weight) + 0.5;

    return impl;
exit:
    free(family);
    free(path);
    return NULL;
}

oc_error oc_load_fonts(oc_collection* collection) {
    CTFontCollectionRef ct_collection;
    CFArrayRef ct_fonts;

    size_t font_count;

    oc_font** fonts = NULL;
    size_t elements = 0;
    
    oc_error err = oc_error_ok;
    oc_collection collection_copy;

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
    fonts = malloc(font_count * sizeof(*fonts));
    
    if (fonts == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    for (size_t i = 0; i < font_count; i++) {
        CTFontDescriptorRef ct_font = CFArrayGetValueAtIndex(ct_fonts, i);
        // todo: there is probably much more wrong can happen than oom
        oc__font_impl* impl = oc__init_font_impl(ct_font);
        if (impl == NULL) {
            err = oc_error_out_of_memory;
            goto exit;
        }

        fonts[elements++] = &impl->font;
    }

    collection_copy.impl = (oc_collection_impl*)ct_fonts;
    collection_copy.fonts = fonts;
    collection_copy.elements = elements;

    ct_fonts = (CFArrayRef)collection->impl;
    elements = collection->elements;
    fonts = collection->fonts;

    *collection = collection_copy;
exit:
    while (elements--) oc__free_font_impl(fonts[elements]);
    free(fonts);
    if (ct_fonts) CFRelease(ct_fonts);

    return err;
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
    if (face) {
        CFRelease(face->impl);
        memset(face, 0, sizeof(*face));
    }
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
