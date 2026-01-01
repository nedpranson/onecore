#include <onecore.h>
#ifdef ONECORE_DWRITE

#include "../unexpected.h"
#include <assert.h>

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
    table.buffer = table_data;
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
