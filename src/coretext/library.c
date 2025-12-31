#include <onecore.h>

#ifdef ONECORE_CORETEXT

oc_error oc_init_library(oc_library* plibrary) {
    return plibrary == NULL ? oc_error_invalid_param : oc_error_ok;
}

void oc_free_library(oc_library library) {
    (void)library;
}

#endif // ONECORE_CORETEXT
