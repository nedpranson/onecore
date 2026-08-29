#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define ONECORE_IMPLEMENTATION
#include "../onecore.h"

/* ONECORE_DIRECTWRITE_LOADER_IMPLEMENTATION */
#include <initguid.h>

#include <d2d1.h>
#include <dwrite.h>
#include <math.h>

struct oc_face_impl {
    IDWriteFontFace* dw_face;
    IDWriteFactory*  dw_factory;
};

typedef struct {
    const void* data;
    size_t      size;
} oc__memory_view;

typedef struct {
    const IDWriteFontFileStreamVtbl* lpVtbl;
    LONG                             ref_count;
    oc__memory_view                  memory_view;
} OC__IDWriteFontFileStream;

typedef struct {
    const IDWriteFontFileLoaderVtbl* lpVtbl;
    LONG                             ref_count;
} OC__IDWriteFontFileLoader;

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
    const void**           fragment_start,
    UINT64                 offset,
    UINT64                 fragment_size,
    void**                 fragment_context) {

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

    *fragment_start = (uint8_t*)this->memory_view.data + offset;
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

OC__IDWriteFontFileLoader oc__file_loader = { &OC__IDWriteFontFileLoaderVtbl, 0 };
IDWriteFontFileLoader*    oc__dw_file_loader = (IDWriteFontFileLoader*)&oc__file_loader;

#define OC__OVERRIDE_LIBRARY_IMPL

oc_error oc_init_library(oc_library** olibrary) {
    HRESULT         err;
    IDWriteFactory* dw_factory;

    if (olibrary == NULL) {
        return oc_error_invalid_param;
    }

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
        // DWRITE_E_ALREADYREGISTERED;
        dw_factory->lpVtbl->Release(dw_factory);
        return oc__unexpected(err);
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

    dw_factory->lpVtbl->UnregisterFontFileLoader(dw_factory, oc__dw_file_loader);
    dw_factory->lpVtbl->Release(dw_factory);
}

static oc_error oc__init_face(IDWriteFactory* dw_factory, IDWriteFontFace* dw_face, oc_26p6 desired_size, uint16_t dpi, oc_face* oface) {
    DWRITE_FONT_METRICS metrics;
    oc_16p16            scaled;
    oc_16p16            scale;
    int32_t             ppem;
    oc_face             face;

    assert(dpi > 0);
    assert(desired_size >= 1 << 6);

    face.impl = malloc(sizeof(face));
    if (face.impl == NULL) {
        dw_face->lpVtbl->Release(dw_face);
        return oc_error_out_of_memory;
    }

    dw_face->lpVtbl->GetMetrics(dw_face, &metrics);

    // https://github.com/freetype/freetype/blob/85c8efe0afa5ad0df35114e317a065f544943c52/include/freetype/internal/ftobjs.h#L665
    scaled = (desired_size * dpi + 36) / 72;
    scale = oc_div_16p16(scaled, metrics.designUnitsPerEm);

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L3368
    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        dw_face->lpVtbl->Release(dw_face);
        return oc_error_invalid_pixel_size;
    }

    face.impl->dw_face = dw_face;
    face.impl->dw_factory = dw_factory;
    face.size.scale = scale;
    face.size.ppem = (uint16_t)ppem;
    face.upem = metrics.designUnitsPerEm;
    face.ascent = metrics.ascent;
    face.descent = metrics.descent;
    face.leading = metrics.lineGap;
    face.underline_position = metrics.underlinePosition;
    face.underline_thickness = metrics.underlineThickness;

    *oface = face;
    return oc_error_ok;
}

static oc_error oc__open_face_from_font_file(IDWriteFactory* dw_factory, IDWriteFontFile* font_file, const oc_open_params* uparams, oc_face* oface) {
    HRESULT               err;
    WINBOOL               is_supported_fonttype;
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFace*      dw_face;
    UINT32                face_num;
    oc_open_params        params = oc__open_params_defaults(uparams);

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

    err = dw_factory->lpVtbl->CreateFontFace(
        dw_factory,
        face_type,
        1,
        &font_file,
        params.face_index,
        DWRITE_FONT_SIMULATIONS_NONE,
        &dw_face);

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

    return oc__init_face(dw_factory, dw_face, params.desired_size, params.dpi, oface);
}

