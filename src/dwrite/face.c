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

typedef struct IOCSimplifiedGeometrySink {
    const ID2D1SimplifiedGeometrySinkVtbl* lpVtbl;
    const oc_outline_funcs* funcs;
    void* ctx;
    LONG ref_count;
} IOCSimplifiedGeometrySink;

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
    if (view.data == NULL) {
        return E_INVALIDARG;
    }

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

// todo: assert of ref_count not being -1
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

static HRESULT STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_Close(ID2D1SimplifiedGeometrySink* This) {
    (void)This;
    return S_OK;
}

static void STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_EndFigure(ID2D1SimplifiedGeometrySink* This, D2D1_FIGURE_END figureEnd) {
    (void)figureEnd;
    IOCSimplifiedGeometrySink* this = (IOCSimplifiedGeometrySink*)This;

    this->funcs->end_figure(this->ctx);
}

static void STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_AddBeziers(ID2D1SimplifiedGeometrySink* This, const D2D1_BEZIER_SEGMENT* beziers, UINT beziersCount) {
    IOCSimplifiedGeometrySink* this = (IOCSimplifiedGeometrySink*)This;

    oc_point points[3];
    for (UINT32 i = 0; i < beziersCount; i++) {
        points[0].x = beziers[i].point1.x;
        points[0].y = -beziers[i].point1.y;

        points[1].x = beziers[i].point2.x;
        points[1].y = -beziers[i].point2.y;

        points[2].x = beziers[i].point3.x;
        points[2].y = -beziers[i].point3.y;

        this->funcs->cubic_to(points[0], points[1], points[2], this->ctx);
    }
}

static void STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_AddLines(ID2D1SimplifiedGeometrySink* This, const D2D1_POINT_2F* points, UINT pointsCount) {
    IOCSimplifiedGeometrySink* this = (IOCSimplifiedGeometrySink*)This;

    oc_point point;
    for (UINT32 i = 0; i < pointsCount; i++) {
        point.x = points[i].x;
        point.y = -points[i].y;
        this->funcs->line_to(point, this->ctx);
    }
}

static void STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_BeginFigure(ID2D1SimplifiedGeometrySink* This, D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) {
    (void)figureBegin;
    IOCSimplifiedGeometrySink* this = (IOCSimplifiedGeometrySink*)This;

    oc_point point = { startPoint.x, -startPoint.y };
    this->funcs->start_figure(point, this->ctx);
}

static void STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_SetSegmentFlags(ID2D1SimplifiedGeometrySink* This, D2D1_PATH_SEGMENT vertexFlags) {
    (void)This;
    (void)vertexFlags;
}

static void STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_SetFillMode(ID2D1SimplifiedGeometrySink* This, D2D1_FILL_MODE fillMode) {
    (void)This;
    (void)fillMode;
};

static ULONG STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_Release(IUnknown* This) {
    IOCSimplifiedGeometrySink* this = (IOCSimplifiedGeometrySink*)This;
    return InterlockedDecrement(&this->ref_count);
}

static ULONG STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_AddRef(IUnknown* This) {
    IOCSimplifiedGeometrySink* this = (IOCSimplifiedGeometrySink*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader)) {
        IOCSimplifiedGeometrySink_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const ID2D1SimplifiedGeometrySinkVtbl IOCSimplifiedGeometrySinkVtbl = {
    { IOCSimplifiedGeometrySink_QueryInterface,
        IOCSimplifiedGeometrySink_AddRef,
        IOCSimplifiedGeometrySink_Release },
    IOCSimplifiedGeometrySink_SetFillMode,
    IOCSimplifiedGeometrySink_SetSegmentFlags,
    IOCSimplifiedGeometrySink_BeginFigure,
    IOCSimplifiedGeometrySink_AddLines,
    IOCSimplifiedGeometrySink_AddBeziers,
    IOCSimplifiedGeometrySink_EndFigure,
    IOCSimplifiedGeometrySink_Close,
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
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err;
    IDWriteFontFile* font_file;

    memory_view key = { data, size };

    // declared static to provide a single global instance
    // IOCFontFileLoader is thread-safe, so shared access is safe
    static IOCFontFileLoader ioc_font_file_loader = { &IOCFontFileLoaderVtbl, 0 };
    IDWriteFontFileLoader* font_file_loader = (IDWriteFontFileLoader*)&ioc_font_file_loader;

    // todo: move register code to init_library
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

void oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    IOCSimplifiedGeometrySink ioc_simplified_geometry_sink;
    ioc_simplified_geometry_sink.lpVtbl = &IOCSimplifiedGeometrySinkVtbl;
    ioc_simplified_geometry_sink.funcs = outline_funcs;
    ioc_simplified_geometry_sink.ref_count = 1;
    ioc_simplified_geometry_sink.ctx = context;

    IDWriteGeometrySink* geometry_sink = (IDWriteGeometrySink*)&ioc_simplified_geometry_sink;

    DWRITE_FONT_METRICS metrics;
    face.dw_font_face->lpVtbl->GetMetrics(face.dw_font_face, &metrics);

    // dwrite does not call line_to at the end to the beg
    HRESULT err = face.dw_font_face->lpVtbl->GetGlyphRunOutline(
        face.dw_font_face,
        metrics.designUnitsPerEm,
        &glyph_index,
        NULL,
        NULL,
        1,
        FALSE,
        FALSE,
        geometry_sink);

    if (err != S_OK) {
        printf("GetGlyphRunOutline failed: %ld\n", err);
    }

    ULONG refs = geometry_sink->lpVtbl->Base.Release((IUnknown*)geometry_sink);

    (void)refs;
    assert(refs == 0);
}

#endif // ONECORE_DWRITE
