#include "shared.h"
#ifdef ONECORE_CORETEXT

#include <CoreText/CoreText.h>
#include <math.h>

inline oc_error oc_init_library(oc_library* plibrary) {
    return plibrary == NULL ? oc_error_invalid_param : oc_error_ok;
}

inline void oc_free_library(oc_library library) {
    (void)library;
}

static oc_error open_face_from_descriptors(CFArrayRef cf_descriptors_ref, const oc_face_params* pparams, oc_face* pface) {
    CFIndex count = CFArrayGetCount(cf_descriptors_ref);
    if (count == 0) {
        return oc_error_failed_to_open;
    }

    oc_face_params params = fill_face_params(pparams);
    if (params.face_index >= count) {
        return oc_error_invalid_param;
    }

    CTFontDescriptorRef ctf_descriptor_ref = (CTFontDescriptorRef)CFArrayGetValueAtIndex(cf_descriptors_ref, params.face_index);
    if (ctf_descriptor_ref == NULL) {
        return oc_error_out_of_memory;
    }

    oc_16p16 scaled = (params.desired_size * params.dpi + 36) / 72;
    CTFontRef ctf_font_ref = CTFontCreateWithFontDescriptor(ctf_descriptor_ref, scaled / 64.0, NULL);
    if (ctf_font_ref == NULL) {
        return oc_error_out_of_memory;
    }

    oc_16p16 scale = oc_div_16p16(scaled, CTFontGetUnitsPerEm(ctf_font_ref));
    int32_t ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        // todo: add this test case
        return oc_error_invalid_param;
    }

    CGFloat size = CTFontGetSize(ctf_font_ref);
    uint16_t upem = CTFontGetUnitsPerEm(ctf_font_ref);

    pface->internals = (void*)ctf_font_ref;
    pface->metrics.upem = upem;
    pface->metrics.ppem = ppem;
    pface->metrics.scale = scale;
    pface->metrics.ascent = (CTFontGetAscent(ctf_font_ref) * upem / size);
    pface->metrics.descent = (CTFontGetDescent(ctf_font_ref) * upem / size);
    pface->metrics.leading = (CTFontGetLeading(ctf_font_ref) * upem / size);
    pface->metrics.underline_position = (CTFontGetUnderlinePosition(ctf_font_ref) * upem / size);
    pface->metrics.underline_thickness = (CTFontGetUnderlineThickness(ctf_font_ref) * upem / size);

    return oc_error_ok;
}

oc_error oc_open_face(oc_library library, const char* path, const oc_face_params* pparams, oc_face* pface) {
    (void)library;

    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (path == NULL) {
        return oc_error_invalid_param;
    }

    CFStringRef cf_path_ref = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    if (cf_path_ref == NULL) {
        // macos error handling sucks
        return oc_error_failed_to_open;
    }

    CFURLRef cf_url_ref = CFURLCreateWithFileSystemPath(NULL, cf_path_ref, kCFURLPOSIXPathStyle, false);
    CFRelease(cf_path_ref);

    if (cf_url_ref == NULL) {
        return oc_error_failed_to_open;
    }

    CFArrayRef cf_descriptors_ref = CTFontManagerCreateFontDescriptorsFromURL(cf_url_ref);
    CFRelease(cf_url_ref);

    if (cf_descriptors_ref == NULL) {
        return oc_error_failed_to_open;
    }

    oc_error err = open_face_from_descriptors(cf_descriptors_ref, pparams, pface);
    CFRelease(cf_descriptors_ref);

    return err;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, const oc_face_params* pparams, oc_face* pface) {
    (void)library;

    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    CFDataRef cf_data_ref = CFDataCreateWithBytesNoCopy(NULL, data, size, kCFAllocatorNull);
    if (cf_data_ref == NULL) {
        return oc_error_out_of_memory;
    }

    CFArrayRef cf_descriptors_ref = CTFontManagerCreateFontDescriptorsFromData(cf_data_ref);
    CFRelease(cf_data_ref);

    if (cf_descriptors_ref == NULL) {
        return oc_error_failed_to_open;
    }

    oc_error err = open_face_from_descriptors(cf_descriptors_ref, pparams, pface);
    CFRelease(cf_descriptors_ref);

    return err;
}

void oc_free_face(oc_face face) {
    CFRelease(face.internals);
}

uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    if (charcode > 0x10FFFF) {
        return 0;
    }

    // check out CFStringGetSurrogatePairForLongCharacter

    // CTFontGetGlyphsForCharacters writes cg_glyph[1] when the length is 2 (i.e. when encoding a surrogate pair)
    // in this case it will always be set to 0, but we still need to pass 2 elements
    // we reuse the second element to store the utf16 character sequence length
    CGGlyph cg_glyph[2];
    UniChar uni_char[2];

    if (charcode <= 0xFFFF) {
        uni_char[0] = charcode;
        cg_glyph[1] = 1;
    } else {
        uint32_t norm = charcode - 0x10000;
        uni_char[0] = (norm >> 10) + 0xD800;
        uni_char[1] = (norm & 0x3FF) + 0xDC00;
        cg_glyph[1] = 2;
    }

    // cg_glyph[0] will always be set by Core Text no matter the status
    // thus we can ignore returned value
    CTFontGetGlyphsForCharacters(
        face.internals,
        uni_char,
        cg_glyph,
        cg_glyph[1]);

    return cg_glyph[0];
}

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable, void** pcontext) {
    if (ptable == NULL || pcontext == NULL) {
        return oc_error_invalid_param;
    }

    CFDataRef cf_data_ref = CTFontCopyTable(face.internals, tag, kCTFontTableOptionNoOptions);
    if (cf_data_ref == NULL) {
        // todo: fix this it can be oom error
        return oc_error_table_missing;
    }

    oc_table table;
    table.data = CFDataGetBytePtr(cf_data_ref);
    table.size = CFDataGetLength(cf_data_ref);

    *ptable = table;
    *pcontext = (void*)cf_data_ref;

    return oc_error_ok;
}

inline void oc_free_table(oc_face face, void* context) {
    (void)face;
    CFRelease(context);
}

void oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_glyph_metrics* pmetrics) {
    if (pmetrics == NULL) {
        return;
    }

    CFIndex glyph_count = CTFontGetGlyphCount(face.internals);
    if (glyph_index >= glyph_count) {
        memset(pmetrics, 0, sizeof(oc_glyph_metrics));
        return;
    }

    CGSize advance;
    CTFontGetAdvancesForGlyphs(face.internals, kCTFontOrientationHorizontal, &glyph_index, &advance, 1);

    CGRect bbox = CTFontGetBoundingRectsForGlyphs(face.internals, kCTFontOrientationHorizontal, &glyph_index, NULL, 1);

    CGFloat size = CTFontGetSize(face.internals);
    uint16_t upem = face.metrics.upem;

    pmetrics->width = bbox.size.width * upem / size;
    pmetrics->height = bbox.size.height * upem / size;
    pmetrics->bearing_x = bbox.origin.x * upem / size;
    pmetrics->bearing_y = (bbox.size.height + bbox.origin.y) * upem / size;
    pmetrics->advance = advance.width * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        return;
    }

    pmetrics->width = oc_mul_16p16(pmetrics->width, face.metrics.scale);
    pmetrics->height = oc_mul_16p16(pmetrics->height, face.metrics.scale);
    pmetrics->bearing_x = oc_mul_16p16(pmetrics->bearing_x, face.metrics.scale);
    pmetrics->bearing_y = oc_mul_16p16(pmetrics->bearing_y, face.metrics.scale);
    pmetrics->advance = oc_mul_16p16(pmetrics->advance, face.metrics.scale);
}

typedef struct point_2f {
    float x;
    float y;
} point_2f;

typedef struct outline_context {
    const oc_outline_funcs* funcs;
    void* ctx;
    CGPoint start;
    CGPoint origin;
    CGFloat fsize;
    CGFloat funits_per_em;
} outline_context;

static void oc_path_applier(void* info, const CGPathElement* element) {
    outline_context* ctx = (outline_context*)info;
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
        point_2f forigin = { ctx->origin.x * fupem / fppem, ctx->origin.y * fupem / fppem };
        point_2f fcontrol = { element->points[0].x * fupem / fppem, element->points[0].y * fupem / fppem };
        point_2f fto = { element->points[1].x * fupem / fppem, element->points[1].y * fupem / fppem };

        point_2f cubic[2];
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

bool oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    if (outline_funcs == NULL) {
        return false;
    }

    CGPathRef path = CTFontCreatePathForGlyph(face.internals, glyph_index, NULL);
    if (path == NULL) {
        return false;
    }

    outline_context ctx = { 0 };
    ctx.funcs = outline_funcs;
    ctx.ctx = context;
    ctx.fsize = CTFontGetSize(face.internals);
    ctx.funits_per_em = CTFontGetUnitsPerEm(face.internals);

    CGPathApply(path, &ctx, oc_path_applier);
    CGPathRelease(path);

    return true;
}

