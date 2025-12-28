#include <onecore.h>
#ifdef ONECORE_DWRITE

#include "../unexpected.h"

oc_error oc_library_init(oc_library* plibrary)
{
    if (plibrary == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        &IID_IDWriteFactory,
        (IUnknown**)&plibrary->dw_factory);

    switch (err) {
    case S_OK:
        return oc_error_ok;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }
}

void oc_library_free(oc_library library)
{
    library.dw_factory->lpVtbl->Release(library.dw_factory);
}

#endif // ONECORE_DWRITE
