#include "onecore.h"
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

    CTFontRef ctf_font_ref = CTFontCreateWithFontDescriptor(ctf_descriptor_ref, params.desired_size / (float)params.dpi * 72.0f, NULL);
    if (ctf_font_ref == NULL) {
        return oc_error_out_of_memory;
    }

    pface->internals = (void*)ctf_font_ref;
    pface->font_size = params.desired_size; // todo: convert back from like get size...
    pface->font_dpi = params.dpi;

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

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable) {
    oc_table table;

    if (ptable == NULL) {
        return oc_error_invalid_param;
    }

    CFDataRef cf_data_ref = CTFontCopyTable(face.internals, tag, kCTFontTableOptionNoOptions);
    if (cf_data_ref == NULL) {
        return oc_error_table_missing;
    }

    table.data = CFDataGetBytePtr(cf_data_ref);
    table.size = CFDataGetLength(cf_data_ref);
    table.__handle = (void*)cf_data_ref;

    *ptable = table;

    return oc_error_ok;
}

inline void oc_free_table(oc_face face, oc_table table) {
    (void)face;
    CFRelease(table.__handle);
}

void oc_get_metrics(oc_face face, oc_metrics* pmetrics) {
    CGFloat fsize = CTFontGetSize(face.internals);
    CGFloat funits_per_em = (CGFloat)CTFontGetUnitsPerEm(face.internals);

    pmetrics->units_per_em = (uint16_t)funits_per_em;
    pmetrics->ascent = (uint16_t)(CTFontGetAscent(face.internals) * funits_per_em / fsize);
    pmetrics->descent = (uint16_t)(CTFontGetDescent(face.internals) * funits_per_em / fsize);
    pmetrics->leading = (int16_t)(CTFontGetLeading(face.internals) * funits_per_em / fsize);
    pmetrics->underline_position = (int16_t)(CTFontGetUnderlinePosition(face.internals) * funits_per_em / fsize);
    pmetrics->underline_thickness = (uint16_t)(CTFontGetUnderlineThickness(face.internals) * funits_per_em / fsize);
}

