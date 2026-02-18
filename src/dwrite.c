#include "onecore.h"
#include "shared.h"
#ifdef ONECORE_DWRITE

#include <initguid.h>

#include <d2d1.h>
#include <dwrite.h>
#include <math.h>

struct face_internals {
    IDWriteFontFace* face;
    IDWriteFactory* library;
};

#define DW(x) _Generic((x),                       \
    oc_library: ((IDWriteFactory*)(x).internals), \
    oc_face: ((struct face_internals*)(x).internals)->face)

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
    D2D1_POINT_2F start;
    D2D1_POINT_2F origin;
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

    LONG refs = InterlockedDecrement(&this->ref_count);
    if (refs == 0) {
        free(this);
    }

    assert(refs != -1);
    return refs;
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

static ULONG STDMETHODCALLTYPE
IOCFontFileLoader_Release(IDWriteFontFileLoader* This) {
    IOCFontFileLoader* this = (IOCFontFileLoader*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    assert(refs != -1);
    return refs;
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

    if (this->origin.x != this->start.x || this->origin.y != this->start.y) {
        oc_point point = { this->start.x, -this->start.y };
        this->funcs->line_to(point, this->ctx);
    }

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

    assert(beziersCount > 0);
    this->origin = beziers[beziersCount - 1].point3;
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

    assert(pointsCount > 0);
    this->origin = points[pointsCount - 1];
}

static void STDMETHODCALLTYPE
IOCSimplifiedGeometrySink_BeginFigure(ID2D1SimplifiedGeometrySink* This, D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) {
    (void)figureBegin;
    IOCSimplifiedGeometrySink* this = (IOCSimplifiedGeometrySink*)This;

    oc_point point = { startPoint.x, -startPoint.y };
    this->funcs->start_figure(point, this->ctx);
    this->start = startPoint;
    this->origin = startPoint;
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

    LONG refs = InterlockedDecrement(&this->ref_count);
    assert(refs != -1);
    return refs;
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

IOCFontFileLoader ioc_font_file_loader = { &IOCFontFileLoaderVtbl, 0 };
IDWriteFontFileLoader* font_file_loader = (IDWriteFontFileLoader*)&ioc_font_file_loader;

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
        return unexpected(err);
    }

    err = dw_factory->lpVtbl->RegisterFontFileLoader(dw_factory, font_file_loader);
    if (err != S_OK) {
        // DWRITE_E_ALREADYREGISTERED;
        dw_factory->lpVtbl->Release(dw_factory);
        return unexpected(err);
    }

    plibrary->internals = dw_factory;
    return oc_error_ok;
}

inline void oc_free_library(oc_library library) {
    DW(library)->lpVtbl->UnregisterFontFileLoader(DW(library), font_file_loader);
    DW(library)->lpVtbl->Release(DW(library));
}

static oc_error open_face_from_font_file(oc_library library, IDWriteFontFile* font_file, const oc_face_params* pparams, oc_face* pface) {
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

    IDWriteFontFace* dw_font_face;
    oc_face_params params = fill_face_params(pparams);

    // todo: we should handle simulations
    err = DW(library)->lpVtbl->CreateFontFace(
        DW(library),
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
        return unexpected(err);
    }

    // dw_font_face needs to have ref to dw_library
    // alloc struct { dw_font_face, dw_library }

    struct face_internals* internals = malloc(sizeof(struct face_internals));
    if (internals == NULL) {
        dw_font_face->lpVtbl->Release(dw_font_face);
        return oc_error_out_of_memory;
    }

    internals->face = dw_font_face;
    internals->library = DW(library);

    DWRITE_FONT_METRICS metrics;
    dw_font_face->lpVtbl->GetMetrics(dw_font_face, &metrics);

    
    uint16_t ppem = roundf(params.desired_size * params.dpi / 72.0f);
    oc_i26p6 hh = params.desired_size * 64.0f;

    // https://github.com/freetype/freetype/blob/85c8efe0afa5ad0df35114e317a065f544943c52/include/freetype/internal/ftobjs.h#L665
    oc_i16p16 scaled_height = (hh * (int32_t)ppem + 36) / 72;
    oc_i16p16 scale = oc_div_ip16p16(scaled_height, metrics.designUnitsPerEm);

    pface->internals = internals;
    pface->metrics.ppem = ppem;
    pface->metrics.upem = metrics.designUnitsPerEm;
    pface->metrics.scale = scale;
    pface->metrics.ascent = metrics.ascent;
    pface->metrics.descent = metrics.descent;
    pface->metrics.leading = metrics.lineGap;
    pface->metrics.underline_position = metrics.underlinePosition;
    pface->metrics.underline_thickness = metrics.underlineThickness;

    return oc_error_ok;
}

