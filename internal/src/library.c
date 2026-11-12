/*
 * @fuse
 * @base LIBRARY
 * @deps
 *  - BASE
 */
#include "onecore.h"
/* ---- */

#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#if defined(ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION) || defined(ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION)
#include <initguid.h>
#include <dwrite.h>
#endif

#if defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION) && defined(ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION)
struct oc_library {
    FT_Library      ft_library;
    IDWriteFactory* dw_factory;
};
#endif

#if !defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION) && !defined(ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION) && !defined(ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION)
static void* oc__noop_library;
#endif

#ifdef ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION
extern IDWriteFontFileLoader* oc__dw_file_loader;
#endif

oc_error oc_init_library(oc_library** olibrary) {
    if (olibrary == NULL) {
        return oc_error_invalid_param;
    }

#if defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION) && defined(ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION)
    FT_Error        ft_err;
    FT_Library      ft_library;
    HRESULT         result;
    IDWriteFactory* dw_factory;
    oc_library*     library = NULL;

    ft_err = FT_Init_FreeType(&ft_library);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(ft_err);
    }

    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&dw_factory);
    switch (result) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        FT_Done_FreeType(ft_library);
        return oc_error_out_of_memory;
    default:
        FT_Done_FreeType(ft_library);
        return oc__unexpected(result);
    }

    library = malloc(sizeof(*library));
    if (!library) {
        dw_factory->lpVtbl->Release(dw_factory);
        FT_Done_FreeType(ft_library);
        return oc_error_out_of_memory;
    }

    library->ft_library = ft_library;
    library->dw_factory = dw_factory;
    *olibrary = library;
    return oc_error_ok;

#elif defined(ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION)
    HRESULT         err;
    IDWriteFactory* dw_factory;

    err = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&dw_factory);
    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    err = dw_factory->lpVtbl->RegisterFontFileLoader(dw_factory, oc__dw_file_loader);
    if (err != S_OK) {
        dw_factory->lpVtbl->Release(dw_factory);
        return oc__unexpected(err);
    }

    *olibrary = (oc_library*)dw_factory;
    return oc_error_ok;

#elif defined(ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION)
    HRESULT         result;
    IDWriteFactory* dw_factory;

    result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, &IID_IDWriteFactory, (IUnknown**)&dw_factory);
    switch (result) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(result);
    }

    *olibrary = (oc_library*)dw_factory;
    return oc_error_ok;

#elif defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION)
    FT_Error   ft_err;
    FT_Library ft_library;

    ft_err = FT_Init_FreeType(&ft_library);
    switch (ft_err) {
    case FT_Err_Ok:
        break;
    case FT_Err_Out_Of_Memory:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(ft_err);
    }

    *olibrary = (oc_library*)ft_library;
    return oc_error_ok;

#else
    *olibrary = (oc_library*)&oc__noop_library;
    return oc_error_ok;
#endif
}

void oc_free_library(oc_library* library) {
    if (!library) {
        return;
    }

#if defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION) && defined(ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION)
    FT_Library      ft_library = library->ft_library;
    IDWriteFactory* dw_factory = library->dw_factory;

    dw_factory->lpVtbl->Release(dw_factory);
    FT_Done_FreeType(ft_library);
    free(library);

#elif defined(ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION)
    IDWriteFactory* dw_factory = (IDWriteFactory*)library;
    dw_factory->lpVtbl->UnregisterFontFileLoader(dw_factory, oc__dw_file_loader);
    dw_factory->lpVtbl->Release(dw_factory);

#elif defined(ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION)
    IDWriteFactory* dw_factory = (IDWriteFactory*)library;
    dw_factory->lpVtbl->Release(dw_factory);

#elif defined(ONECORE_FREETYPE_LOADER_IMPLEMENTATION)
    FT_Library ft_library = (FT_Library)library;
    FT_Done_FreeType(ft_library);

#else
    (void)library;
#endif
}
