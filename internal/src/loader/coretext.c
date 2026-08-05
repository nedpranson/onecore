#include <stdint.h>
#define ONECORE_IMPLEMENTATION
#include "../onecore.h"

/* ONECORE_CORETEXT_LOADER_IMPLEMENTATION */
#include <CoreText/CoreText.h>

static oc_error oc__init_face(CTFontDescriptorRef descriptor, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    CTFontRef ct_font;
    oc_face   face;

    oc_16p16 scaled;
    CGFloat  size;

    oc_16p16 ppem;
    uint16_t upem;

    scaled = (desired_size * dpi + 36) / 72;
    ct_font = CTFontCreateWithFontDescriptor(descriptor, scaled / 64.0, NULL);

    if (ct_font == NULL) {
        return oc_error_out_of_memory;
    }

    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        CFRelease(ct_font);
        return oc_error_invalid_pixel_size;
    }

    size = CTFontGetSize(ct_font);
    upem = CTFontGetUnitsPerEm(ct_font);

    face.impl = (oc_face_impl*)ct_font;
    face.size.scale = oc_div_16p16(scaled, upem);
    face.size.ppem = (uint16_t)ppem;
    face.upem = upem;
    face.ascent = CTFontGetAscent(ct_font) * upem / size;
    face.descent = CTFontGetDescent(ct_font) * upem / size;
    face.leading = CTFontGetLeading(ct_font) * upem / size;
    face.underline_position = CTFontGetUnderlinePosition(ct_font) * upem / size;
    face.underline_thickness = CTFontGetUnderlineThickness(ct_font) * upem / size;

    *oface = face;
    return oc_error_ok;
}

static oc_error oc__open_face_from_descriptors(CFArrayRef descriptors, const oc_open_params* uparams, oc_face* oface) {
    CTFontDescriptorRef descriptor;

    CFIndex        count = CFArrayGetCount(descriptors);
    oc_open_params params = oc__open_params_defaults(uparams);

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

    return oc__init_face(descriptor, params.desired_size, params.dpi, oface);
}

oc_error ocl_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    CFStringRef ct_path;
    CFURLRef    url_path;

    CFArrayRef descriptors;
    oc_error   err;

    if (!(library && path && oface)) {
        return oc_error_invalid_param;
    }

    // CFURLCreateWithFileSystemPath returns NULL when allocating empty string
    if (*path == '\0') {
        return oc_error_failed_to_open;
    }

    // todo (stage 2): validate utf8 so ct_path would only fail on oom
    ct_path = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    if (ct_path == NULL) {
        return oc_error_failed_to_open; // or oom
    }

    url_path = CFURLCreateWithFileSystemPath(NULL, ct_path, kCFURLPOSIXPathStyle, false);
    CFRelease(ct_path);

    if (url_path == NULL) {
        return oc_error_out_of_memory;
    }

    descriptors = CTFontManagerCreateFontDescriptorsFromURL(url_path);
    CFRelease(url_path);

    // todo (stage 2): think how to reliably handle this err
    if (descriptors == NULL) {
        // file not found
        // invalid file
        // oom
        return oc_error_failed_to_open;
    }

    err = oc__open_face_from_descriptors(descriptors, uparams, oface);
    CFRelease(descriptors);

    return err;
}

oc_error ocl_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    CFDataRef  ct_data;
    CFArrayRef descriptors;
    oc_error   err;

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
        // invalid file
        // oom
        return oc_error_failed_to_open;
    }

    err = oc__open_face_from_descriptors(descriptors, uparams, oface);
    CFRelease(descriptors);

    return err;
}

void ocl_free_face(oc_face* face) {
    if (face) {
        CFRelease(face->impl);
        memset(face, 0, sizeof(*face));
    }
}

uint16_t ocl_get_char_index(const oc_face* face, uint32_t charcode) {
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

oc_error ocl_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t  ppem;

    CTFontRef ct_font;
    CTFontRef ct_font_copy;

    if (!face) {
        return oc_error_invalid_param;
    }

    if (desired_size < 1 << 6) {
        return oc_error_invalid_param;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    scaled = (desired_size * dpi + 36) / 72;
    scale = oc_div_16p16(scaled, face->upem);

    ct_font = (CTFontRef)face->impl;
    ct_font_copy = CTFontCreateCopyWithAttributes(ct_font, scaled / 64.0, NULL, NULL);

    if (ct_font_copy == NULL) {
        return oc_error_out_of_memory;
    }

    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        CFRelease(ct_font_copy);
        return oc_error_invalid_pixel_size;
    }

    face->impl = (oc_face_impl*)ct_font_copy;
    face->size.scale = scale;
    face->size.ppem = (uint16_t)ppem;

    CFRelease(ct_font);
    return oc_error_ok;
}