// smth is off with mac!!!!
oc_error oc_render_glyph(oc_face face, uint16_t glyph_index, oc_bbox* pbbox, unsigned char* buffer, size_t buffer_size) {
    if (pbbox == NULL) {
        return oc_error_invalid_param;
    }

    CFIndex glyph_count = CTFontGetGlyphCount(face.internals);
    if (glyph_index >= glyph_count) {
        return oc_error_invalid_param;
    }

    CGRect rect;
    CTFontGetBoundingRectsForGlyphs(
        face.internals,
        kCTFontOrientationHorizontal,
        &glyph_index,
        &rect,
        1);

    CGFloat size = CTFontGetSize(face.internals);
    uint16_t upem = face.metrics.upem;

    oc_26p6 origin_x = oc_mul_16p16(rect.origin.x * upem / size, face.metrics.scale);
    oc_26p6 origin_y = oc_mul_16p16(rect.origin.y * upem / size, face.metrics.scale);

    oc_26p6 w = oc_mul_16p16(rect.size.width * upem / size, face.metrics.scale);
    oc_26p6 h = oc_mul_16p16(rect.size.height * upem / size, face.metrics.scale);

    oc_26p6 frac_x = origin_x - OC_26P6_FLOOR(origin_x);
    oc_26p6 frac_y = origin_y - OC_26P6_FLOOR(origin_y);

    uint32_t rows = (h + frac_y + 63) >> 6;
    uint32_t cols = (w + frac_x + 63) >> 6;

    pbbox->rows = rows;
    pbbox->cols = cols;

    if (buffer == NULL) {
        return oc_error_ok;
    }

    if (buffer_size < rows * cols) {
        return oc_error_insufficient_buffer;
    }

    CGColorSpaceRef linear_gray = CGColorSpaceCreateWithName(kCGColorSpaceLinearGray);
    if (linear_gray == NULL) {
        return oc_error_out_of_memory;
    }

    memset(buffer, 0, rows * cols);

    CGContextRef ctx = CGBitmapContextCreate(
        buffer,
        cols,
        rows,
        8,
        cols,
        linear_gray,
        kCGImageAlphaOnly);
    CGColorSpaceRelease(linear_gray);
    if (ctx == NULL) {
        return oc_error_out_of_memory;
    }

    CGRect fill_rect;
    fill_rect.origin.x = 0;
    fill_rect.origin.y = 0;
    fill_rect.size.height = rows;
    fill_rect.size.width = cols;

    // https://github.com/ghostty-org/ghostty/blob/main/src/font/face/coretext.zig#L478

    CGContextSetGrayFillColor(ctx, 0.0, 0.0);
    CGContextFillRect(ctx, fill_rect);

    CGContextSetAllowsFontSmoothing(ctx, false);
    CGContextSetShouldSmoothFonts(ctx, false);

    CGContextSetAllowsFontSubpixelPositioning(ctx, true);
    CGContextSetShouldSubpixelPositionFonts(ctx, true);

    CGContextSetAllowsFontSubpixelQuantization(ctx, false);
    CGContextSetShouldSubpixelQuantizeFonts(ctx, false);

    CGContextSetAllowsAntialiasing(ctx, true);
    CGContextSetShouldAntialias(ctx, true);

    CGContextSetGrayFillColor(ctx, 1.0, 1.0);
    CGContextSetGrayStrokeColor(ctx, 1.0, 1.0);

    CGContextTranslateCTM(ctx, frac_x / 64.0, frac_y / 64.0);

    CGPoint pos = {
        -origin_x / 64.0,
        -origin_y / 64.0,
    };

    CTFontDrawGlyphs(
        face.internals,
        &glyph_index,
        &pos,
        1,
        ctx);
    CGContextRelease(ctx);

    return oc_error_ok;
}

#endif // ONECORE_CORETEXT
