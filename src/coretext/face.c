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
        return oc_error_out_of_memory;
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

    CGFontRef cgf_font_ref = CTFontCopyGraphicsFont(ctf_font_ref, NULL);
    CFRelease(ctf_font_ref);

    if (cgf_font_ref == NULL) {
        return oc_error_out_of_memory;
    }

    pface->ct_font_ref = cgf_font_ref;
    return oc_error_ok;
}

void oc_face_free(oc_face face) {
    CGFontRelease(face.ct_font_ref);
}

#endif // ONECORE_CORETEXT