oc_error ocl_get_sfnt_table(const oc_face* face, oc_tag tag, uint32_t offset, void* data, uint32_t* size) {
    CTFontRef ct_font;
    CFDataRef ct_table;

    const UInt8* buffer;
    CFIndex      length;

    if (!(face && size)) {
        return oc_error_invalid_param;
    }

    ct_font = (CTFontRef)face->impl;
    ct_table = CTFontCopyTable(ct_font, tag, kCTFontTableOptionNoOptions);
    length = (CFIndex)*size;

    assert(length == 0 || length >= offset);

    if (ct_table == NULL) {
        // todo (stage 2): check if this can oom
        return oc_error_table_missing; // or oom
    }

    if (length == 0) {
        length = CFDataGetLength(ct_table);

        assert(UINT32_MAX >= length);
        *size = (uint32_t)length;
    } else {
        buffer = CFDataGetBytePtr(ct_table);
        memcpy(data, buffer + offset, length);
    }

    CFRelease(ct_table);
    return oc_error_ok;
}

void ocl_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    CTFontRef ct_font;
    CFIndex   count;

    CGSize advance;
    CGRect rect;

    uint16_t upem;
    CGFloat  size;
    oc_26p6  scale;

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

    upem = face->upem;
    size = CTFontGetSize(ct_font);

    metrics.width = rect.size.width * upem / size;
    metrics.height = rect.size.height * upem / size;
    metrics.bearing_x = rect.origin.x * upem / size;
    metrics.bearing_y = (rect.size.height + rect.origin.y) * upem / size;
    metrics.advance = advance.width * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->size.scale;

    metrics.width = oc_mul_16p16(metrics.width, scale);
    metrics.height = oc_mul_16p16(metrics.height, scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, scale);
    metrics.advance = oc_mul_16p16(metrics.advance, scale);

    if (flags & OC_LOAD_NO_FITTING) {
        goto exit;
    }

    oc__fit_metrics(&metrics);
exit:
    if (ometrics)
        *ometrics = metrics;
}

void ocl_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    CTFontRef ct_font;
    CGRect    rect;

    uint16_t upem;
    CGFloat  size;
    oc_26p6  scale;

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

    upem = face->upem;
    size = CTFontGetSize(ct_font);

    cbox.min_x = CGRectGetMinX(rect) * upem / size;
    cbox.min_y = CGRectGetMinY(rect) * upem / size;
    cbox.max_x = CGRectGetMaxX(rect) * upem / size;
    cbox.max_y = CGRectGetMaxY(rect) * upem / size;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->size.scale;

    cbox.min_x = oc_mul_16p16(cbox.min_x, scale);
    cbox.min_y = oc_mul_16p16(cbox.min_y, scale);
    cbox.max_x = oc_mul_16p16(cbox.max_x, scale);
    cbox.max_y = oc_mul_16p16(cbox.max_y, scale);

exit:
    if (ocbox)
        *ocbox = cbox;
}

typedef struct {
    float x;
    float y;
} oc__point_2f;

typedef struct {
    const oc_outline_funcs* funcs;
    void*                   ctx;
    CGPoint                 start;
    CGPoint                 origin;
    CGFloat                 fsize;
    CGFloat                 funits_per_em;
} oc__outline_context;

