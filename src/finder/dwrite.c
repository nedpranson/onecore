#define ONECORE_IMPLEMENTATION
#include "onecore.h"

extern oc_error oc__init_face(IDWriteFactory* dw_factory, IDWriteFontFace* dw_face, oc_26p6 desired_size, uint16_t dpi, oc_face* oface);

/* ONECORE_DIRECTWRITE_FINDER_IMPLEMENTATION */
#include <initguid.h>

#include <dwrite.h>

struct oc_collection_impl {
    oc_library* oc_library;
    char**      families;
    UINT32      nfamilies;
};

typedef struct {
    IDWriteFactory* dw_factory;
    IDWriteFont*    dw_font;
    oc_font         font;
} oc__font_impl;

#ifndef OC__OVERRIDE_LIBRARY_IMPL
#define OC__OVERRIDE_LIBRARY_IMPL

oc_error oc_init_library(oc_library** olibrary) {
    HRESULT         result;
    IDWriteFactory* dw_factory;

    if (olibrary == NULL) {
        return oc_error_invalid_param;
    }

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
}

void oc_free_library(oc_library* library) {
    IDWriteFactory* dw_factory;

    if (!library) {
        return;
    }

    dw_factory = (IDWriteFactory*)library;
    dw_factory->lpVtbl->Release(dw_factory);
}
#endif

static inline void oc__free_font(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    impl->dw_font->lpVtbl->Release(impl->dw_font);
    free(impl);
}

oc_error ocf_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_collection collection = { 0 };
    oc_error      err = oc_error_ok;

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }

    collection.impl = calloc(1, sizeof(oc_collection_impl));
    if (collection.impl == NULL) {
        return oc_error_out_of_memory;
    }

    collection.impl->oc_library = library;
exit:
    if (ocollection)
        *ocollection = collection;
    return err;
}

void ocf_free_collection(oc_collection* collection) {
    if (collection) {
        while (collection->nfonts--) {
            oc__free_font(collection->fonts[collection->nfonts]);
        }

        while (collection->impl->nfamilies--) {
            free(collection->impl->families[collection->impl->nfamilies]);
        }

        free(collection->fonts);
        free(collection->impl->families);
        free(collection->impl);

        memset(collection, 0, sizeof(*collection));
    }
}

static const oc_slant oc__slant_map[] = {
    [DWRITE_FONT_STYLE_NORMAL] = oc_slant_roman,
    [DWRITE_FONT_STYLE_OBLIQUE] = oc_slant_oblique,
    [DWRITE_FONT_STYLE_ITALIC] = oc_slant_italic,
};

static oc_font* oc__init_font(IDWriteFactory* dw_factory, IDWriteFont* dw_font, const char* family) {
    DWRITE_FONT_WEIGHT weight;
    DWRITE_FONT_STYLE  style;

    oc__font_impl* impl;

    weight = dw_font->lpVtbl->GetWeight(dw_font);
    style = dw_font->lpVtbl->GetStyle(dw_font);

    impl = malloc(sizeof(*impl));
    if (impl == NULL) {
        return NULL;
    }

    impl->dw_factory = dw_factory;
    impl->dw_font = dw_font;
    impl->font.family = family;
    impl->font.weight = (uint16_t)weight;
    impl->font.slant = oc__slant_map[style];

    return &impl->font;
}

oc_error ocf_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;
    HRESULT  hr;

    oc_library oc_library;

    IDWriteFactory*        dw_factory;
    IDWriteFontCollection* dw_collection = NULL;

    UINT32 family_count;
    UINT32 font_count;

    union {
        char*  str;
        UINT32 len;
    }*     families = NULL;
    UINT32 nfamilies = 0;

    WCHAR* wide_buf = NULL;
    UINT32 wide_buf_len;

    oc_font** fonts = NULL;
    uint32_t  nfonts = 0;

    oc_collection      tmp_collection;
    oc_collection_impl tmp_impl;

    if (!collection) {
        oc__exit(oc_error_invalid_param);
    }

    oc_library = collection->impl->oc_library;
