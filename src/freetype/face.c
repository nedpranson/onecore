#include <onecore.h>
#ifdef ONECORE_FREETYPE

#include "../unexpected.h"

oc_error oc_face_new(oc_library library, const char* path, long face_index, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    FT_Error err = FT_New_Face(library.ft_library, path, face_index, &pface->ft_face);
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
