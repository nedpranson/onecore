#include <onecore.h>
#include <stdint.h>
#ifdef ONECORE_CORETEXT

oc_error oc_face_new(oc_library library, const char* path, long face_index, oc_face* pface) {
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

    if (face_index < 0 || face_index >= CFArrayGetCount(cf_descriptors_ref)) {
        CFRelease(cf_descriptors_ref);
        return oc_error_invalid_param;
    }

    CTFontDescriptorRef ctf_descriptor_ref = (CTFontDescriptorRef)CFArrayGetValueAtIndex(cf_descriptors_ref, face_index);
    CFRetain(cf_descriptors_ref);

    if (ctf_descriptor_ref == NULL) {
        return oc_error_out_of_memory;
    }

    CTFontRef ctf_font_ref = CTFontCreateWithFontDescriptor(ctf_descriptor_ref, 0, NULL);
    CFRelease(ctf_descriptor_ref);

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

void oc_face_free(oc_face face) {
    CFRelease(face.ct_font_ref);
}

uint16_t oc_face_get_char_index(oc_face face, uint32_t charcode) {
    if (charcode > 0x10FFFF) {
        return 0;
    }

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

oc_error oc_face_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable) {
    oc_table table;

    if (ptable == NULL) {
        return oc_error_invalid_param;
    }

    CFDataRef cf_data_ref = CTFontCopyTable(face.ct_font_ref, tag, kCTFontTableOptionNoOptions);
    if (cf_data_ref == NULL) {
        return oc_error_table_missing;
    }

    table.buffer = CFDataGetBytePtr(cf_data_ref);
    table.size = CFDataGetLength(cf_data_ref);
    table.__handle = (void*)cf_data_ref;

    *ptable = table;

    return oc_error_ok;
}

inline void oc_table_free(oc_table table) {
    CFRelease(table.__handle);
}

void oc_face_get_metrics(oc_face face, oc_metrics* pmetrics) {
    CGFloat fsize = CTFontGetSize(face.ct_font_ref);
    CGFloat funits_per_em = (CGFloat)CTFontGetUnitsPerEm(face.ct_font_ref);

    pmetrics->units_per_em = (uint16_t)funits_per_em;
    pmetrics->ascent = (uint16_t)(CTFontGetAscent(face.ct_font_ref) * funits_per_em / fsize);
    pmetrics->descent = (uint16_t)(CTFontGetDescent(face.ct_font_ref) * funits_per_em / fsize);
    pmetrics->leading = (int16_t)(CTFontGetLeading(face.ct_font_ref) * funits_per_em / fsize);
    pmetrics->underline_position = (int16_t)(CTFontGetUnderlinePosition(face.ct_font_ref) * funits_per_em / fsize);
    pmetrics->underline_thickness = (uint16_t)(CTFontGetUnderlineThickness(face.ct_font_ref) * funits_per_em / fsize);
}

#endif // ONECORE_CORETEXT
