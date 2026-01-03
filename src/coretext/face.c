#include <onecore.h>
#ifdef ONECORE_CORETEXT

static oc_error open_face_from_descriptors(CFArrayRef cf_descriptors_ref, long face_index, oc_face* pface) {
    CFIndex count = CFArrayGetCount(cf_descriptors_ref);
    if (count == 0) {
        return oc_error_failed_to_open;
    }

    // todo: make face_index unsigned what even is this
    if (face_index < 0 || face_index >= count) {
        return oc_error_invalid_param;
    }

    CTFontDescriptorRef ctf_descriptor_ref = (CTFontDescriptorRef)CFArrayGetValueAtIndex(cf_descriptors_ref, face_index);
    if (ctf_descriptor_ref == NULL) {
        return oc_error_out_of_memory;
    }

    // font size passed here!
    CTFontRef ctf_font_ref = CTFontCreateWithFontDescriptor(ctf_descriptor_ref, 0, NULL);
    if (ctf_font_ref == NULL) {
        return oc_error_out_of_memory;
    }

    // CGFontRef cgf_font_ref = CTFontCopyGraphicsFont(ctf_font_ref, NULL);
    // CFRelease(ctf_font_ref);

    // if (cgf_font_ref == NULL) {
    // return oc_error_out_of_memory;
    //}

    pface->ct_font_ref = ctf_font_ref;
    return oc_error_ok;
}

oc_error oc_open_face(oc_library library, const char* path, long face_index, oc_face* pface) {
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

    oc_error err = open_face_from_descriptors(cf_descriptors_ref, face_index, pface);
    CFRelease(cf_descriptors_ref);

    return err;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, long face_index, oc_face* pface) {
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

    oc_error err = open_face_from_descriptors(cf_descriptors_ref, face_index, pface);
    CFRelease(cf_descriptors_ref);

    return err;
}

void oc_free_face(oc_face face) {
    CFRelease(face.ct_font_ref);
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
        face.ct_font_ref,
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

    CFDataRef cf_data_ref = CTFontCopyTable(face.ct_font_ref, tag, kCTFontTableOptionNoOptions);
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
    CGFloat fsize = CTFontGetSize(face.ct_font_ref);
    CGFloat funits_per_em = (CGFloat)CTFontGetUnitsPerEm(face.ct_font_ref);

    pmetrics->units_per_em = (uint16_t)funits_per_em;
    pmetrics->ascent = (uint16_t)(CTFontGetAscent(face.ct_font_ref) * funits_per_em / fsize);
    pmetrics->descent = (uint16_t)(CTFontGetDescent(face.ct_font_ref) * funits_per_em / fsize);
    pmetrics->leading = (int16_t)(CTFontGetLeading(face.ct_font_ref) * funits_per_em / fsize);
    pmetrics->underline_position = (int16_t)(CTFontGetUnderlinePosition(face.ct_font_ref) * funits_per_em / fsize);
    pmetrics->underline_thickness = (uint16_t)(CTFontGetUnderlineThickness(face.ct_font_ref) * funits_per_em / fsize);
}

bool oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_glyph_metrics* pglyph_metrics) {
    if (pglyph_metrics == NULL) {
        return false;
    }

    CFIndex glyph_count = CTFontGetGlyphCount(face.ct_font_ref);
    if (glyph_index >= glyph_count) {
        return false;
    }

    CGSize advance;
    CTFontGetAdvancesForGlyphs(face.ct_font_ref, kCTFontOrientationHorizontal, &glyph_index, &advance, 1);

    CGRect bbox = CTFontGetBoundingRectsForGlyphs(face.ct_font_ref, kCTFontOrientationHorizontal, &glyph_index, NULL, 1);

    CGFloat fsize = CTFontGetSize(face.ct_font_ref);
    CGFloat funits_per_em = (CGFloat)CTFontGetUnitsPerEm(face.ct_font_ref);

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
        ctx->funcs->end_figure(ctx->ctx);
        break;
    }
}

void oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    CGPathRef path = CTFontCreatePathForGlyph(face.ct_font_ref, glyph_index, NULL);
    if (path == NULL) {
        printf("CTFontCreatePathForGlyph failed\n");
        return;
    }

    outline_context ctx = { 0 };
    ctx.funcs = outline_funcs;
    ctx.ctx = context;
    ctx.fsize = CTFontGetSize(face.ct_font_ref);
    ctx.funits_per_em = CTFontGetUnitsPerEm(face.ct_font_ref);

    CGPathApply(path, &ctx, oc_path_applier);
    CGPathRelease(path);
}

#endif // ONECORE_CORETEXT
