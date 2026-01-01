#include <onecore.h>
#ifdef ONECORE_DWRITE

#include "../unexpected.h"
#include <assert.h>

typedef struct memory_view_s {
    const void* data;
    size_t size;
} memory_view;

typedef struct IOCFontFileStream {
    const IDWriteFontFileStreamVtbl* lpVtbl;
    LONG ref_count;
    memory_view memory_view;
} IOCFontFileStream;

typedef struct IOCFontFileLoader {
    const IDWriteFontFileLoaderVtbl* lpVtbl;
    LONG ref_count;
} IOCFontFileLoader;

static HRESULT STDMETHODCALLTYPE
IOCFontFileStream_GetLastWriteTime(IDWriteFontFileStream* This, UINT64* last_writetime) {
    (void)This;
    if (last_writetime == NULL) {
        return E_POINTER;
    }

    *last_writetime = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IOCFontFileStream_GetFileSize(IDWriteFontFileStream* This, UINT64* size) {
    IOCFontFileStream* this = (IOCFontFileStream*)This;

    if (size == NULL) {
        return E_POINTER;
    }

    *size = this->memory_view.size;
    return S_OK;
}

static void STDMETHODCALLTYPE
IOCFontFileStream_ReleaseFileFragment(IDWriteFontFileStream* This, void* fragment_context) {
    (void)This;
    (void)fragment_context;
}

static HRESULT STDMETHODCALLTYPE
IOCFontFileStream_ReadFileFragment(
    IDWriteFontFileStream* This,
    const void** fragment_start,
    UINT64 offset,
    UINT64 fragment_size,
    void** fragment_context) {

    IOCFontFileStream* this = (IOCFontFileStream*)This;

    if (fragment_start == NULL) {
        return E_POINTER;
    }
    *fragment_start = NULL;

    if (fragment_context == NULL) {
        return E_POINTER;
    }
    *fragment_context = NULL;

    if (offset > this->memory_view.size || fragment_size > this->memory_view.size - offset) {
        return E_FAIL;
    }

    *fragment_start = this->memory_view.data + offset;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
IOCFontFileStream_Release(IDWriteFontFileStream* This) {
    IOCFontFileStream* this = (IOCFontFileStream*)This;

    ULONG new_count = InterlockedDecrement(&this->ref_count);
    if (new_count == 0) {
        free(this);
    }
    return new_count;
}

static ULONG STDMETHODCALLTYPE
IOCFontFileStream_AddRef(IDWriteFontFileStream* This) {
    IOCFontFileStream* this = (IOCFontFileStream*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
IOCFontFileStream_QueryInterface(IDWriteFontFileStream* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileStream)) {
        IOCFontFileStream_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const IDWriteFontFileStreamVtbl IOCFontFileStreamVtbl = {
    IOCFontFileStream_QueryInterface,
    IOCFontFileStream_AddRef,
    IOCFontFileStream_Release,
    IOCFontFileStream_ReadFileFragment,
    IOCFontFileStream_ReleaseFileFragment,
    IOCFontFileStream_GetFileSize,
    IOCFontFileStream_GetLastWriteTime,
};

static HRESULT STDMETHODCALLTYPE
IOCFontFileLoader_CreateStreamFromKey(IDWriteFontFileLoader* This, const void* key, UINT32 key_size, IDWriteFontFileStream** stream) {
    (void)This;

    if (stream == NULL) {
        return E_POINTER;
    }
    *stream = NULL;

    if (key == NULL) {
        return E_POINTER;
    }

    if (key_size != sizeof(memory_view)) {
        return E_INVALIDARG;
    }

    memory_view view = *(const memory_view*)key;

    IOCFontFileStream* ioc_font_file_stream = malloc(sizeof(IOCFontFileStream));
    if (ioc_font_file_stream == NULL) {
        return E_OUTOFMEMORY;
    }

    ioc_font_file_stream->lpVtbl = &IOCFontFileStreamVtbl;
    ioc_font_file_stream->ref_count = 1;
    ioc_font_file_stream->memory_view = view;

    *stream = (IDWriteFontFileStream*)ioc_font_file_stream;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
IOCFontFileLoader_Release(IDWriteFontFileLoader* This) {
    IOCFontFileLoader* this = (IOCFontFileLoader*)This;
    return InterlockedDecrement(&this->ref_count);
}

static ULONG STDMETHODCALLTYPE
IOCFontFileLoader_AddRef(IDWriteFontFileLoader* This) {
    IOCFontFileStream* this = (IOCFontFileStream*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
IOCFontFileLoader_QueryInterface(IDWriteFontFileLoader* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader)) {
        IOCFontFileLoader_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const IDWriteFontFileLoaderVtbl IOCFontFileLoaderVtbl = {
    IOCFontFileLoader_QueryInterface,
    IOCFontFileLoader_AddRef,
    IOCFontFileLoader_Release,
    IOCFontFileLoader_CreateStreamFromKey
};


static oc_error open_face_from_font_file(oc_library library, IDWriteFontFile* font_file, long face_index, oc_face* pface) {
    HRESULT err;
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

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    if (!is_supported_fonttype) {
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

oc_error oc_open_face(oc_library library, const char* path, long face_index, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (path == NULL) {
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

    (void)ok;
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
    case DWRITE_E_FILENOTFOUND:
        return oc_error_failed_to_open;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    oc_error result = open_face_from_font_file(library, font_file, face_index, pface);

    font_file->lpVtbl->Release(font_file);
    return result;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, long face_index, oc_face* pface) {
    (void)pface;
    (void)face_index;

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err;
    IDWriteFontFile* font_file;

    memory_view key = { data, size };

    // If someone will ever move IOCFontFileLoader to the heap do not forget to add all releases
    IOCFontFileLoader ioc_font_file_loader = { &IOCFontFileLoaderVtbl, 1 };
    IDWriteFontFileLoader* font_file_loader = (IDWriteFontFileLoader*)&ioc_font_file_loader;

    err = library.dw_factory->lpVtbl->RegisterFontFileLoader(library.dw_factory, font_file_loader);
    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    err = library.dw_factory->lpVtbl->CreateCustomFontFileReference(
        library.dw_factory,
        &key,
        sizeof(memory_view),
        font_file_loader,
        &font_file);

    library.dw_factory->lpVtbl->UnregisterFontFileLoader(library.dw_factory, font_file_loader);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    oc_error result = open_face_from_font_file(library, font_file, face_index, pface);

    font_file->lpVtbl->Release(font_file);
    assert(font_file_loader->lpVtbl->Release(font_file_loader) == 0);

    return result;
}

void oc_free_face(oc_face face) {
    face.dw_font_face->lpVtbl->Release(face.dw_font_face);
}

uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    UINT16 index;

    HRESULT err = face.dw_font_face->lpVtbl->GetGlyphIndices(
        face.dw_font_face,
        &charcode,
        1,
        &index);

    (void)err;
    assert(err == S_OK);

    return index;
}

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable) {
    if (ptable == NULL) {
        return oc_error_invalid_param;
    }

    const void* table_data;
    UINT32 table_size;

    void* context;
    WINBOOL exists;

    HRESULT err = face.dw_font_face->lpVtbl->TryGetFontTable(
        face.dw_font_face,
        _byteswap_ulong(tag), // swapping bytes because windows table tags are little-endian
        &table_data,
        &table_size,
        &context,
        &exists);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    if (exists == FALSE) {
        return oc_error_table_missing;
    }

    oc_table table;
    table.data = table_data;
    table.size = table_size;
    table.__handle = context;

    *ptable = table;
    return oc_error_ok;
}

inline void oc_free_table(oc_face face, oc_table table) {
    face.dw_font_face->lpVtbl->ReleaseFontTable(face.dw_font_face, table.__handle);
}

void oc_get_metrics(oc_face face, oc_metrics* pmetrics) {
    DWRITE_FONT_METRICS metrics;
    face.dw_font_face->lpVtbl->GetMetrics(face.dw_font_face, &metrics);

    pmetrics->units_per_em = metrics.designUnitsPerEm;
    pmetrics->ascent = metrics.ascent;
    pmetrics->descent = metrics.descent;
    pmetrics->leading = metrics.lineGap;
    pmetrics->underline_position = metrics.underlinePosition;
    pmetrics->underline_thickness = metrics.underlineThickness;
}

bool oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_glyph_metrics* pglyph_metrics) {
    if (pglyph_metrics == NULL) {
        return false;
    }

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    UINT16 glyph_count = face.dw_font_face->lpVtbl->GetGlyphCount(face.dw_font_face);
    if (glyph_index >= glyph_count) {
        return false;
    }

    DWRITE_GLYPH_METRICS metrics;
    HRESULT err = face.dw_font_face->lpVtbl->GetDesignGlyphMetrics(
        face.dw_font_face,
        &glyph_index,
        1,
        &metrics,
        FALSE);

    (void)err;
    assert(err == S_OK);

    pglyph_metrics->width = metrics.advanceWidth - metrics.leftSideBearing - metrics.rightSideBearing;
    pglyph_metrics->height = metrics.advanceHeight - metrics.topSideBearing - metrics.bottomSideBearing;
    pglyph_metrics->bearing_x = metrics.leftSideBearing;
    pglyph_metrics->bearing_y = metrics.verticalOriginY - metrics.topSideBearing;
    pglyph_metrics->advance = metrics.advanceWidth;

    return true;
}

#endif // ONECORE_DWRITE
