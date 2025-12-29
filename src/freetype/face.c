#include <onecore.h>
#ifdef ONECORE_FREETYPE

#include "../unexpected.h"

oc_error oc_face_new(oc_library library, const char* path, long face_index, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (path == NULL) {
        return oc_error_invalid_param;
    }

    FT_Open_Args open_args = { 0 };
    open_args.flags = FT_OPEN_PATHNAME;
    open_args.pathname = (char*)path;

    // using FT_Open_Face as FT_New_Face fails if file extention does not match file type.
    FT_Error err = FT_Open_Face(library.ft_library, &open_args, face_index, &pface->ft_face);
    switch (err) {
    case FT_Err_Ok:
        return oc_error_ok;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    case FT_Err_Cannot_Open_Resource:
        return oc_error_failed_to_open;
    case FT_Err_Invalid_Argument:
        return oc_error_invalid_param;
    default:
        return unexpected(err);
    }
}

void oc_face_free(oc_face face) {
    FT_Done_Face(face.ft_face);
}

#endif // ONECORE_FREETYPE