bool oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_glyph_metrics* pglyph_metrics) {
    if (pglyph_metrics == NULL) {
        return false;
    }

    CFIndex glyph_count = CTFontGetGlyphCount(face.internals);
    if (glyph_index >= glyph_count) {
        return false;
    }

    CGSize advance;
    CTFontGetAdvancesForGlyphs(face.internals, kCTFontOrientationHorizontal, &glyph_index, &advance, 1);

    CGRect bbox = CTFontGetBoundingRectsForGlyphs(face.internals, kCTFontOrientationHorizontal, &glyph_index, NULL, 1);

    CGFloat fsize = CTFontGetSize(face.internals);
    CGFloat funits_per_em = (CGFloat)CTFontGetUnitsPerEm(face.internals);

    pglyph_metrics->width = (uint16_t)(bbox.size.width * funits_per_em / fsize);
    pglyph_metrics->height = (uint16_t)(bbox.size.height * funits_per_em / fsize);
    pglyph_metrics->bearing_x = (int16_t)(bbox.origin.x * funits_per_em / fsize);
    pglyph_metrics->bearing_y = (int16_t)((bbox.size.height + bbox.origin.y) * funits_per_em / fsize);
    pglyph_metrics->advance = (uint16_t)(advance.width * funits_per_em / fsize);

    return true;
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
    CGFloat fsize = ctx->fsize;
    CGFloat funits_per_em = ctx->funits_per_em;

    switch (element->type) {
    case kCGPathElementMoveToPoint: {
        oc_point point = {
            element->points[0].x * funits_per_em / fsize,
            element->points[0].y * funits_per_em / fsize
        };

        ctx->funcs->start_figure(point, ctx->ctx);
        ctx->start = element->points[0];
        ctx->origin = element->points[0];
    }; break;
    case kCGPathElementAddLineToPoint: {
        oc_point point = {
            element->points[0].x * funits_per_em / fsize,
            element->points[0].y * funits_per_em / fsize
        };

        ctx->funcs->line_to(point, ctx->ctx);
        ctx->origin = element->points[0];
    } break;
    case kCGPathElementAddQuadCurveToPoint: {
        point_2f forigin = { ctx->origin.x * funits_per_em / fsize, ctx->origin.y * funits_per_em / fsize };
        point_2f fcontrol = { element->points[0].x * funits_per_em / fsize, element->points[0].y * funits_per_em / fsize };
        point_2f fto = { element->points[1].x * funits_per_em / fsize, element->points[1].y * funits_per_em / fsize };

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
            { element->points[0].x * funits_per_em / fsize, element->points[0].y * funits_per_em / fsize },
            { element->points[1].x * funits_per_em / fsize, element->points[1].y * funits_per_em / fsize },
            { element->points[2].x * funits_per_em / fsize, element->points[2].y * funits_per_em / fsize },
        };

        ctx->funcs->cubic_to(points[0], points[1], points[2], ctx->ctx);
        ctx->origin = element->points[2];
    } break;
    case kCGPathElementCloseSubpath:
        if (ctx->origin.x != ctx->start.x || ctx->origin.y != ctx->start.y) {
            oc_point point = { ctx->start.x * funits_per_em / fsize, ctx->start.y * funits_per_em / fsize };
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

oc_error oc_render_glyph(oc_face face, uint16_t glyph_index, oc_bbox* pbbox, unsigned char* buffer, size_t buffer_size) {
    CGRect rect;
    CTFontGetBoundingRectsForGlyphs(
        face.internals,
        kCTFontOrientationHorizontal,
        &glyph_index,
        &rect,
        1);

    CGFloat off_x = rect.origin.x - floor(rect.origin.x);
    CGFloat off_y = rect.origin.y - floor(rect.origin.y);

    if (buffer == NULL) {
        pbbox->height = ceil(rect.size.height + off_y);
        pbbox->width = ceil(rect.size.width + off_x);

        return oc_error_ok;
    }

    if (pbbox->height != ceil(rect.size.height + off_y) || pbbox->width != ceil(rect.size.width + off_x)) {
        return oc_error_invalid_param;
    }

    if (buffer_size < pbbox->height * pbbox->width) {
        return oc_error_insufficient_buffer;
    }

    CGColorSpaceRef linear_gray = CGColorSpaceCreateWithName(kCGColorSpaceLinearGray);
    if (linear_gray == NULL) {
        return oc_error_out_of_memory;
    }

    // why?
    memset(buffer, 0, pbbox->width * pbbox->height);

    CGContextRef ctx = CGBitmapContextCreate(
        buffer,
        pbbox->width,
        pbbox->height,
        8,
        pbbox->width,
        linear_gray,
        kCGImageAlphaOnly);
    CGColorSpaceRelease(linear_gray);
    if (ctx == NULL) {
        return oc_error_out_of_memory;
    }

    CGRect fill_rect;
    fill_rect.origin.x = 0;
    fill_rect.origin.y = 0;
    fill_rect.size.height = pbbox->height;
    fill_rect.size.width = pbbox->width;

    // todo: check if we need subpixel and stuff

    CGContextSetGrayFillColor(ctx, 0.0, 0.0);
    CGContextFillRect(ctx, fill_rect);

    CGContextSetAllowsAntialiasing(ctx, true);
    CGContextSetShouldAntialias(ctx, true);

    CGContextSetGrayFillColor(ctx, 1.0, 1.0);
    CGContextSetGrayStrokeColor(ctx, 1.0, 1.0);

    CGContextTranslateCTM(ctx, off_x, off_y);
    //CGContextScaleCTM(ctx, width / bounds.size.width, height / bounds.size.height);

    CGPoint pos = {
        .x = -rect.origin.x,
        .y = -rect.origin.y,
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
