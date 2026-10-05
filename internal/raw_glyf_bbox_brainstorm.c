#include <stdint.h>
#include <stddef.h>

#include <dwrite.h>

static uint16_t
read_u16_be(const uint8_t* p) {
    return ((uint16_t)p[0] << 8) | p[1];
}

static int16_t
read_i16_be(const uint8_t* p) {
    return (int16_t)read_u16_be(p);
}

static uint32_t
read_u32_be(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           p[3];
}

/*
 * Brainstorm helper:
 * Reads the raw glyf header for one glyph without allocating or copying
 * the head, loca, or glyf tables.
 *
 * The returned bbox values are raw font units from the glyf table.
 * The table pointers are borrowed and are released before returning.
 */
static HRESULT
try_get_raw_glyf_bbox(
    IDWriteFontFace* face,
    uint16_t         glyph_index,
    int16_t*         min_x,
    int16_t*         min_y,
    int16_t*         max_x,
    int16_t*         max_y) {
    const void* head_data = NULL;
    const void* loca_data = NULL;
    const void* glyf_data = NULL;

    UINT32 head_size = 0;
    UINT32 loca_size = 0;
    UINT32 glyf_size = 0;

    void* head_context = NULL;
    void* loca_context = NULL;
    void* glyf_context = NULL;

    WINBOOL exists = FALSE;
    HRESULT hr = E_FAIL;
    uint32_t glyph_offset;
    uint32_t next_glyph_offset;

    if (!face || !min_x || !min_y || !max_x || !max_y) {
        return E_INVALIDARG;
    }

    hr = face->lpVtbl->TryGetFontTable(
        face,
        DWRITE_MAKE_OPENTYPE_TAG('h', 'e', 'a', 'd'),
        &head_data,
        &head_size,
        &head_context,
        &exists);
    if (FAILED(hr) || !exists || head_size < 52) {
        goto exit;
    }

    hr = face->lpVtbl->TryGetFontTable(
        face,
        DWRITE_MAKE_OPENTYPE_TAG('l', 'o', 'c', 'a'),
        &loca_data,
        &loca_size,
        &loca_context,
        &exists);
    if (FAILED(hr) || !exists) {
        goto exit;
    }

    hr = face->lpVtbl->TryGetFontTable(
        face,
        DWRITE_MAKE_OPENTYPE_TAG('g', 'l', 'y', 'f'),
        &glyf_data,
        &glyf_size,
        &glyf_context,
        &exists);
    if (FAILED(hr) || !exists) {
        goto exit;
    }

    const uint8_t* head = (const uint8_t*)head_data;
    const uint8_t* loca = (const uint8_t*)loca_data;
    const uint8_t* glyf = (const uint8_t*)glyf_data;
    int16_t index_to_loc_format = read_i16_be(head + 50);

    if (index_to_loc_format == 0) {
        size_t offset = (size_t)glyph_index * 2;
        if (offset > loca_size || loca_size - offset < 4) {
            hr = E_INVALIDARG;
            goto exit;
        }

        glyph_offset = (uint32_t)read_u16_be(loca + offset) * 2;
        next_glyph_offset =
            (uint32_t)read_u16_be(loca + offset + 2) * 2;
    } else if (index_to_loc_format == 1) {
        size_t offset = (size_t)glyph_index * 4;
        if (offset > loca_size || loca_size - offset < 8) {
            hr = E_INVALIDARG;
            goto exit;
        }

        glyph_offset = read_u32_be(loca + offset);
        next_glyph_offset = read_u32_be(loca + offset + 4);
    } else {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (glyph_offset > next_glyph_offset ||
        next_glyph_offset > glyf_size ||
        next_glyph_offset - glyph_offset < 10) {
        hr = E_INVALIDARG;
        goto exit;
    }

    const uint8_t* glyph = glyf + glyph_offset;
    *min_x = read_i16_be(glyph + 2);
    *min_y = read_i16_be(glyph + 4);
    *max_x = read_i16_be(glyph + 6);
    *max_y = read_i16_be(glyph + 8);
    hr = S_OK;

exit:
    if (glyf_context) {
        face->lpVtbl->ReleaseFontTable(face, glyf_context);
    }
    if (loca_context) {
        face->lpVtbl->ReleaseFontTable(face, loca_context);
    }
    if (head_context) {
        face->lpVtbl->ReleaseFontTable(face, head_context);
    }
    return hr;
}
