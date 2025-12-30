#include <onecore.h>
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

    CGGlyph cg_glyph;
    UniChar uni_char[2];

    if (charcode <= 0xFFFF) {
        uni_char[0] = charcode;
        uni_char[1] = 0;
    } else {
        uint32_t norm = charcode - 0x10000;
        uni_char[0] = (norm >> 10) + 0xD800;
        uni_char[1] = (norm & 0x3FF) + 0xDC00;
    }

    CTFontGetGlyphsForCharacters(
        face.ct_font_ref,
        uni_char,
        &cg_glyph,
        2 - (uni_char[1] == 0));

    return cg_glyph;
}

#endif // ONECORE_CORETEXT
