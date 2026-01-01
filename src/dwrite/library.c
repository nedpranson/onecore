#include <onecore.h>
#ifdef ONECORE_DWRITE

#include "../unexpected.h"

oc_error oc_init_library(oc_library* plibrary) {
    if (plibrary == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        &IID_IDWriteFactory,
        (IUnknown**)&plibrary->dw_factory);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    plibrary->memory_font_file_loader = NULL;
    return oc_error_ok;
}

void oc_free_library(oc_library library) {
    if (library.memory_font_file_loader != NULL) {
        library.dw_factory->lpVtbl->UnregisterFontFileLoader(
                library.dw_factory,
                library.memory_font_file_loader);
        library.memory_font_file_loader->lpVtbl->Release(library.memory_font_file_loader);
    }
    library.dw_factory->lpVtbl->Release(library.dw_factory);
}

#endif // ONECORE_DWRITE
