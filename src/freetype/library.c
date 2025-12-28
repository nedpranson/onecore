#include <onecore.h>

#ifdef ONECORE_FREETYPE

#include "../unexpected.h"

oc_error oc_library_init(oc_library* plibrary)
{
    if (plibrary == NULL) {
        return oc_error_invalid_param;
    }

    FT_Error err = FT_Init_FreeType(&plibrary->ft_library);
    switch (err) {
    case FT_Err_Ok:
        return oc_error_ok;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }
}

void oc_library_free(oc_library library)
{
    FT_Done_FreeType(library.ft_library);
}

#endif // ONECORE_FREETYPE