oc_error ocl_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    int32_t          err;
    int              size;
    wchar_t*         dw_path;
    IDWriteFactory*  dw_factory;
    IDWriteFontFile* dw_font_file;

    if (!(library && path && oface)) {
        return oc_error_invalid_param;
    }

    size = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    if (size <= 1) {
        return oc_error_failed_to_open;
    }

    dw_path = malloc(size * sizeof(wchar_t));
    if (dw_path == NULL) {
        return oc_error_out_of_memory;
    }

    size = MultiByteToWideChar(CP_UTF8, 0, path, -1, dw_path, size);
    assert(size > 0);

    dw_factory = (IDWriteFactory*)library;
    err = dw_factory->lpVtbl->CreateFontFileReference(
        dw_factory,
        dw_path,
        NULL,
        &dw_font_file);
    free(dw_path);

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

    err = oc__open_face_from_font_file(dw_factory, dw_font_file, uparams, oface);
    dw_font_file->lpVtbl->Release(dw_font_file);

    return err;
}

oc_error ocl_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    int32_t          err;
    IDWriteFontFile* font_file;
    IDWriteFactory*  dw_factory;
    oc__memory_view  key;

    if (!(library && data && oface)) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    dw_factory = (IDWriteFactory*)library;

    key.data = data;
    key.size = size;

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

    err = oc__open_face_from_font_file(dw_factory, font_file, uparams, oface);
    font_file->lpVtbl->Release(font_file);

    return err;
}

void ocl_free_face(oc_face* face) {
    face->impl->dw_face->lpVtbl->Release(face->impl->dw_face);
    free(face->impl);
    memset(face, 0, sizeof(*face));
}

uint16_t ocl_get_char_index(const oc_face* face, uint32_t charcode) {
    HRESULT          err;
    IDWriteFontFace* dw_face;
    UINT16           index;

    if (face == NULL) {
        return 0;
    }

    dw_face = face->impl->dw_face;
    err = dw_face->lpVtbl->GetGlyphIndices(
        dw_face,
        &charcode,
        1,
        &index);

    (void)err;
    assert(err == S_OK);

    return index;
}

oc_error ocl_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t  ppem;

    if (!face) {
        return oc_error_invalid_param;
    }

    if (desired_size < 1 << 6) {
        return oc_error_invalid_param;
    }

    if (dpi == 0) {
        dpi = 72;
    }

    scaled = (desired_size * dpi + 36) / 72;
    scale = oc_div_16p16(scaled, face->upem);
    ppem = (scaled + 32) >> 6;

    if (ppem > UINT16_MAX) {
        return oc_error_invalid_pixel_size;
    }

    face->size.scale = scale;
    face->size.ppem = (uint16_t)ppem;

    return oc_error_ok;
}

oc_error ocl_get_sfnt_table(const oc_face* face, oc_tag tag, uint32_t offset, void* data, uint32_t* size) {
    HRESULT          err;
    IDWriteFontFace* dw_face;

    const void* table_data;
    UINT32      table_size;

    void*   context;
    WINBOOL exists;

    uint32_t length;

    if (!(face && size)) {
        return oc_error_invalid_param;
    }

    dw_face = face->impl->dw_face;
    length = *size;

    assert(sizeof(UINT32) == sizeof(uint32_t));
    assert(length == 0 || length >= offset);

    err = dw_face->lpVtbl->TryGetFontTable(
        dw_face,
        _byteswap_ulong(tag), // swapping bytes as windows table tags are little-endian
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

    if (!exists) {
        return oc_error_table_missing;
    }

    if (length == 0) {
        *size = table_size;
    } else {
        memcpy(data, (uint8_t*)table_data + offset, length);
    }

    dw_face->lpVtbl->ReleaseFontTable(dw_face, context);
    return oc_error_ok;
}

void ocl_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    HRESULT              err;
    DWRITE_GLYPH_METRICS dw_metrics;
    IDWriteFontFace*     dw_face;
    oc_16p16             scale;
    UINT16               count;
    oc_glyph_metrics     metrics = { 0 };

    if (!(face && ometrics)) {
        goto exit;
    }

    dw_face = face->impl->dw_face;
    count = dw_face->lpVtbl->GetGlyphCount(dw_face);

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    if (index >= count) {
        goto exit;
    }

    err = dw_face->lpVtbl->GetDesignGlyphMetrics(
        dw_face,
        &index,
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
        goto exit;
    }

    scale = face->size.scale;

    metrics.width = oc_mul_16p16(metrics.width, scale);
    metrics.height = oc_mul_16p16(metrics.height, scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, scale);
    metrics.advance = oc_mul_16p16(metrics.advance, scale);

    if (flags & OC_LOAD_NO_FITTING) {
        goto exit;
    }

    oc__fit_metrics(&metrics);
exit:
    if (ometrics)
        *ometrics = metrics;
}