#ifdef ONECORE_FREETYPE_LOADER_IMPLEMENTATION
    dw_factory = oc_library->dw_factory;
#else
    dw_factory = (IDWriteFactory*)oc_library;
#endif
    hr = dw_factory->lpVtbl->GetSystemFontCollection(dw_factory, &dw_collection, TRUE);

    switch (hr) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(hr));
    }

    family_count = dw_collection->lpVtbl->GetFontFamilyCount(dw_collection);
    if (family_count == 0) {
        goto done;
    }

    font_count = 0;
    families = malloc(family_count * sizeof(*families));

    if (families == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    for (UINT32 i = 0; i < family_count; i++) {
        IDWriteFontFamily*       family;
        IDWriteLocalizedStrings* names;
        UINT32                   length;

        hr = dw_collection->lpVtbl->GetFontFamily(dw_collection, i, &family);
        assert(hr == S_OK);

        hr = family->lpVtbl->GetFamilyNames(family, &names);
        assert(hr == S_OK);

        hr = names->lpVtbl->GetStringLength(names, 0, &length);
        assert(hr == S_OK);

        families[i].len = length;
        wide_buf_len = OC__MAX(wide_buf_len, length);
        font_count += family->lpVtbl->GetFontCount(family);

        names->lpVtbl->Release(names);
        family->lpVtbl->Release(family);
    }

    assert(font_count > 0);

    wide_buf = malloc((wide_buf_len + 1) * sizeof(WCHAR));
    if (wide_buf == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    fonts = malloc(sizeof(*fonts) * font_count);
    if (fonts == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    for (UINT32 i = 0; i < family_count; i++) {
        IDWriteFontFamily*       font_family;
        IDWriteLocalizedStrings* names;

        UINT32 wide_length = families[i].len;
        UINT32 font_index;

        char* family;
        int   length;

        hr = dw_collection->lpVtbl->GetFontFamily(dw_collection, i, &font_family);
        assert(hr == S_OK);

        hr = font_family->lpVtbl->GetFamilyNames(font_family, &names);
        assert(hr == S_OK);

        hr = names->lpVtbl->GetString(names, 0, wide_buf, wide_length + 1);
        names->lpVtbl->Release(names);
        assert(hr == S_OK);

        length = WideCharToMultiByte(
            CP_UTF8,
            0,
            wide_buf,
            wide_length,
            NULL,
            0,
            NULL,
            NULL);

        assert(length > 0);

        family = malloc(length + 1);
        if (family == NULL) {
            font_family->lpVtbl->Release(font_family);
            oc__exit(oc_error_out_of_memory);
        }

        length = WideCharToMultiByte(
            CP_UTF8,
            0,
            wide_buf,
            wide_length,
            family,
            length,
            NULL,
            NULL);

        assert(length > 0);

        family[length] = '\0';
        families[nfamilies++].str = family;
        font_index = font_family->lpVtbl->GetFontCount(font_family);

        while (font_index--) {
            IDWriteFont* dw_font;
            oc_font*     font;

            hr = font_family->lpVtbl->GetFont(font_family, font_index, &dw_font);
            assert(hr == S_OK);

            font = oc__init_font(dw_factory, dw_font, family);
            if (font == NULL) {
                font_family->lpVtbl->Release(font_family);
                oc__exit(oc_error_out_of_memory);
            }

            fonts[nfonts++] = font;
        }

        font_family->lpVtbl->Release(font_family);
    }
done:
    tmp_impl.oc_library = oc_library;
    tmp_impl.families = (char**)families;
    tmp_impl.nfamilies = nfamilies;

    tmp_collection.impl = collection->impl;
    tmp_collection.fonts = fonts;
    tmp_collection.nfonts = nfonts;

    families = (void*)collection->impl->families;
    nfamilies = collection->impl->nfamilies;
    fonts = collection->fonts;
    nfonts = collection->nfonts;

    *collection->impl = tmp_impl;
    *collection = tmp_collection;
exit:
    while (nfonts--)
        oc__free_font(fonts[nfonts]);
    while (nfamilies--)
        free(families[nfamilies].str);

    if (dw_collection)
        dw_collection->lpVtbl->Release(dw_collection);

    free(fonts);
    free(families);
    free(wide_buf);

    return err;
}

bool ocf_has_character(const oc_font* font, uint32_t character) {
    oc__font_impl* impl;
    IDWriteFont*   dw_font;

    HRESULT result;
    WINBOOL exists;

    if (!font) {
        return false;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    dw_font = impl->dw_font;

    result = dw_font->lpVtbl->HasCharacter(dw_font, character, &exists);
    return result == S_OK && exists;
}

#ifdef ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION
// todo: add support for freetype
oc_error ocf_open_font(const oc_font* font, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    oc_error err;
    HRESULT  result;

    oc__font_impl*   impl;
    IDWriteFontFace* dw_face;

    oc_face face = { 0 };

    if (!font) {
        return oc_error_invalid_param;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    result = impl->dw_font->lpVtbl->CreateFontFace(impl->dw_font, &dw_face);

    switch (result) {
    case S_OK:
        break;
    default:
        oc__exit(oc__unexpected(result));
    }

    if (desired_size == 0) {
        desired_size = 12 << 6;
    } else if (desired_size < 1 << 6) {
        desired_size = 1 << 6;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    err = oc__init_face(impl->dw_factory, dw_face, desired_size, dpi, &face);
exit:
    *oface = face;
    return err;
}
#endif

size_t ocf_copy_path(const oc_font* font, char* buf, size_t len) {
    oc__font_impl* impl;
    HRESULT        result;

    IDWriteFontFace* face;
    IDWriteFontFile* file;

    UINT32 nfiles;

    const void* key;
    UINT32      key_size;

    IDWriteFontFileLoader*      loader;
    IDWriteLocalFontFileLoader* local_loader;

    WCHAR* wide_path;
    UINT32 wide_len;

    int    path_len;
    size_t copy_len;

    if (!font) {
        return 0;
    }

    impl = oc__parentof(oc__font_impl, font, font);
    result = impl->dw_font->lpVtbl->CreateFontFace(impl->dw_font, &face);

    if (result != S_OK) {
        return 0;
    }

    nfiles = 1;
    result = face->lpVtbl->GetFiles(face, &nfiles, &file);
    face->lpVtbl->Release(face);

    if (result != S_OK || nfiles == 0) {
        return 0;
    }

    result = file->lpVtbl->GetReferenceKey(file, &key, &key_size);
    assert(result == S_OK);

    result = file->lpVtbl->GetLoader(file, &loader);
    file->lpVtbl->Release(file);

    assert(result == S_OK);

    result = loader->lpVtbl->QueryInterface(
        loader,
        &IID_IDWriteLocalFontFileLoader,
        (void**)&local_loader);
    loader->lpVtbl->Release(loader);

    if (result != S_OK) {
        return 0;
    }

    result = local_loader->lpVtbl->GetFilePathLengthFromKey(
        local_loader,
        key,
        key_size,
        &wide_len);

    assert(result == S_OK);
    if (wide_len == 0) {
        local_loader->lpVtbl->Release(local_loader);
        return 0;
    }

    wide_path = malloc((wide_len + 1) * sizeof(WCHAR));
    if (wide_path == NULL) {
        local_loader->lpVtbl->Release(local_loader);
        return 0;
    }

    result = local_loader->lpVtbl->GetFilePathFromKey(
        local_loader,
        key,
        key_size,
        wide_path,
        wide_len + 1);

    local_loader->lpVtbl->Release(local_loader);
    assert(result == S_OK);

    path_len = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide_path,
        wide_len,
        NULL,
        0,
        NULL,
        NULL);

    assert(path_len > 0);
    if (len == 0) {
        free(wide_path);
        return path_len;
    }

    copy_len = len < (size_t)path_len ? len : (size_t)path_len;
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide_path,
        wide_len,
        buf,
        (int)copy_len,
        NULL,
        NULL);

    free(wide_path);
    return copy_len;
}
