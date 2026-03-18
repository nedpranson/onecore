#include "minwindef.h"
#include "winerror.h"
#include <stdint.h>
#include <stdlib.h>
#define ONECORE_IMPLEMENTATION
#include "onecore.h"

/* ONECORE_DIRECTWRITE_IMPLEMENTATION */
#include <assert.h>
#include <initguid.h>

#include <d2d1.h>
#include <dwrite.h>

#define oc__exit(e) \
    do {            \
        err = (e);  \
        goto exit;  \
    } while (0)

// tood: use wyhash as this one likes to collide alot
// static uint64_t oc__fnv1a(const void* ptr, size_t size) {
//     const uint8_t* data = ptr;
//     uint64_t hash = 14695981039346656037ULL;
//
//     for (size_t i = 0; i < size; i++) {
//         hash ^= data[i];
//         hash *= 1099511628211ULL;
//     }
//
//     return hash;
// }
//
// typedef struct {
//     uint64_t key;
//     char*    val;
// } oc__kv_t;
//
// // tood: rename to set_t an handle true hash collision
// //       a.k.a. just check if input key is same as value key
// typedef struct {
//     oc__kv_t* kvs;
//     size_t    len;
//     size_t    cap;
// } oc__map_t;
//
// static void oc__free_map(oc__map_t* map) {
//     for (size_t i = 0; i < map->cap; i++) {
//         free(map->kvs[i].val);
//     }
//     free(map->kvs);
// }
//
// static oc__kv_t* oc__map_find(oc__map_t* map, uint64_t hash) {
//     size_t index = 0;
//     size_t mask = map->cap - 1;
//
//     if (hash == 0) {
//         hash = (uint64_t)(-1);
//     }
//
//     for (;; index++) {
//         size_t i = (hash + index) & mask;
//         oc__kv_t* kv = map->kvs + i;
//
//         if (kv->key == hash || kv->key == 0) {
//             return kv;
//         }
//     }
// }
//
// static bool oc__map_ensure_capacity(oc__map_t* map, size_t new_capacity) {
//     size_t capacity = map->cap;
//     size_t load = 100;
//
//     if (capacity > 0) {
//         load = (new_capacity * 100 + new_capacity - 1) / capacity;
//     }
//
//     if (load >= 80) {
//         oc__map_t map_copy;
//         oc__kv_t* kvs;
//
//         capacity = capacity > 16 ? capacity : 16;
//         while (new_capacity > capacity) {
//             capacity += capacity;
//         }
//
//         kvs = calloc(capacity, sizeof(*kvs));
//         if (kvs == NULL) {
//             return false;
//         }
//
//         map_copy.kvs = kvs;
//         map_copy.len = map->len;
//         map_copy.cap = capacity;
//
//         kvs = map->kvs;
//
//         for (size_t i = 0; i < map->len; i++) {
//             oc__kv_t kv = kvs[i];
//             if (kv.key != 0) {
//                 *oc__map_find(&map_copy, kv.key) = kv;
//             }
//         }
//
//         *map = map_copy;
//         free(kvs);
//     }
//
//     return true;
// }
//
// static oc__kv_t* oc__map_obtain(oc__map_t* map, uint64_t key) {
//     oc__kv_t* kv = NULL;
//     if (oc__map_ensure_capacity(map, map->len + 1)) {
//         kv = oc__map_find(map, key);
//         if (kv->key == 0) {
//             kv->key = key;
//             map->len++;
//         }
//     }
//
//     return kv;
// }

struct oc_face_impl {
    IDWriteFontFace* dw_face;
    IDWriteFactory* dw_factory;
};