void ocl_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    HRESULT              err;
    DWRITE_GLYPH_METRICS metrics;
    IDWriteFontFace*     dw_face;
    UINT16               count;
    oc_16p16             scale;
    oc_bbox              cbox = { 0 };

    if (!(face && ocbox)) {
        goto exit;
    }

    dw_face = face->impl->dw_face;
    count = dw_face->lpVtbl->GetGlyphCount(dw_face);

    if (index >= count) {
        goto exit;
    }

    err = dw_face->lpVtbl->GetDesignGlyphMetrics(
        dw_face,
        &index,
        1,
        &metrics,
        FALSE);

    (void)err;
    assert(err == S_OK);

    cbox.min_x = metrics.leftSideBearing;
    cbox.min_y = metrics.verticalOriginY + metrics.bottomSideBearing - (INT32)metrics.advanceHeight;
    cbox.max_x = metrics.advanceWidth - metrics.rightSideBearing;
    cbox.max_y = metrics.verticalOriginY - metrics.topSideBearing;

    if (flags & OC_LOAD_NO_SCALE) {
        goto exit;
    }

    scale = face->size.scale;

    cbox.min_x = oc_mul_16p16(cbox.min_x, scale);
    cbox.min_y = oc_mul_16p16(cbox.min_y, scale);
    cbox.max_x = oc_mul_16p16(cbox.max_x, scale);
    cbox.max_y = oc_mul_16p16(cbox.max_y, scale);

exit:
    if (ocbox)
        *ocbox = cbox;
}

typedef enum {
    oc__path_move,
    oc__path_line,
    oc__path_conic,
    oc__path_cubic,
    oc__path_close,
} oc__path;

typedef struct {
    oc_point pt1;
    oc_point pt2;
    oc_point pt3;
    oc__path type;
} oc__path_element;

typedef struct {
    D2D1_POINT_2F pt1;
    D2D1_POINT_2F pt2;
    D2D1_POINT_2F pt3;
    oc__path      type;
} oc__path_felement;

typedef struct {
    const ID2D1SimplifiedGeometrySinkVtbl* lpVtbl;

    uint8_t*  tags;
    oc_point* points;
    uint16_t* contours;

    oc__path_element element;

    D2D1_POINT_2F origin;
    D2D1_POINT_2F start;

    LONG ref_count;
    bool doomed; // if true oom is reached
} OC__PathSink;

static bool oc__walk_outline(OC__PathSink* sink, const oc__path_felement* element) {
    oc__path_element next_element = {
        .pt1 = { element->pt1.x * 2.0f, element->pt1.y * -2.0f },
        .pt2 = { element->pt2.x * 2.0f, element->pt2.y * -2.0f },
        .pt3 = { element->pt3.x * 2.0f, element->pt3.y * -2.0f },
        .type = element->type,
    };

    oc_point start = { sink->start.x * 2.0f, sink->start.y * -2.0f };

    bool implicit;
    bool closing;

    switch (sink->element.type) {
    case oc__path_move:
        if (!oc__append(sink->points, oc__point_bsr(sink->element.pt1, 1)))
            return true;
        if (!oc__append(sink->tags, OC__CURVE_TAG_ON))
            return true;
        break;
    case oc__path_line:
        if (!oc__points_equal(sink->element.pt1, start)) {
            if (!oc__append(sink->points, oc__point_bsr(sink->element.pt1, 1)))
                return true;
            if (!oc__append(sink->tags, OC__CURVE_TAG_ON))
                return true;
        }
        break;
    case oc__path_conic:
        if (!oc__append(sink->points, oc__point_bsr(sink->element.pt1, 1)))
            return true;
        if (!oc__append(sink->tags, OC__CURVE_TAG_CONIC))
            return true;

        implicit = false;
        closing = false;

        if (next_element.type == oc__path_close) {
            closing = oc__points_equal(sink->element.pt2, start);
        }

        if (next_element.type == oc__path_conic) {
            implicit = oc__is_midpoint(sink->element.pt2, sink->element.pt1, next_element.pt1);
        }

        if (!implicit && !closing) {
            if (!oc__append(sink->points, oc__point_bsr(sink->element.pt2, 1)))
                return true;
            if (!oc__append(sink->tags, OC__CURVE_TAG_ON))
                return true;
        }
        break;
    case oc__path_cubic:
        if (!oc__append(sink->points, oc__point_bsr(sink->element.pt1, 1)))
            return true;
        if (!oc__append(sink->points, oc__point_bsr(sink->element.pt2, 1)))
            return true;

        if (!oc__append(sink->tags, OC__CURVE_TAG_CUBIC))
            return true;
        if (!oc__append(sink->tags, OC__CURVE_TAG_CUBIC))
            return true;

        closing = false;
        if (next_element.type == oc__path_close) {
            closing = oc__points_equal(sink->element.pt3, start);
        }

        if (!closing) {
            if (!oc__append(sink->points, oc__point_bsr(sink->element.pt3, 1)))
                return true;
            if (!oc__append(sink->tags, OC__CURVE_TAG_ON))
                return true;
        }
        break;
    case oc__path_close:
        assert(oc__len(sink->points) > 0);
        if (!oc__append(sink->contours, oc__len(sink->points) - 1))
            return true;
        break;
    default:
        break;
    }

    sink->element = next_element;
    return false;
}