static void oc__path_applier(void* info, const CGPathElement* element) {
    oc__outline_context* ctx = (oc__outline_context*)info;
    CGFloat              fppem = ctx->fsize;
    CGFloat              fupem = ctx->funits_per_em;

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

bool ocl_get_outline(const oc_face* face, uint16_t index, oc_load_flags flags, const oc_outline_funcs* funcs, void* user) {
    CTFontRef           ct_font;
    CGPathRef           outline;
    oc__outline_context context = { 0 };

    (void)flags;

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

typedef struct {
    size_t len;
    size_t cap;
} oc__array;

#define oc__head(arr)      ((oc__array*)(arr) - 1)
#define oc__make(type)     ((type)NULL)
#define oc__free(arr)      ((void) ((arr) ? free(oc__head(arr)) : (void)0), (arr)=NULL)
#define oc__len(arr)       ((arr) ? oc__head(arr)->len : 0)
#define oc__cap(arr)       ((arr) ? oc__head(arr)->cap : 0)
#define oc__grow(arr, cap) oc__grow_impl(arr, sizeof(*arr), cap)

#define oc__append(arr, val) \
    ((arr = oc__grow(arr, oc__len(arr) + 1)), \
     (arr) ? ((arr)[oc__head(arr)->len++] = (val), 1) : 0)

// note: this impl will not preserve arr on oom
static inline void* oc__grow_impl(void* arr, size_t size, size_t new_cap) {
    void* new_arr = NULL;

    size_t len = oc__len(arr);
    size_t cap = oc__cap(arr);

    if (cap >= new_cap) {
        return arr;
    }

    // note: it would be better to ensure init_cap is atleast one byte
    //       when we will use an type with bigger size then 128, we should update
    size_t init_cap = 128 / size;
    new_cap += new_cap / 2 + init_cap;

    // note: we ignore alignment for now as for basic types it will be fine
    //       though when we will use structs with bigger allignment then 16, we should update
    oc__array* new_head = malloc(sizeof(*new_head) + new_cap * size);
    if (new_head) {
        new_arr = new_head + 1;

        new_head->len = len;
        new_head->cap = new_cap;

        memcpy(new_arr, arr, len * size);

        oc__free(arr);
        return new_head + 1;
    }

    return arr;
}

#define OC__CURVE_TAG_ON    0x1
#define OC__CURVE_TAG_CONIC 0x0
#define OC__CURVE_TAG_CUBIC 0x2

typedef struct {
    oc_point          points[3];
    CGPathElementType type;
} oc__path_element;

typedef struct {
    uint8_t*  tags;
    oc_point* points;
    uint16_t* contours;

    oc__path_element element;
    oc_point         start_point;

    CGFloat fppem;
    CGFloat fupem;
} oc__applier_context;

static inline bool oc__is_midpoint(oc_point pt, oc_point a, oc_point b) {
    uint64_t sumx = (uint64_t)a.x + (uint64_t)b.x;
    uint64_t sumy = (uint64_t)a.y + (uint64_t)b.y;
    return (uint64_t)pt.x * 2 == sumx && (uint64_t)pt.y * 2 == sumy;
}

static inline bool oc__points_equal(oc_point a, oc_point b) {
    return a.x == b.x && a.y == b.y;
}

static void oc__walk_applier(void* info, const CGPathElement* element) {
    oc__applier_context* ctx = (oc__applier_context*)info;

    oc__path_element next_element = {
        {
            { element->points[0].x * ctx->fupem / ctx->fppem * 2.0, element->points[0].y * ctx->fupem / ctx->fppem * 2.0 },
            { element->points[1].x * ctx->fupem / ctx->fppem * 2.0, element->points[1].y * ctx->fupem / ctx->fppem * 2.0 },
            { element->points[2].x * ctx->fupem / ctx->fppem * 2.0, element->points[2].y * ctx->fupem / ctx->fppem * 2.0 },
        },
        element->type,
    };

    // todo: do not forget to check oom
    switch (ctx->element.type) {
    case kCGPathElementMoveToPoint:
        // maybe this can happen
        // if (final_count > 0) {
        //     if (contour_count == 0 || contour_ends[contour_count - 1] != final_count - 1) {
        //         contour_ends[contour_count++] = final_count - 1;
        //     }
        // }

        oc__append(ctx->points, ctx->element.points[0]);
        oc__append(ctx->tags, OC__CURVE_TAG_ON);

        ctx->start_point = ctx->element.points[0];
        break;
    case kCGPathElementAddLineToPoint:
        if (!oc__points_equal(ctx->element.points[0], ctx->start_point)) {
            oc__append(ctx->points, ctx->element.points[0]);
            oc__append(ctx->tags, OC__CURVE_TAG_ON);
        }
        break;
    case kCGPathElementAddQuadCurveToPoint:
        oc__append(ctx->points, ctx->element.points[0]);
        oc__append(ctx->tags, OC__CURVE_TAG_CONIC);

        bool implicit = false;
        bool closing = oc__points_equal(ctx->element.points[1], ctx->start_point);

        if (next_element.type == kCGPathElementAddQuadCurveToPoint) {
            implicit = oc__is_midpoint(ctx->element.points[1], ctx->element.points[0], next_element.points[0]);
        }

        if (!implicit && !closing) {
            oc__append(ctx->points, ctx->element.points[1]);
            oc__append(ctx->tags, OC__CURVE_TAG_ON);
        }
        break;
    case kCGPathElementAddCurveToPoint:
        // todo:
        break;
    case kCGPathElementCloseSubpath:
        assert(oc__len(ctx->points) > 0);
        oc__append(ctx->contours, oc__len(ctx->points) - 1);
        break;
    case -1:
        break;
    }

    ctx->element = next_element;
}

void ocl_print_raw_outline(const oc_face* face, uint16_t index) {
    CTFontRef           ct_font;
    CGPathRef           outline;
    oc__applier_context ctx = { 0 };

    if (!face) {
        return;
    }

    ct_font = (CTFontRef)face->impl;
    outline = CTFontCreatePathForGlyph(ct_font, index, NULL);

    if (!outline) {
        return;
    }

    ctx.fppem = CTFontGetSize(ct_font);
    ctx.fupem = CTFontGetUnitsPerEm(ct_font);
    ctx.element.type = -1;

    CGPathApply(outline, &ctx, oc__walk_applier);
    CGPathRelease(outline);

    assert(ctx.element.type == kCGPathElementCloseSubpath);
    // todo: check if it is even possible to get Close without any points
    if (oc__len(ctx.points) > 0) {
        oc__append(ctx.contours, oc__len(ctx.points) - 1);
    }

    printf("contours(%ld):\n", oc__len(ctx.contours));
    for (size_t i = 0; i < oc__len(ctx.contours); i++) {
        printf("  end(%d)\n", ctx.contours[i]);
    }

    printf("points(%ld):\n", oc__len(ctx.points));
    for (size_t i = 0; i < oc__len(ctx.points); i++) {
        printf("  tag(%d) point(%d, %d)\n", (int)ctx.tags[i], ctx.points[i].x >> 1, ctx.points[i].y >> 1);
    }

    oc__free(ctx.tags);
    oc__free(ctx.points);
    oc__free(ctx.contours);
}

// AI generated slop but it does seem to work

// typedef struct {
//     int type;        // 0 = move, 1 = line, 2 = quad, 3 = close
//     CGPoint pt;      // for move/line (Font Units)
//     CGPoint ctrl;    // for quad (Font Units)
//     CGPoint end;     // for quad (Font Units)
// } oc_path_elem;
//
// typedef struct {
//     oc_path_elem* elements;
//     size_t count;
//     size_t capacity;
//     CGFloat scale;
// } oc_elem_buffer;
//
// static inline bool points_equal_approx(CGPoint a, CGPoint b) {
//     return (fabs(a.x - b.x) < 0.05) && (fabs(a.y - b.y) < 0.05);
// }
//
// static inline bool is_midpoint(CGPoint pt, CGPoint a, CGPoint b) {
//     printf("pt: (%f, %f), a: (%f, %f), b: (%f, %f)\n", pt.x, pt.y, a.x, a.y, b.x, b.y);
//     double midx = (a.x + b.x) * 0.5;
//     double midy = (a.y + b.y) * 0.5;
//     return (fabs(pt.x - midx) < 0.05) && (fabs(pt.y - midy) < 0.05);
// }
//
// static void oc__collect_applier(void* info, const CGPathElement* element) {
//     oc_elem_buffer* buf = (oc_elem_buffer*)info;
//     CGFloat s = buf->scale;
//
//     if (buf->count >= buf->capacity) {
//         buf->capacity = buf->capacity ? buf->capacity * 2 : 128;
//         buf->elements = realloc(buf->elements, buf->capacity * sizeof(oc_path_elem));
//     }
//
//     switch (element->type) {
//     case kCGPathElementMoveToPoint:
//         buf->elements[buf->count++] = (oc_path_elem){
//             .type = 0,
//             .pt = CGPointMake(element->points[0].x * s, element->points[0].y * s)
//         };
//         break;
//
//     case kCGPathElementAddLineToPoint:
//         buf->elements[buf->count++] = (oc_path_elem){
//             .type = 1,
//             .pt = CGPointMake(element->points[0].x * s, element->points[0].y * s)
//         };
//         break;
//
//     case kCGPathElementAddQuadCurveToPoint:
//         buf->elements[buf->count++] = (oc_path_elem){
//             .type = 2,
//             .ctrl = CGPointMake(element->points[0].x * s, element->points[0].y * s),
//             .end  = CGPointMake(element->points[1].x * s, element->points[1].y * s)
//         };
//         break;
//
//     case kCGPathElementCloseSubpath:
//         buf->elements[buf->count++] = (oc_path_elem){ .type = 3 };
//         break;
//
//     default:
//         break;
//     }
// }
//
// typedef struct {
//     CGPoint pt;
//     int tag;
// } oc_final_pt;
//
// void ocl_print_raw_outline(const oc_face* face, uint16_t index) {
//     if (!face || !face->impl) return;
//
//     CTFontRef ct_font = (CTFontRef)face->impl;
//     CGPathRef outline = CTFontCreatePathForGlyph(ct_font, index, NULL);
//     if (!outline) return;
//
//     CGFloat fsize = CTFontGetSize(ct_font);
//     CGFloat fupem = CTFontGetUnitsPerEm(ct_font);
//
//     oc_elem_buffer buf = { 0 };
//     buf.scale = fupem / fsize;
//
//     CGPathApply(outline, &buf, oc__collect_applier);
//
//     oc_final_pt* final_points = malloc(buf.count * 2 * sizeof(oc_final_pt));
//     int final_count = 0;
//
//     int* contour_ends = malloc(buf.count * sizeof(int));
//     int contour_count = 0;
//
//     CGPoint contour_start_pt = CGPointZero;
//
//     for (size_t i = 0; i < buf.count; i++) {
//         switch (buf.elements[i].type) {
//         case 0: // MoveTo
//             if (final_count > 0) {
//                 if (contour_count == 0 || contour_ends[contour_count - 1] != final_count - 1) {
//                     contour_ends[contour_count++] = final_count - 1;
//                 }
//             }
//             contour_start_pt = buf.elements[i].pt;
//             final_points[final_count++] = (oc_final_pt){buf.elements[i].pt, 1};
//             break;
//
//         case 1: // LineTo
//             // Ignore if LineTo just repeats the start point at the end of the contour
//             if (!points_equal_approx(buf.elements[i].pt, contour_start_pt)) {
//                 final_points[final_count++] = (oc_final_pt){buf.elements[i].pt, 1};
//             }
//             break;
//
//         case 2: // QuadCurve
//             // Off-curve control point is ALWAYS pushed as tag(0)
//             final_points[final_count++] = (oc_final_pt){buf.elements[i].ctrl, 0};
//
//             // Check if end point is an implicit midpoint OR duplicate of starting point
//             bool is_implicit = false;
//             if (i + 1 < buf.count && buf.elements[i + 1].type == 2) { // need to know one in advance
//                 if (is_midpoint(buf.elements[i].end, buf.elements[i].ctrl, buf.elements[i + 1].ctrl)) {
//                     is_implicit = true;
//                 }
//             }
//
//             bool is_closing_start_pt = points_equal_approx(buf.elements[i].end, contour_start_pt);
//
//             // Skip pushing endpoint if it's synthetic OR if it's wrapping back to point #0
//             if (!is_implicit && !is_closing_start_pt) {
//                 final_points[final_count++] = (oc_final_pt){buf.elements[i].end, 1};
//             }
//             break;
//
//         case 3: // CloseSubpath
//             if (final_count > 0) {
//                 contour_ends[contour_count++] = final_count - 1;
//             }
//             break;
//         }
//     }
//
//     if (final_count > 0 && (contour_count == 0 || contour_ends[contour_count - 1] != final_count - 1)) {
//         contour_ends[contour_count++] = final_count - 1;
//     }
//
//     // Output exact FreeType format
//     printf("contours(%d):\n", contour_count);
//     for (int i = 0; i < contour_count; i++) {
//         printf("  end(%d)\n", contour_ends[i]);
//     }
//
//     printf("points(%d):\n", final_count);
//     for (int i = 0; i < final_count; i++) {
//         long x = (long)round(final_points[i].pt.x);
//         long y = (long)round(final_points[i].pt.y);
//         printf("  tag(%d) point(%ld, %ld)\n", final_points[i].tag, x, y);
//     }
//
//     free(final_points);
//     free(contour_ends);
//     free(buf.elements);
//     CGPathRelease(outline);
// }

oc_error ocl_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t pitch) {
    oc_error err = oc_error_ok;

    CTFontRef ct_font;
    CFIndex   count;

    oc_bbox cbox;
    oc_bbox pbox;

    CGColorSpaceRef linear_gray;
    CGContextRef    context;
    CGRect          rect;
    CGPoint         pos;

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
    ocl_get_glyph_cbox(face, index, OC_LOAD_DEFAULT, &cbox);

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

    linear_gray = CGColorSpaceCreateWithName(kCGColorSpaceLinearGray);
    if (linear_gray == NULL) {
        err = oc_error_out_of_memory;
        goto exit;
    }

    context = CGBitmapContextCreate(
        buffer,
        extent.cols,
        extent.rows,
        8,
        pitch,
        linear_gray,
        kCGImageAlphaNone);
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

    CGContextClearRect(context, rect);

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
    if (oextent)
        *oextent = extent;
    return err;
}
