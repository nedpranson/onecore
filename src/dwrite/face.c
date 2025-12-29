#include <onecore.h>
#ifdef ONECORE_DWRITE

#include "../unexpected.h"
#include <assert.h>

oc_error oc_face_new(oc_library library, const char* path, long face_index, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0) - 1;
    if (wlen == -1) {
        return oc_error_failed_to_open;
    }

    if (wlen == 0) {
        return oc_error_failed_to_open;
    }

    wchar_t* wpath = (wchar_t*)malloc((wlen + 1) * sizeof(wchar_t));
    if (wpath == NULL) {
        return oc_error_out_of_memory;
    }

    int ok = MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, wlen + 1);
    assert(ok != 0);

    IDWriteFontFile* font_file;
    err = library.dw_factory->lpVtbl->CreateFontFileReference(
        library.dw_factory,
        wpath,
        NULL,
        &font_file);
    free(wpath);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    WINBOOL is_supported_fonttype;

    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;

    UINT32 face_num;

    err = font_file->lpVtbl->Analyze(
        font_file,
        &is_supported_fonttype,
        &file_type,
        &face_type,
        &face_num);

    if (err != S_OK) {
        font_file->lpVtbl->Release(font_file);
        switch (err) {
        case E_OUTOFMEMORY:
            return oc_error_out_of_memory;
        default:
            return unexpected(err);
        }
    }

    if (!is_supported_fonttype) {
        font_file->lpVtbl->Release(font_file);
        return oc_error_failed_to_open;
    }

    // todo: we should handle simulations
    err = library.dw_factory->lpVtbl->CreateFontFace(
        library.dw_factory,
        face_type,
        1,
        &font_file,
        face_index,
        DWRITE_FONT_SIMULATIONS_NONE,
        &pface->dw_font_face);
    font_file->lpVtbl->Release(font_file);

    switch (err) {
    case S_OK:
        return oc_error_ok;
    case E_INVALIDARG:
        return oc_error_invalid_param;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }
}

void oc_face_free(oc_face face) {
    face.dw_font_face->lpVtbl->Release(face.dw_font_face);
}

#endif // ONECORE_DWRITE
