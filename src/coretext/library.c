#include <onecore.h>

#ifdef ONECORE_CORETEXT

oc_error oc_library_init(oc_library* plibrary)
{
    return plibrary == NULL ? oc_error_invalid_param : oc_error_ok;
}

void oc_library_free(oc_library library)
{
    (void)library;
}

#endif // ONECORE_CORETEXT