static HRESULT STDMETHODCALLTYPE
OC__PathSink_Close(ID2D1SimplifiedGeometrySink* This) {
    (void)This;
    return S_OK;
}

static void STDMETHODCALLTYPE
OC__PathSink_EndFigure(ID2D1SimplifiedGeometrySink* This, D2D1_FIGURE_END figureEnd) {
    OC__PathSink* this = (OC__PathSink*)This;
    oc__path_felement element = { 0 };

    (void)figureEnd;

    if (oc__unlikely(this->doomed)) {
        return;
    }

    element.type = oc__path_close;
    this->doomed = oc__walk_outline(this, &element);
}

static void STDMETHODCALLTYPE
OC__PathSink_AddBeziers(ID2D1SimplifiedGeometrySink* This, const D2D1_BEZIER_SEGMENT* beziers, UINT beziersCount) {
    OC__PathSink* this = (OC__PathSink*)This;
    oc__path_felement element = { 0 };

    if (oc__unlikely(this->doomed)) {
        return;
    }

    for (UINT32 i = 0; i < beziersCount; i++) {
        D2D1_POINT_2F pt1 = beziers[i].point1;
        D2D1_POINT_2F pt2 = beziers[i].point2;
        D2D1_POINT_2F pt3 = beziers[i].point3;

        D2D1_POINT_2F origin = this->origin;

        float cx1 = (3.0f * pt1.x - origin.x) * 0.5f;
        float cy1 = (3.0f * pt1.y - origin.y) * 0.5f;

        float cx2 = (3.0f * pt2.x - pt3.x) * 0.5f;
        float cy2 = (3.0f * pt2.y - pt3.y) * 0.5f;

        float dx = cx1 - cx2;
        float dy = cy1 - cy2;

        if (dx * dx + dy * dy < 0.05f * 0.05f) {
            element.pt1 = (D2D1_POINT_2F) { roundf(cx2), roundf(cy2) };
            element.pt2 = pt3;
            element.type = oc__path_conic;
        } else {
            element.pt1 = pt1;
            element.pt2 = pt2;
            element.pt3 = pt3;
            element.type = oc__path_cubic;
        }

        if (oc__unlikely(this->doomed = oc__walk_outline(this, &element))) {
            break;
        }

        this->origin = pt3;
    }
}

static void STDMETHODCALLTYPE
OC__PathSink_AddLines(ID2D1SimplifiedGeometrySink* This, const D2D1_POINT_2F* points, UINT pointsCount) {
    OC__PathSink* this = (OC__PathSink*)This;
    oc__path_felement element = { 0 };

    if (oc__unlikely(this->doomed)) {
        return;
    }

    element.type = oc__path_line;
    for (UINT32 i = 0; i < pointsCount; i++) {
        element.pt1 = points[i];

        if (oc__unlikely(this->doomed = oc__walk_outline(this, &element))) {
            break;
        }

        this->origin = points[i];
    }
}