struct oc_collection_impl {
    IDWriteFactory* dw_factory;
    char** family_names;
    uint16_t family_names_length;
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

oc_error oc_init_library(oc_library* olibrary) {
    HRESULT err;
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

    olibrary->internals = dw_factory;
    return oc_error_ok;
}

void oc_free_library(oc_library* library) {
    IDWriteFactory* dw_factory;

    if (!library) {
        return;
    }

    dw_factory = library->internals;

    dw_factory->lpVtbl->UnregisterFontFileLoader(dw_factory, oc__dw_file_loader);
    dw_factory->lpVtbl->Release(dw_factory);

    memset(library, 0, sizeof(oc_library));
}

typedef struct {
    IDWriteFont* dw_font;
    oc_font font;
} oc__font_impl;

static inline void oc__free_font(oc_font* font) {
    oc__font_impl* impl = oc__parentof(oc__font_impl, font, font);
    impl->dw_font->lpVtbl->Release(impl->dw_font);
    free(impl);
}

// todo: make ocol be **oc_collection
// and make oc_collection anonymous no internals shaize
oc_error oc_init_collection(const oc_library* library, oc_collection* ocollection) {
    oc_collection collection = { 0 };
    oc_error err = oc_error_ok;

    if (!(library && ocollection)) {
        err = oc_error_invalid_param;
        goto exit;
    }


    collection.impl = malloc(sizeof(oc_collection_impl));
    if (collection.impl == NULL) {
        return oc_error_out_of_memory;
    }

    collection.impl->dw_factory = library->internals;
    collection.impl->family_names = NULL;
    collection.impl->family_names_length = 0;
    collection.fonts = NULL;
    collection.elements = 0;
exit:
    if (ocollection) *ocollection = collection;
    return err;
}

void oc_free_collection(oc_collection* collection) {
    if (collection) {
        for (size_t i = 0; i < collection->elements; i++) {
            oc__free_font(collection->fonts[i]);
        }

        free(collection->fonts);
        free(collection->impl->family_names);
        free(collection->impl);

        memset(collection, 0, sizeof(*collection));
    }
}

static inline char* oc__utf16_to_utf8(const wchar_t* utf16, size_t utf16_size) {
    char* utf8;
    size_t utf8_size;

    utf8_size = WideCharToMultiByte(
        CP_UTF8,
        0, // todo: WC_ERR_INVALID_CHARS and test this
        utf16,
        utf16_size,
        NULL,
        0,
        NULL,
        NULL);

    if (utf8_size == 0) {
        return NULL;
    }

    utf8 = malloc(utf8_size + 1);
    if (utf8 != NULL) {
        WideCharToMultiByte(
            CP_UTF8,
            0,
            utf16,
            utf16_size,
            utf8,
            utf8_size,
            NULL,
            NULL);

        utf8[utf8_size] = '\0';
    }

    return utf8;
}

static oc_font* oc__init_font(IDWriteFont* dw_font, const char* family) {
    DWRITE_FONT_WEIGHT weight;
    oc__font_impl* impl;

    weight = dw_font->lpVtbl->GetWeight(dw_font);
    impl = malloc(sizeof(*impl));

    if (impl == NULL) {
        return NULL;
    }

    impl->dw_font = dw_font;
    impl->font.family = family;
    impl->font.weight = (uint16_t)weight;

    return &impl->font;
}

oc_error oc_load_fonts(oc_collection* collection) {
    oc_error err = oc_error_ok;
    HRESULT hr;

    IDWriteFactory* dw_factory;
    IDWriteFontCollection* dw_collection = NULL;

    UINT32 font_count;
    UINT32 family_index = 0;

    // char** family_names = NULL;
    UINT32 family_count;

    WCHAR* wide_buf = NULL;
    UINT32 wide_buf_len;

    oc_font** fonts = NULL;
    size_t elements = 0;

    // todo: make IDWriteLocalizedStrings* and char* union
    //       and on exit handle dw releases
    union {
        char*  str;
        UINT32 len;
    }* family_names = NULL;

    oc_collection collection_copy;

    // on failure collection must stay the same
    // this function should have no side effects on failure!

    if (!collection) {
        oc__exit(oc_error_invalid_param);
    }

    dw_factory = collection->impl->dw_factory;
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
    font_count = 0;

    family_names = malloc(family_count * sizeof(*family_names));
    if (family_names == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    // todo: just allocate family names here!!!
    for (UINT32 i = 0; i < family_count; i++) {
        IDWriteFontFamily* family;
        IDWriteLocalizedStrings* names;
        UINT32 length;

        hr = dw_collection->lpVtbl->GetFontFamily(dw_collection, i, &family);
        assert(hr == S_OK);

        hr = family->lpVtbl->GetFamilyNames(family, &names);
        assert(hr == S_OK);

        // todo: test this locale idx feature an other backends
        hr = names->lpVtbl->GetStringLength(names, 0, &length);
        assert(hr == S_OK);

        family_names[i].len = length;
        wide_buf_len = OC__MAX(wide_buf_len, length);
        font_count += family->lpVtbl->GetFontCount(family);

        names->lpVtbl->Release(names);
        family->lpVtbl->Release(family);
    }

    wide_buf = malloc((wide_buf_len + 1) * sizeof(WCHAR));
    if (wide_buf == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    fonts = malloc(sizeof(*fonts) * font_count);
    if (fonts == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    for (UINT32 i = 0; i < family_count; i++) {
        IDWriteFontFamily* font_family;
        IDWriteLocalizedStrings* names;

        UINT32 wide_length = family_names[i].len;
        UINT32 font_index;

        char* family;
        int length;

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
            // todo: do smth!
            assert(false);
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
        family_names[font_index++].str = family;
        font_index = font_family->lpVtbl->GetFontCount(font_family);

        while (font_index--) {
            IDWriteFont* dw_font;
            oc_font* font;

            hr = font_family->lpVtbl->GetFont(font_family, font_index, &dw_font);
            assert(hr == S_OK);

            font = oc__init_font(dw_font, family);
            if (font == NULL) {
                // todo: handle
                assert(false);
        //         family->lpVtbl->Release(family);
        //         err = oc_error_out_of_memory;
        //         goto exit;
            }

            fonts[elements++] = font;
        }

        font_family->lpVtbl->Release(font_family);
    }

    collection_copy.impl = collection->impl;
    collection_copy.elements = font_count;
    collection_copy.fonts = fonts;

    fonts = collection->fonts;
    elements = collection->elements;

    *collection = collection_copy;
exit:
    while (elements--) oc__free_font(fonts[elements]);
    while (family_index--) free(family_names[family_index].str);

    if (dw_collection) dw_collection->lpVtbl->Release(dw_collection);

    free(family_names);
    free(wide_buf);
    free(fonts);

    return err;
}

static oc_error oc__open_face_from_font_file(IDWriteFactory* dw_factory, IDWriteFontFile* font_file, const oc_open_params* uparams, oc_face* oface) {
    HRESULT err;
    WINBOOL is_supported_fonttype;
    DWRITE_FONT_FILE_TYPE file_type;
    DWRITE_FONT_FACE_TYPE face_type;
    IDWriteFontFace* dw_face;
    UINT32 face_num;
    oc_face face;
    DWRITE_FONT_METRICS metrics;
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t ppem;
    oc_open_params params = oc__open_params_defaults(uparams);

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

    // todo: we should handle simulations
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

    face.impl = malloc(sizeof(face));
    if (face.impl == NULL) {
        dw_face->lpVtbl->Release(dw_face);
        return oc_error_out_of_memory;
    }

    dw_face->lpVtbl->GetMetrics(dw_face, &metrics);

    // https://github.com/freetype/freetype/blob/85c8efe0afa5ad0df35114e317a065f544943c52/include/freetype/internal/ftobjs.h#L665
    scaled = (params.desired_size * params.dpi + 36) / 72;
    scale = oc_div_16p16(scaled, metrics.designUnitsPerEm);

    // https://github.com/freetype/freetype/blob/master/src/base/ftobjs.c#L3368
    ppem = (scaled + 32) >> 6;
    if (ppem > UINT16_MAX) {
        return oc_error_invalid_pixel_size;
    }

    face.impl->dw_face = dw_face;
    face.impl->dw_factory = dw_factory;
    face.metrics.upem = metrics.designUnitsPerEm;
    face.metrics.ppem = (uint16_t)ppem;
    face.metrics.scale = scale;
    face.metrics.ascent = metrics.ascent;
    face.metrics.descent = metrics.descent;
    face.metrics.leading = metrics.lineGap;
    face.metrics.underline_position = metrics.underlinePosition;
    face.metrics.underline_thickness = metrics.underlineThickness;

    *oface = face;
    return oc_error_ok;
}

oc_error oc_open_face(const oc_library* library, const char* path, const oc_open_params* uparams, oc_face* oface) {
    int32_t err;
    int size;
    wchar_t* dw_path;
    IDWriteFactory* dw_factory;
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

    dw_factory = library->internals;
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

oc_error oc_open_memory_face(const oc_library* library, const void* data, size_t size, const oc_open_params* uparams, oc_face* oface) {
    int32_t err;
    IDWriteFontFile* font_file;
    IDWriteFactory* dw_factory;
    oc__memory_view key;

    if (!(library && data && oface)) {
        return oc_error_invalid_param;
    }

    if (data == NULL) {
        return oc_error_invalid_param;
    }

    dw_factory = library->internals;

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

void oc_free_face(oc_face* face) {
    face->impl->dw_face->lpVtbl->Release(face->impl->dw_face);
    free(face->impl);
    memset(face, 0, sizeof(*face));
}

uint16_t oc_get_char_index(const oc_face* face, uint32_t charcode) {
    HRESULT err;
    IDWriteFontFace* dw_face;
    UINT16 index;

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

// race!!!!!! to face.metrics->ppem and face->metrics.scale should we allow it?
oc_error oc_set_size(oc_face* face, oc_26p6 desired_size, uint16_t dpi) {
    oc_16p16 scaled;
    oc_16p16 scale;
    int32_t ppem;

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
    scale = oc_div_16p16(scaled, face->metrics.upem);
    ppem = (scaled + 32) >> 6;

    if (ppem > UINT16_MAX) {
        return oc_error_invalid_pixel_size;
    }

    face->metrics.ppem = (uint16_t)ppem;
    face->metrics.scale = scale;

    return oc_error_ok;
}

oc_error oc_get_sfnt_table(const oc_face* face, oc_tag tag, oc_table* otable, void** ocontext) {
    HRESULT dw_err;
    const void* table_data;
    UINT32 table_size;
    void* context;
    WINBOOL exists;
    oc_table table = { 0 };
    oc_error err = oc_error_ok;

    if (!(face && otable && ocontext)) {
        oc__exit(oc_error_invalid_param);
    }

    dw_err = face->impl->dw_face->lpVtbl->TryGetFontTable(
        face->impl->dw_face,
        _byteswap_ulong(tag), // swapping bytes because windows table tags are little-endian
        &table_data,
        &table_size,
        &context,
        &exists);

    switch (dw_err) {
    case S_OK:
        break;
    case E_OUTOFMEMORY:
        oc__exit(oc_error_out_of_memory);
    default:
        oc__exit(oc__unexpected(err));
    }

    if (!exists) {
        oc__exit(oc_error_table_missing);
    }

    table.data = table_data;
    table.size = table_size;

    *ocontext = context;
exit:
    if (otable) *otable = table;
    return err;
}

void oc_free_table(const oc_face* face, void* context) {
    IDWriteFontFace* dw_face;

    if (!face) return;

    dw_face = face->impl->dw_face;
    dw_face->lpVtbl->ReleaseFontTable(dw_face, context);
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

void oc_get_glyph_metrics(const oc_face* face, uint16_t index, oc_load_flags flags, oc_glyph_metrics* ometrics) {
    HRESULT err;
    DWRITE_GLYPH_METRICS dw_metrics;
    IDWriteFontFace* dw_face;
    oc_16p16 scale;
    UINT16 count;
    oc_glyph_metrics metrics = { 0 };

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

    scale = face->metrics.scale;

    metrics.width = oc_mul_16p16(metrics.width, scale);
    metrics.height = oc_mul_16p16(metrics.height, scale);
    metrics.bearing_x = oc_mul_16p16(metrics.bearing_x, scale);
    metrics.bearing_y = oc_mul_16p16(metrics.bearing_y, scale);
    metrics.advance = oc_mul_16p16(metrics.advance, scale);

    // if (flags & OC_LOAD_NO_HINTING) {
    // goto done;
    //}

    // fit_metrics(&metrics);

exit:
    if (ometrics) *ometrics = metrics;
}

void oc_get_glyph_cbox(const oc_face* face, uint16_t index, oc_load_flags flags, oc_bbox* ocbox) {
    HRESULT err;
    DWRITE_GLYPH_METRICS metrics;
    IDWriteFontFace* dw_face;
    UINT16 count;
    oc_16p16 scale;
    oc_bbox cbox = { 0 };

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

    scale = face->metrics.scale;

    cbox.min_x = oc_mul_16p16(cbox.min_x, scale);
    cbox.min_y = oc_mul_16p16(cbox.min_y, scale);
    cbox.max_x = oc_mul_16p16(cbox.max_x, scale);
    cbox.max_y = oc_mul_16p16(cbox.max_y, scale);

exit:
    if (ocbox) *ocbox = cbox;
}

bool oc_get_outline(const oc_face* face, uint16_t index, const oc_outline_funcs* funcs, void* user) {
    HRESULT err;
    ULONG refs;
    OC__ID2D1SimplifiedGeometrySink geometry_sink = { 0 };

    if (!(face && funcs)) {
        return false;
    }

    geometry_sink.lpVtbl = &OC__ID2D1SimplifiedGeometrySinkVtbl;
    geometry_sink.funcs = funcs;
    geometry_sink.ref_count = 1;
    geometry_sink.ctx = user;

    err = face->impl->dw_face->lpVtbl->GetGlyphRunOutline(
        face->impl->dw_face,
        face->metrics.upem,
        &index,
        NULL,
        NULL,
        1,
        FALSE,
        FALSE,
        (IDWriteGeometrySink*)&geometry_sink);

    if (err != S_OK) {
        return false;
    }

    refs = geometry_sink.lpVtbl->Base.Release((IUnknown*)&geometry_sink);

    (void)refs;
    assert(refs == 0);

    return true;
}

// todo: check this rendering thingy as smth is a bit off with dwrite
//       it seems dwrite does hard edges, idk if we can change that
oc_error oc_render_glyph(const oc_face* face, uint16_t index, oc_extent* oextent, unsigned char* buffer, size_t buffer_size) {
    oc_error err = oc_error_ok;
    HRESULT dw_err = S_OK;

    IDWriteFontFace* dw_face;
    IDWriteFactory* dw_factory;

    oc_bbox cbox;
    oc_bbox pbox;

    DWRITE_MATRIX transform;
    UINT16 count;
    RECT bounds;

    IDWriteGlyphRunAnalysis* analysis = NULL;
    DWRITE_GLYPH_RUN glyph_run = { 0 };

    uint8_t* bitmap = NULL;
    oc_extent extent = { 0 };

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
    oc_get_glyph_cbox(face, index, OC_LOAD_DEFAULT, &cbox);

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

    if (buffer_size < extent.rows * extent.cols) {
        oc__exit(oc_error_insufficient_buffer);
    }

    transform.m11 = 1.0f;
    transform.m12 = 0.0f;
    transform.m21 = 0.0f;
    transform.m22 = 1.0f;
    transform.dx = (cbox.min_x & 63) / 64.0f;
    transform.dy = -(cbox.min_y & 63) / 64.0f;

    glyph_run.fontFace = dw_face;
    glyph_run.fontEmSize = oc_mul_16p16(face->metrics.upem, face->metrics.scale) / 64.0f;
    glyph_run.glyphCount = 1;
    glyph_run.glyphIndices = &index;

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

    bitmap = malloc(extent.rows * extent.cols * 3);
    if (bitmap == NULL) {
        oc__exit(oc_error_out_of_memory);
    }

    bounds.left = 0;
    bounds.bottom = 0;

    bounds.top = -(int32_t)extent.rows;
    bounds.right = extent.cols;

    err = analysis->lpVtbl->CreateAlphaTexture(
        analysis,
        DWRITE_TEXTURE_CLEARTYPE_3x1,
        &bounds,
        bitmap,
        extent.rows * extent.cols * 3);

    if (err != S_OK) {
        oc__exit(oc__unexpected(err));
    }

    for (uint32_t i = 0; i < extent.rows * extent.cols; i++) {
        uint8_t r = bitmap[i * 3 + 0];
        uint8_t g = bitmap[i * 3 + 1];
        uint8_t b = bitmap[i * 3 + 2];

        buffer[i] = (r + b + g) / 3.0f;
    }
exit:
    if (bitmap) free(bitmap);
    if (analysis) analysis->lpVtbl->Release(analysis);
    if (oextent) *oextent = extent;
    return err;
}