oc_error oc_open_face(oc_library library, const char* path, const oc_face_params* pparams, oc_face* pface) {
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
    err = DW(library)->lpVtbl->CreateFontFileReference(
        DW(library),
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

    oc_error result = open_face_from_font_file(library, font_file, pparams, pface);
    font_file->lpVtbl->Release(font_file);

    return result;
}

oc_error oc_open_memory_face(oc_library library, const void* data, size_t size, const oc_face_params* pparams, oc_face* pface) {
    if (pface == NULL) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    HRESULT err;
    IDWriteFontFile* font_file;

    memory_view key = { data, size };

    err = DW(library)->lpVtbl->CreateCustomFontFileReference(
        DW(library),
        &key,
        sizeof(memory_view),
        font_file_loader,
        &font_file);

    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    oc_error result = open_face_from_font_file(library, font_file, pparams, pface);
    font_file->lpVtbl->Release(font_file);

    return result;
}

void oc_free_face(oc_face face) {
    DW(face)->lpVtbl->Release(DW(face));
    free(face.internals);
}

uint16_t oc_get_char_index(oc_face face, uint32_t charcode) {
    UINT16 index;

    HRESULT err = DW(face)->lpVtbl->GetGlyphIndices(
        DW(face),
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

    HRESULT err = DW(face)->lpVtbl->TryGetFontTable(
        DW(face),
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

    *ptable = table;
    *pcontext = context;

    return oc_error_ok;
}

inline void oc_free_table(oc_face face, void* context) {
    DW(face)->lpVtbl->ReleaseFontTable(DW(face), context);
}

void oc_get_glyph_metrics(oc_face face, uint16_t glyph_index, oc_load_flags flags, oc_glyph_metrics* pmetrics) {
    if (pmetrics == NULL) {
        return;
    }

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    UINT16 glyph_count = DW(face)->lpVtbl->GetGlyphCount(DW(face));
    if (glyph_index >= glyph_count) {
        memset(pmetrics, 0, sizeof(oc_glyph_metrics));
        return;
    }

    DWRITE_GLYPH_METRICS metrics;
    HRESULT err = DW(face)->lpVtbl->GetDesignGlyphMetrics(
        DW(face),
        &glyph_index,
        1,
        &metrics,
        FALSE);

    (void)err;
    assert(err == S_OK);

    if (flags & OC_LOAD_NO_SCALE) {
        pmetrics->width = metrics.advanceWidth - metrics.leftSideBearing - metrics.rightSideBearing;
        pmetrics->height = metrics.advanceHeight - metrics.topSideBearing - metrics.bottomSideBearing;
        pmetrics->bearing_x = metrics.leftSideBearing;
        pmetrics->bearing_y = metrics.verticalOriginY - metrics.topSideBearing;
        pmetrics->advance = metrics.advanceWidth;

        return;
    }

    pmetrics->width = oc_mul_ip16p16((metrics.advanceWidth - metrics.leftSideBearing - metrics.rightSideBearing), face.metrics.scale);
    pmetrics->height = oc_mul_ip16p16((metrics.advanceHeight - metrics.topSideBearing - metrics.bottomSideBearing), face.metrics.scale);
    pmetrics->bearing_x = oc_mul_ip16p16(metrics.leftSideBearing, face.metrics.scale);
    pmetrics->bearing_y = oc_mul_ip16p16((metrics.verticalOriginY - metrics.topSideBearing), face.metrics.scale);
    pmetrics->advance = oc_mul_ip16p16(metrics.advanceWidth, face.metrics.scale);
}

bool oc_get_outline(oc_face face, uint16_t glyph_index, const oc_outline_funcs* outline_funcs, void* context) {
    if (outline_funcs == NULL) {
        return false;
    }

    IOCSimplifiedGeometrySink ioc_simplified_geometry_sink = { 0 };
    ioc_simplified_geometry_sink.lpVtbl = &IOCSimplifiedGeometrySinkVtbl;
    ioc_simplified_geometry_sink.funcs = outline_funcs;
    ioc_simplified_geometry_sink.ref_count = 1;
    ioc_simplified_geometry_sink.ctx = context;

    IDWriteGeometrySink* geometry_sink = (IDWriteGeometrySink*)&ioc_simplified_geometry_sink;

    // dwrite does not call line_to at the end to the beg
    HRESULT err = DW(face)->lpVtbl->GetGlyphRunOutline(
        DW(face),
        face.metrics.upem,
        &glyph_index,
        NULL,
        NULL,
        1,
        FALSE,
        FALSE,
        geometry_sink);

    if (err != S_OK) {
        return false;
    }

    ULONG refs = geometry_sink->lpVtbl->Base.Release((IUnknown*)geometry_sink);

    (void)refs;
    assert(refs == 0);

    return true;
}

// todo: allow bbox to be bigger then glyph bbox
//       just modify it back to required size or sum
//       or mb just pass in buffer_size, not sure

oc_error oc_render_glyph(oc_face face, uint16_t glyph_index, oc_bbox* pbbox, unsigned char* buffer, size_t buffer_size) {
    IDWriteFactory* library = ((struct face_internals*)face.internals)->library;
    HRESULT err;

    if (pbbox == NULL) {
        return oc_error_invalid_param;
    }

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    UINT16 glyph_count = DW(face)->lpVtbl->GetGlyphCount(DW(face));
    if (glyph_index >= glyph_count) {
        return oc_error_invalid_param;
    }

    DWRITE_GLYPH_METRICS metrics;
    err = DW(face)->lpVtbl->GetDesignGlyphMetrics(
        DW(face),
        &glyph_index,
        1,
        &metrics,
        FALSE);
    assert(err == S_OK);

    float fppem = face.metrics.ppem;
    float fupem = face.metrics.upem;

    float origin_x = metrics.leftSideBearing * fppem / fupem;
    float origin_y = ((INT32)metrics.advanceHeight - metrics.verticalOriginY - metrics.bottomSideBearing) * fppem / fupem;

    float frac_x = origin_x - floorf(origin_x);
    float frac_y = origin_y - floorf(origin_y);

    // todo: add smth like this to our oc_render_glyph
    DWRITE_MATRIX transform = {
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        frac_x,
        frac_y,
    };

    DWRITE_GLYPH_RUN glyph_run = { 0 };
    glyph_run.fontFace = DW(face);
    glyph_run.fontEmSize = face.metrics.ppem;
    glyph_run.glyphCount = 1;
    glyph_run.glyphIndices = &glyph_index;

    IDWriteGlyphRunAnalysis* analysis;
    err = library->lpVtbl->CreateGlyphRunAnalysis(
        library,
        &glyph_run,
        1.0f,
        &transform,
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
        DWRITE_MEASURING_MODE_NATURAL,
        -origin_x,
        -origin_y,
        &analysis);
    switch (err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        return oc_error_out_of_memory;
    default:
        return unexpected(err);
    }

    RECT bounds;
    err = analysis->lpVtbl->GetAlphaTextureBounds(analysis, DWRITE_TEXTURE_CLEARTYPE_3x1, &bounds);
    if (err != S_OK) {
        analysis->lpVtbl->Release(analysis);
        return unexpected(err);
    }

    // cut everything that is below (0, 0)
    bounds.left = 0;
    bounds.bottom = 0;

    assert(bounds.top <= 0);
    assert(bounds.right >= 0);

    uint32_t rows = -bounds.top;
    uint32_t cols = bounds.right;

    pbbox->rows = rows;
    pbbox->cols = cols;

    if (buffer == NULL) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_ok;
    }

    if (buffer_size < rows * cols) {
        return oc_error_insufficient_buffer;
    }

    unsigned char* buffer_3x = malloc(rows * cols * 3);
    if (buffer_3x == NULL) {
        analysis->lpVtbl->Release(analysis);
        return oc_error_out_of_memory;
    }

    err = analysis->lpVtbl->CreateAlphaTexture(
        analysis,
        DWRITE_TEXTURE_CLEARTYPE_3x1,
        &bounds,
        buffer_3x,
        rows * cols * 3);
    analysis->lpVtbl->Release(analysis);

    if (err != S_OK) {
        free(buffer_3x);
        return unexpected(err);
    }

    for (uint32_t i = 0; i < rows * cols; i++) {
        uint8_t r = buffer_3x[i * 3 + 0];
        uint8_t g = buffer_3x[i * 3 + 1];
        uint8_t b = buffer_3x[i * 3 + 2];

        buffer[i] = (float)(r + b + g) / 3.0f;
    }

    free(buffer_3x);
    return oc_error_ok;
}

#endif // ONECORE_DWRITE