static void STDMETHODCALLTYPE
OC__PathSink_BeginFigure(ID2D1SimplifiedGeometrySink* This, D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) {
    OC__PathSink* this = (OC__PathSink*)This;
    oc__path_felement element = { 0 };

    (void)figureBegin;

    if (oc__unlikely(this->doomed)) {
        return;
    }

    element.type = oc__path_move;
    element.pt1 = startPoint;

    this->doomed = oc__walk_outline(this, &element);

    this->origin = startPoint;
    this->start = startPoint;
}

static void STDMETHODCALLTYPE
OC__PathSink_SetSegmentFlags(ID2D1SimplifiedGeometrySink* This, D2D1_PATH_SEGMENT vertexFlags) {
    (void)This;
    (void)vertexFlags;
}

static void STDMETHODCALLTYPE
OC__PathSink_SetFillMode(ID2D1SimplifiedGeometrySink* This, D2D1_FILL_MODE fillMode) {
    (void)This;
    (void)fillMode;
}

static ULONG STDMETHODCALLTYPE
OC__PathSink_Release(IUnknown* This) {
    OC__PathSink* this = (OC__PathSink*)This;

    LONG refs = InterlockedDecrement(&this->ref_count);
    assert(refs != -1);
    return refs;
}

static ULONG STDMETHODCALLTYPE
OC__PathSink_AddRef(IUnknown* This) {
    OC__PathSink* this = (OC__PathSink*)This;
    return InterlockedIncrement(&this->ref_count);
}

