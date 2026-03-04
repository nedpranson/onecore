#define ONECORE_IMPLEMENTATION
#include "onecore.h"

/* ONECORE_DIRECTWRITE_IMPLEMENTATION */
#include <assert.h>
#include <initguid.h>

#include <d2d1.h>
#include <dwrite.h>

struct oc_face_impl {
    IDWriteFontFace* dw_face;
    IDWriteFactory* dw_factory;
};

typedef struct {
    const void* data;
    size_t size;
} oc__memory_view;

typedef struct {
    const IDWriteFontFileStreamVtbl* lpVtbl;
    LONG ref_count;
    oc__memory_view memory_view;
} OC__IDWriteFontFileStream;

typedef struct {
    const IDWriteFontFileLoaderVtbl* lpVtbl;
    LONG ref_count;
} OC__IDWriteFontFileLoader;

typedef struct {
    const ID2D1SimplifiedGeometrySinkVtbl* lpVtbl;
    const oc_outline_funcs* funcs;
    D2D1_POINT_2F start;
    D2D1_POINT_2F origin;
    void* ctx;
    LONG ref_count;
} OC__ID2D1SimplifiedGeometrySink;

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileStream_GetLastWriteTime(IDWriteFontFileStream* This, UINT64* last_writetime) {
    (void)This;
    if (last_writetime == NULL) {
        return E_POINTER;
    }

    *last_writetime = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileStream_GetFileSize(IDWriteFontFileStream* This, UINT64* size) {
    OC__IDWriteFontFileStream* this = (OC__IDWriteFontFileStream*)This;

    if (size == NULL) {
        return E_POINTER;
    }

    *size = this->memory_view.size;
    return S_OK;
}

static void STDMETHODCALLTYPE
OC__IDWriteFontFileStream_ReleaseFileFragment(IDWriteFontFileStream* This, void* fragment_context) {
    (void)This;
    (void)fragment_context;
}

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileStream_ReadFileFragment(
    IDWriteFontFileStream* This,
    const void** fragment_start,
    UINT64 offset,
    UINT64 fragment_size,
    void** fragment_context) {

    OC__IDWriteFontFileStream* this = (OC__IDWriteFontFileStream*)This;

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
OC__IDWriteFontFileStream_Release(IDWriteFontFileStream* This) {
    OC__IDWriteFontFileStream* this = (OC__IDWriteFontFileStream*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    if (refs == 0) {
        free(this);
    }

    assert(refs != -1);
    return refs;
}

static ULONG STDMETHODCALLTYPE
OC__IDWriteFontFileStream_AddRef(IDWriteFontFileStream* This) {
    OC__IDWriteFontFileStream* this = (OC__IDWriteFontFileStream*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileStream_QueryInterface(IDWriteFontFileStream* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileStream)) {
        OC__IDWriteFontFileStream_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const IDWriteFontFileStreamVtbl OC__IDWriteFontFileStreamVtbl = {
    OC__IDWriteFontFileStream_QueryInterface,
    OC__IDWriteFontFileStream_AddRef,
    OC__IDWriteFontFileStream_Release,
    OC__IDWriteFontFileStream_ReadFileFragment,
    OC__IDWriteFontFileStream_ReleaseFileFragment,
    OC__IDWriteFontFileStream_GetFileSize,
    OC__IDWriteFontFileStream_GetLastWriteTime,
};

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileLoader_CreateStreamFromKey(IDWriteFontFileLoader* This, const void* key, UINT32 key_size, IDWriteFontFileStream** stream) {
    (void)This;

    if (stream == NULL) {
        return E_POINTER;
    }
    *stream = NULL;

    if (key == NULL) {
        return E_POINTER;
    }

    if (key_size != sizeof(oc__memory_view)) {
        return E_INVALIDARG;
    }

    oc__memory_view view = *(const oc__memory_view*)key;
    if (view.data == NULL) {
        return E_INVALIDARG;
    }

    OC__IDWriteFontFileStream* file_stream = malloc(sizeof(OC__IDWriteFontFileStream));
    if (file_stream == NULL) {
        return E_OUTOFMEMORY;
    }

    file_stream->lpVtbl = &OC__IDWriteFontFileStreamVtbl;
    file_stream->ref_count = 1;
    file_stream->memory_view = view;

    *stream = (IDWriteFontFileStream*)file_stream;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
OC__IDWriteFontFileLoader_Release(IDWriteFontFileLoader* This) {
    OC__IDWriteFontFileLoader* this = (OC__IDWriteFontFileLoader*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    assert(refs != -1);
    return refs;
}

static ULONG STDMETHODCALLTYPE
OC__IDWriteFontFileLoader_AddRef(IDWriteFontFileLoader* This) {
    OC__IDWriteFontFileLoader* this = (OC__IDWriteFontFileLoader*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
OC__IDWriteFontFileLoader_QueryInterface(IDWriteFontFileLoader* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader)) {
        OC__IDWriteFontFileLoader_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const IDWriteFontFileLoaderVtbl OC__IDWriteFontFileLoaderVtbl = {
    OC__IDWriteFontFileLoader_QueryInterface,
    OC__IDWriteFontFileLoader_AddRef,
    OC__IDWriteFontFileLoader_Release,
    OC__IDWriteFontFileLoader_CreateStreamFromKey
};

static HRESULT STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_Close(ID2D1SimplifiedGeometrySink* This) {
    (void)This;
    return S_OK;
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_EndFigure(ID2D1SimplifiedGeometrySink* This, D2D1_FIGURE_END figureEnd) {
    (void)figureEnd;
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    if (this->origin.x != this->start.x || this->origin.y != this->start.y) {
        oc_point point = { this->start.x, -this->start.y };
        this->funcs->line_to(point, this->ctx);
    }

    this->funcs->end_figure(this->ctx);
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_AddBeziers(ID2D1SimplifiedGeometrySink* This, const D2D1_BEZIER_SEGMENT* beziers, UINT beziersCount) {
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

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

    assert(beziersCount > 0);
    this->origin = beziers[beziersCount - 1].point3;
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_AddLines(ID2D1SimplifiedGeometrySink* This, const D2D1_POINT_2F* points, UINT pointsCount) {
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    oc_point point;
    for (UINT32 i = 0; i < pointsCount; i++) {
        point.x = points[i].x;
        point.y = -points[i].y;
        this->funcs->line_to(point, this->ctx);
    }

    assert(pointsCount > 0);
    this->origin = points[pointsCount - 1];
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_BeginFigure(ID2D1SimplifiedGeometrySink* This, D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) {
    (void)figureBegin;
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    oc_point point = { startPoint.x, -startPoint.y };
    this->funcs->start_figure(point, this->ctx);
    this->start = startPoint;
    this->origin = startPoint;
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_SetSegmentFlags(ID2D1SimplifiedGeometrySink* This, D2D1_PATH_SEGMENT vertexFlags) {
    (void)This;
    (void)vertexFlags;
}

static void STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_SetFillMode(ID2D1SimplifiedGeometrySink* This, D2D1_FILL_MODE fillMode) {
    (void)This;
    (void)fillMode;
};

static ULONG STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_Release(IUnknown* This) {
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    assert(refs != -1);
    return refs;
}

static ULONG STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_AddRef(IUnknown* This) {
    OC__ID2D1SimplifiedGeometrySink* this = (OC__ID2D1SimplifiedGeometrySink*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
OC__ID2D1SimplifiedGeometrySink_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader)) {
        OC__ID2D1SimplifiedGeometrySink_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const ID2D1SimplifiedGeometrySinkVtbl OC__ID2D1SimplifiedGeometrySinkVtbl = {
    { OC__ID2D1SimplifiedGeometrySink_QueryInterface,
        OC__ID2D1SimplifiedGeometrySink_AddRef,
        OC__ID2D1SimplifiedGeometrySink_Release },
    OC__ID2D1SimplifiedGeometrySink_SetFillMode,
    OC__ID2D1SimplifiedGeometrySink_SetSegmentFlags,
    OC__ID2D1SimplifiedGeometrySink_BeginFigure,
    OC__ID2D1SimplifiedGeometrySink_AddLines,
    OC__ID2D1SimplifiedGeometrySink_AddBeziers,
    OC__ID2D1SimplifiedGeometrySink_EndFigure,
    OC__ID2D1SimplifiedGeometrySink_Close,
};

OC__IDWriteFontFileLoader oc__file_loader = { &OC__IDWriteFontFileLoaderVtbl, 0 };
IDWriteFontFileLoader* oc__dw_file_loader = (IDWriteFontFileLoader*)&oc__file_loader;

oc_error oc_init_library(oc_library* plibrary) {
    if (plibrary == NULL) {
        return oc_error_invalid_param;
    }

    IDWriteFactory* dw_factory;
    HRESULT err;

    err = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_ISOLATED,
        &IID_IDWriteFactory,
        (IUnknown**)&dw_factory);

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
        // DWRITE_E_ALREADYREGISTERED;
        dw_factory->lpVtbl->Release(dw_factory);
        return oc__unexpected(err);
    }

    plibrary->internals = dw_factory;
    return oc_error_ok;
}

void oc_free_library(oc_library library) {
    IDWriteFactory* dw_factory = (IDWriteFactory*)library.internals;

    dw_factory->lpVtbl->UnregisterFontFileLoader(dw_factory, oc__dw_file_loader);
    dw_factory->lpVtbl->Release(dw_factory);
}

static oc_error oc__open_face_from_font_file(oc_library library, IDWriteFontFile* font_file, const oc_open_params* pparams, oc_face* pface) {
    HRESULT err;
    WINBOOL is_supported_fonttype;
    IDWriteFactory* dw_factory;

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
        return oc__unexpected(err);
    }

    if (!is_supported_fonttype) {
        return oc_error_failed_to_open;
    }

    IDWriteFontFace* dw_font_face;
    oc_open_params params = oc__open_params_defaults(pparams);

    dw_factory = (IDWriteFactory*)library.internals;

    // todo: we should handle simulations
    err = dw_factory->lpVtbl->CreateFontFace(
        dw_factory,
        face_type,
        1,
        &font_file,
        params.face_index,
        DWRITE_FONT_SIMULATIONS_NONE,
        &dw_font_face);

    switch (err) {
    case S_OK:
        break;
    case E_INVALIDARG:
        return oc_error_invalid_param;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    oc_face_impl* impl = malloc(sizeof(oc_face_impl));
    if (impl == NULL) {
        dw_font_face->lpVtbl->Release(dw_font_face);
        return oc_error_out_of_memory;
    }

    impl->dw_face = dw_font_face;
    impl->dw_factory = dw_factory;

    DWRITE_FONT_METRICS metrics;
    dw_font_face->lpVtbl->GetMetrics(dw_font_face, &metrics);

    // todo: and make that point has to atleast 1.0

    // https://github.com/freetype/freetype/blob/85c8efe0afa5ad0df35114e317a065f544943c52/include/freetype/internal/ftobjs.h#L665
    oc_16p16 scaled = (params.desired_size * params.dpi + 36) / 72;
    oc_16p16 scale = oc_div_16p16(scaled, metrics.designUnitsPerEm);

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L3368
    int32_t ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        // todo: add this test case
        return oc_error_invalid_param;
    }

    pface->impl = impl;
    pface->metrics.upem = metrics.designUnitsPerEm;
    pface->metrics.ppem = (uint16_t)ppem;
    pface->metrics.scale = scale;
    pface->metrics.ascent = metrics.ascent;
    pface->metrics.descent = metrics.descent;
    pface->metrics.leading = metrics.lineGap;
    pface->metrics.underline_position = metrics.underlinePosition;
    pface->metrics.underline_thickness = metrics.underlineThickness;

    return oc_error_ok;
}

oc_error oc_open_face(oc_library library, const char* path, const oc_open_params* pparams, oc_face* pface) {
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
    IDWriteFactory* dw_factory = library.internals;

    err = dw_factory->lpVtbl->CreateFontFileReference(
        dw_factory,
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
        return oc__unexpected(err);
    }

    oc_error result = oc__open_face_from_font_file(library, font_file, pparams, pface);
    font_file->lpVtbl->Release(font_file);

    return result;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, const oc_open_params* pparams, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err;
    IDWriteFontFile* font_file;
    IDWriteFactory* dw_factory = library.internals;

    oc__memory_view key = { data, size };

    err = dw_factory->lpVtbl->CreateCustomFontFileReference(
        dw_factory,
        &key,
        sizeof(key),
        oc__dw_file_loader,
        &font_file);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    oc_error result = oc__open_face_from_font_file(library, font_file, pparams, pface);
    font_file->lpVtbl->Release(font_file);

    return result;
}

void oc_free_face(oc_face face) {
    face.impl->dw_face->lpVtbl->Release(face.impl->dw_face);
    free(face.impl);
}

uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    UINT16 index;
    IDWriteFontFace* dw_face = face.impl->dw_face;

    HRESULT err = dw_face->lpVtbl->GetGlyphIndices(
        dw_face,
        &charcode,
        1,
        &index);
    (void)err;

    assert(err == S_OK);
    return index;
}

oc_error oc_get_sfnt_table(oc_face face, oc_tag tag, oc_table* ptable, void** pcontext) {
    if (ptable == NULL || pcontext == NULL) {
        return oc_error_invalid_param;
    }

    const void* table_data;
    UINT32 table_size;

    void* context;
    WINBOOL exists;

    HRESULT err = face.impl->dw_face->lpVtbl->TryGetFontTable(
        face.impl->dw_face,
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
        return oc__unexpected(err);
    }

    if (exists == FALSE) {
        return oc_error_table_missing;
    }

    oc_table table;
    table.data = table_data;
    table.size = table_size;

    *ptable = table;
    *pcontext = context;

    return oc_error_ok;
}

inline void oc_free_table(oc_face face, void* context) {
    face.impl->dw_face->lpVtbl->ReleaseFontTable(face.impl->dw_face, context);
}

// static void fit_metrics(oc_glyph_metrics* pmetrics) {
//     oc_26p6 right = OC_26P6_CEIL(OC_26P6_ADD(pmetrics->bearing_x, pmetrics->width));
//     oc_26p6 bottom = OC_26P6_FLOOR(OC_26P6_SUB(pmetrics->bearing_y, pmetrics->height));
//
//     pmetrics->bearing_x = OC_26P6_FLOOR(pmetrics->bearing_x);
//     pmetrics->bearing_y = OC_26P6_CEIL(pmetrics->bearing_y);
//
//     pmetrics->width = OC_26P6_SUB(right, pmetrics->bearing_x);
//     pmetrics->height = OC_26P6_SUB(pmetrics->bearing_y, bottom);
//
//     pmetrics->advance = OC_26P6_ROUND(pmetrics->advance);
// }

void oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_glyph_metrics* pmetrics) {
    if (pmetrics == NULL) {
        return;
    }

    oc_glyph_metrics metrics;
    DWRITE_GLYPH_METRICS dw_metrics;
    IDWriteFontFace* dw_face = face.impl->dw_face;

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    UINT16 glyph_count = dw_face->lpVtbl->GetGlyphCount(dw_face);
    if (glyph_index >= glyph_count) {
        memset(pmetrics, 0, sizeof(oc_glyph_metrics));
        return;
    }

    HRESULT err = dw_face->lpVtbl->GetDesignGlyphMetrics(
        dw_face,
        &glyph_index,
        1,
        &dw_metrics,
        FALSE);
    (void)err;
    assert(err == S_OK);

    metrics.width = (INT32)dw_metrics.advanceWidth - dw_metrics.leftSideBearing - dw_metrics.rightSideBearing;
    metrics.height = (INT32)dw_metrics.advanceHeight - dw_metrics.topSideBearing - dw_metrics.bottomSideBearing;
    metrics.bearing_x = dw_metrics.leftSideBearing;
    metrics.bearing_y = dw_metrics.verticalOriginY - dw_metrics.topSideBearing;
    metrics.advance = dw_metrics.advanceWidth;

    if (flags & OC_LOAD_NO_SCALE) {
        goto done;
    }

    metrics.width = oc_mul_16p16(metrics.width, face.metrics.scale);
    metrics.height = oc_mul_16p16(metrics.height, face.metrics.scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, face.metrics.scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, face.metrics.scale);
    metrics.advance = oc_mul_16p16(metrics.advance, face.metrics.scale);

    // if (flags & OC_LOAD_NO_HINTING) {
    // goto done;
    //}

    // fit_metrics(&metrics);

done:
    *pmetrics = metrics;
}

void oc_get_glyph_bbox(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_bbox* pbbox) {
    if (pbbox == NULL) {
        return;
    }

    oc_bbox bbox;
    DWRITE_GLYPH_METRICS metrics;
    IDWriteFontFace* dw_face = face.impl->dw_face;

    UINT16 glyph_count = dw_face->lpVtbl->GetGlyphCount(dw_face);
    if (glyph_index >= glyph_count) {
        memset(pbbox, 0, sizeof(oc_bbox));
        return;
    }

    HRESULT err = dw_face->lpVtbl->GetDesignGlyphMetrics(
        dw_face,
        &glyph_index,
        1,
        &metrics,
        FALSE);
    (void)err;
    assert(err == S_OK);

    bbox.min_x = metrics.leftSideBearing;
    bbox.min_y = metrics.verticalOriginY + metrics.bottomSideBearing - (INT32)metrics.advanceHeight;
    bbox.max_x = metrics.advanceWidth - metrics.rightSideBearing;
    bbox.max_y = metrics.verticalOriginY - metrics.topSideBearing;

    if (flags & OC_LOAD_NO_SCALE) {
        goto done;
    }

    bbox.min_x = oc_mul_16p16(bbox.min_x, face.metrics.scale);
    bbox.min_y = oc_mul_16p16(bbox.min_y, face.metrics.scale);
    bbox.max_x = oc_mul_16p16(bbox.max_x, face.metrics.scale);
    bbox.max_y = oc_mul_16p16(bbox.max_y, face.metrics.scale);

done:
    *pbbox = bbox;
}

bool oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    if (outline_funcs == NULL) {
        return false;
    }

    OC__ID2D1SimplifiedGeometrySink geometry_sink = { 0 };
    geometry_sink.lpVtbl = &OC__ID2D1SimplifiedGeometrySinkVtbl;
    geometry_sink.funcs = outline_funcs;
    geometry_sink.ref_count = 1;
    geometry_sink.ctx = context;

    HRESULT err = face.impl->dw_face->lpVtbl->GetGlyphRunOutline(
        face.impl->dw_face,
        face.metrics.upem,
        &glyph_index,
        NULL,
        NULL,
        1,
        FALSE,
        FALSE,
        (IDWriteGeometrySink*)&geometry_sink);

    if (err != S_OK) {
        return false;
    }

    ULONG refs = geometry_sink.lpVtbl->Base.Release((IUnknown*)&geometry_sink);

    (void)refs;
    assert(refs == 0);

    return true;
}

// todo: check this rendering thingy as smth is a bit off with dwrite
//       it seems dwrite does hard edges, idk if we can change that
oc_error oc_render_glyph(oc_face face, uint16_t glyph_index, oc_size* psize, unsigned char* buffer, size_t buffer_size) {
    HRESULT err;
    IDWriteFontFace* dw_face = face.impl->dw_face;
    IDWriteFactory* dw_factory = face.impl->dw_factory;

    if (psize == NULL) {
        return oc_error_invalid_param;
    }

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    UINT16 glyph_count = dw_face->lpVtbl->GetGlyphCount(dw_face);
    if (glyph_index >= glyph_count) {
        return oc_error_invalid_param;
    }

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L414
    oc_bbox cbox;
    oc_bbox pbox;
    oc_get_glyph_bbox(face, glyph_index, OC_LOAD_DEFAULT, &cbox);

    pbox.min_x = cbox.min_x >> 6;
    pbox.min_y = cbox.min_y >> 6;
    pbox.max_x = cbox.max_x >> 6;
    pbox.max_y = cbox.max_y >> 6;

    // take fractional part and ceil it
    pbox.max_x += ((cbox.max_x & 63) + 63) >> 6;
    pbox.max_y += ((cbox.max_y & 63) + 63) >> 6;

    uint32_t rows = pbox.max_y - pbox.min_y;
    uint32_t cols = pbox.max_x - pbox.min_x;

    // todo: add smth like this to our oc_render_glyph
    DWRITE_MATRIX transform = {
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        (cbox.min_x & 63) / 64.0f,
        -(cbox.min_y & 63) / 64.0f,
    };

    // is this correct?
    oc_26p6 em_size = oc_mul_16p16(face.metrics.upem, face.metrics.scale);

    DWRITE_GLYPH_RUN glyph_run = { 0 };
    glyph_run.fontFace = dw_face;
    glyph_run.fontEmSize = em_size / 64.0f;
    glyph_run.glyphCount = 1;
    glyph_run.glyphIndices = &glyph_index;

    IDWriteGlyphRunAnalysis* analysis;
    err = dw_factory->lpVtbl->CreateGlyphRunAnalysis(
        dw_factory,
        &glyph_run,
        1.0f,
        &transform,
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
        DWRITE_MEASURING_MODE_NATURAL,
        -cbox.min_x / 64.0f,
        cbox.min_y / 64.0f,
        &analysis);
    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return oc__unexpected(err);
    }

    psize->rows = rows;
    psize->cols = cols;

    if (buffer == NULL) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_ok;
    }

    if (rows == 0 || cols == 0) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_ok;
    }

    if (buffer_size < rows * cols) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_insufficient_buffer;
    }

    unsigned char* buffer_3x = malloc(rows * cols * 3);
    if (buffer_3x == NULL) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_out_of_memory;
    }

    RECT bounds = { 0 };
    bounds.top = -(int32_t)rows;
    bounds.right = cols;

    err = analysis->lpVtbl->CreateAlphaTexture(
        analysis,
        DWRITE_TEXTURE_CLEARTYPE_3x1,
        &bounds,
        buffer_3x,
        rows * cols * 3);
    analysis->lpVtbl->Release(analysis);

    if (err != S_OK) {
        free(buffer_3x);
        return oc__unexpected(err);
    }

    for (uint32_t i = 0; i < rows * cols; i++) {
        uint8_t r = buffer_3x[i * 3 + 0];
        uint8_t g = buffer_3x[i * 3 + 1];
        uint8_t b = buffer_3x[i * 3 + 2];

        buffer[i] = (r + b + g) / 3.0f;
    }

    free(buffer_3x);
    return oc_error_ok;
}
