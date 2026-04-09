#include <onecore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#define FATAL(e)                \
    do {                        \
        if ((e) != oc_error_ok) \
            exit(1);            \
    } while (0)                 \

typedef struct __attribute__((packed)) {
    uint8_t  chars;
    uint8_t  slant;
    uint16_t match;
} score_t;

static const char32_t chars[] = U"„ž“";

static uint32_t score(const oc_font* f) {
    score_t  s = { 0 };
    uint32_t u;

    for (const char32_t* ch = chars; *ch != U'\0'; ch++) {
        if (ocf_has_character(f, *ch)) s.chars++;
    }

    s.slant = 3 - (uint8_t)f->slant;
    s.match = 1000 - abs(f->weight - 400);

    memcpy(&u, &s, sizeof(u));
    return u;
}

static int compr(const void* a, const void* b) {
    const oc_font* af = *(const oc_font**)a;
    const oc_font* bf = *(const oc_font**)b;

    uint32_t sa = score(af);
    uint32_t sb = score(bf);

    return (sa > sb) - (sa < sb);
}

int main() {
    oc_library* lib;
    oc_collection col;

    oc_face aface;
    // oc_face bface;

    FATAL(oc_init_library(&lib));
    FATAL(ocf_init_collection(lib, &col));
    FATAL(ocf_load_fonts(&col));

    qsort(col.fonts, col.nfonts, sizeof(oc_font*), compr);

    FATAL(ocf_open_font(col.fonts[0], 0, 0, &aface));

    for (const char32_t* ch = chars; *ch != U'\0'; ch++) {
        uint32_t         idx;
        oc_glyph_metrics metrics;

        idx = ocl_get_char_index(&aface, *ch);
        ocl_get_glyph_metrics(&aface, idx, OC_LOAD_DEFAULT, &metrics);

        printf("w: %d, h: %d, bx: %d, by: %d, adv: %d\n", metrics.width, metrics.height, metrics.bearing_x, metrics.bearing_y, metrics.advance);
    }

    ocl_free_face(&aface);
    ocf_free_collection(&col);
    oc_free_library(lib);
}