static HRESULT STDMETHODCALLTYPE
OC__PathSink_QueryInterface(IUnknown* This, REFIID riid, void** ppvObject) {
    if (ppvObject == NULL) {
        return E_POINTER;
    }
    *ppvObject = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteFontFileLoader)) {
        OC__PathSink_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const ID2D1SimplifiedGeometrySinkVtbl OC__PathSinkVtbl = {
    { OC__PathSink_QueryInterface,
        OC__PathSink_AddRef,
        OC__PathSink_Release },
    OC__PathSink_SetFillMode,
    OC__PathSink_SetSegmentFlags,
    OC__PathSink_BeginFigure,
    OC__PathSink_AddLines,
    OC__PathSink_AddBeziers,
    OC__PathSink_EndFigure,
    OC__PathSink_Close,
};

oc_error ocl_get_outline(const oc_face* face, uint16_t index, oc_load_flags flags, oc_outline* ooutline) {
    HRESULT          hr;
    ULONG            refs;
    IDWriteFontFace* dw_face;
    UINT16           count;

    oc_error   err = oc_error_ok;
    oc_outline outline = { 0 };

    OC__PathSink sink = { 0 };

    (void)flags;

    // todo: handle first check like this
    if (!(face && ooutline)) {
        return oc_error_invalid_param;
    }

    // todo: cache nglyphs
    dw_face = face->impl->dw_face;
    count = dw_face->lpVtbl->GetGlyphCount(dw_face);

    if (index >= count) {
        oc__exit(oc_error_invalid_param);
    }

    sink.lpVtbl = &OC__PathSinkVtbl;
    sink.element.type = -1;
    sink.ref_count = 1;

    hr = face->impl->dw_face->lpVtbl->GetGlyphRunOutline(
        face->impl->dw_face,
        face->upem,
        &index,
        NULL,
        NULL,
        1,
        FALSE,
        FALSE,
        (IDWriteGeometrySink*)&sink);

    if (hr != S_OK) {
        oc__exit(oc__unexpected(hr));
    }

    refs = sink.lpVtbl->Base.Release((IUnknown*)&sink);

    (void)refs;
    assert(refs == 0);

    if (oc__unlikely(sink.doomed)) {
        oc__exit(oc_error_out_of_memory);
    }

    if (oc__len(sink.points) > 0) {
        assert(sink.element.type == oc__path_close);
        if (!oc__append(sink.contours, oc__len(sink.points) - 1)) {
            oc__exit(oc_error_out_of_memory);
        }
    }

    outline.npoints = (uint16_t)oc__len(sink.points);
    outline.ncontours = (uint16_t)oc__len(sink.contours);

    outline.tags = sink.tags;
    outline.points = sink.points;
    outline.contours = sink.contours;
exit:
    *ooutline = outline;

    if (err != oc_error_ok) {
        oc__free(sink.tags);
        oc__free(sink.points);
        oc__free(sink.contours);
    }

    return err;
}

void ocl_free_outline(oc_outline* outline) {
    if (outline == NULL) {
        return;
    }

    oc__free(outline->tags);
    oc__free(outline->points);
    oc__free(outline->contours);

    memset(outline, 0, sizeof(*outline));
}

oc_error ocl_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t pitch) {
    oc_error err = oc_error_ok;
    HRESULT  dw_err = S_OK;

    IDWriteFontFace* dw_face;
    IDWriteFactory*  dw_factory;

    oc_bbox cbox;
    oc_bbox pbox;

    DWRITE_MATRIX transform;
    UINT16        count;
    RECT          bounds;

    IDWriteGlyphRunAnalysis* analysis = NULL;
    DWRITE_GLYPH_RUN         glyph_run = { 0 };
    oc_extent                extent = { 0 };

    uint8_t* bitmap = NULL;
    size_t   length;

    uint8_t* idx;

    if (!(face && oextent)) {
        oc__exit(oc_error_invalid_param);
    }

    dw_face = face->impl->dw_face;
    dw_factory = face->impl->dw_factory;
    count = dw_face->lpVtbl->GetGlyphCount(dw_face);

    // for some reason GetDesignGlyphMetrics does not catch invalid glyph index
    if (index >= count) {
        oc__exit(oc_error_invalid_param);
    }

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L414
    ocl_get_glyph_cbox(face, index, OC_LOAD_DEFAULT, &cbox);

    pbox.min_x = cbox.min_x >> 6;
    pbox.min_y = cbox.min_y >> 6;
    pbox.max_x = cbox.max_x >> 6;
    pbox.max_y = cbox.max_y >> 6;

    // take fractional part and ceil it
    pbox.max_x += ((cbox.max_x & 63) + 63) >> 6;
    pbox.max_y += ((cbox.max_y & 63) + 63) >> 6;

    extent.rows = pbox.max_y - pbox.min_y;
    extent.cols = pbox.max_x - pbox.min_x;

    if (buffer == NULL) {
        goto exit;
    }

    if (extent.rows == 0 || extent.cols == 0) {
        goto exit;
    }

    transform.m11 = 1.0f;
    transform.m12 = 0.0f;
    transform.m21 = 0.0f;
    transform.m22 = 1.0f;
    transform.dx = (cbox.min_x & 63) / 64.0f;
    transform.dy = -(cbox.min_y & 63) / 64.0f;

    glyph_run.fontFace = dw_face;
    glyph_run.fontEmSize = oc_mul_16p16(face->upem, face->size.scale) / 64.0f;
    glyph_run.glyphCount = 1;
    glyph_run.glyphIndices = &index;

    // todo: when no hinting use dwrite_2.h
    // wih DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE

    dw_err = dw_factory->lpVtbl->CreateGlyphRunAnalysis(
        dw_factory,
        &glyph_run,
        1.0f,
        &transform,
        DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
        DWRITE_MEASURING_MODE_NATURAL,
        -cbox.min_x / 64.0f,
        cbox.min_y / 64.0f,
        &analysis);
    switch (dw_err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(err));
    }

    length = (size_t)extent.rows * (size_t)extent.cols * 3;
    bitmap = malloc(length);

    if (bitmap == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    bounds.left = 0;
    bounds.bottom = 0;

    bounds.top = -(int32_t)extent.rows;
    bounds.right = (int32_t)extent.cols;

    err = analysis->lpVtbl->CreateAlphaTexture(
        analysis,
        DWRITE_TEXTURE_CLEARTYPE_3x1,
        &bounds,
        bitmap,
        length);

    if (err != S_OK) {
        oc__exit(oc__unexpected(err));
    }

    idx = bitmap;
    for (uint32_t y = 0; y < extent.rows; y++) {
        for (uint32_t x = 0; x < extent.cols; x++) {
            uint8_t r = *idx++;
            uint8_t g = *idx++;
            uint8_t b = *idx++;

            buffer[y * pitch + x] = (r + b + g) / 3;
        }
    }
exit:
    if (bitmap)
        free(bitmap);
    if (analysis)
        analysis->lpVtbl->Release(analysis);
    if (oextent)
        *oextent = extent;
    return err;
}
