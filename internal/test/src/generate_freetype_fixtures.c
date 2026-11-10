#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ONECORE_FREETYPE_LOADER_IMPLEMENTATION
#include "onecore.h"

static uint64_t hash_bytes(uint64_t hash, const void* data, size_t size) {
    const unsigned char* bytes = data;

    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}

static uint64_t hash_metrics(const oc_glyph_metrics* metrics) {
    uint64_t hash = UINT64_C(14695981039346656037);

    hash = hash_bytes(hash, &metrics->width, sizeof(metrics->width));
    hash = hash_bytes(hash, &metrics->height, sizeof(metrics->height));
    hash = hash_bytes(hash, &metrics->bearing_x, sizeof(metrics->bearing_x));
    hash = hash_bytes(hash, &metrics->bearing_y, sizeof(metrics->bearing_y));
    return hash_bytes(hash, &metrics->advance, sizeof(metrics->advance));
}

static uint64_t hash_extent(const oc_extent* extent) {
    uint64_t hash = UINT64_C(14695981039346656037);

    hash = hash_bytes(hash, &extent->rows, sizeof(extent->rows));
    return hash_bytes(hash, &extent->cols, sizeof(extent->cols));
}

static uint64_t hash_outline(const oc_outline* outline) {
    uint64_t hash = UINT64_C(14695981039346656037);

    hash = hash_bytes(hash, &outline->npoints, sizeof(outline->npoints));
    hash = hash_bytes(hash, &outline->ncontours, sizeof(outline->ncontours));
    hash = hash_bytes(hash, outline->tags, outline->npoints * sizeof(*outline->tags));
    hash = hash_bytes(hash, outline->points, outline->npoints * sizeof(*outline->points));
    return hash_bytes(hash, outline->contours, outline->ncontours * sizeof(*outline->contours));
}

static void generate(const char* path, const char* font_path, oc_load_flags flags) {
    oc_library* library;
    oc_face face;
    oc_open_params params = { 0 };
    FILE* file = fopen(path, "w");

    params.dpi = 96;
    if (oc_init_library(&library) != oc_error_ok ||
        ocl_open_face(library, font_path, &params, &face) != oc_error_ok) {
        fprintf(stderr, "failed to open reference font %s\n", font_path);
        exit(1);
    }

    if (file == NULL) {
        perror(path);
        exit(1);
    }

    fprintf(file, "codepoint,metrics,extent,outline\n");

    for (uint32_t codepoint = 0; codepoint <= 0x10ffff; codepoint++) {
        uint16_t index = ocl_get_char_index(&face, codepoint);
        oc_glyph_metrics metrics;
        oc_extent extent;
        oc_outline outline;

        if (index == 0) {
            continue;
        }

        ocl_get_glyph_metrics(&face, index, flags, &metrics);
        if (ocl_render_glyph(&face, index, &extent, NULL, 0) != oc_error_ok) {
            fclose(file);
            fprintf(stderr, "failed to get extent for U+%04" PRIX32 "\n", codepoint);
            exit(1);
        }
        if (ocl_get_outline(&face, index, flags, &outline) != oc_error_ok) {
            fclose(file);
            fprintf(stderr, "failed to get outline for U+%04" PRIX32 "\n", codepoint);
            exit(1);
        }

        fprintf(
            file,
            "%04" PRIX32 ",%016" PRIX64 ",%016" PRIX64 ",%016" PRIX64 "\n",
            codepoint,
            hash_metrics(&metrics),
            hash_extent(&extent),
            hash_outline(&outline));
        ocl_free_outline(&outline);
    }

    fclose(file);
    ocl_free_face(&face);
    oc_free_library(library);
}

int main(void) {
    generate("test/files/arial-outline-no-scale.csv", "test/files/arial.ttf", OC_LOAD_NO_SCALE);
    generate("test/files/arial-outline-no-hinting.csv", "test/files/arial.ttf", OC_LOAD_NO_HINTING);
    generate("test/files/arial-outline-no-fitting.csv", "test/files/arial.ttf", OC_LOAD_NO_FITTING);
    generate("test/files/garamond-outline-no-scale.csv", "test/files/AGaramondPro-Regular.otf", OC_LOAD_NO_SCALE);
    generate("test/files/garamond-outline-no-hinting.csv", "test/files/AGaramondPro-Regular.otf", OC_LOAD_NO_HINTING);
    generate("test/files/garamond-outline-no-fitting.csv", "test/files/AGaramondPro-Regular.otf", OC_LOAD_NO_FITTING);
    generate("test/files/source-serif-4-variable-outline-no-scale.csv", "test/files/SourceSerif4Variable-Roman.otf", OC_LOAD_NO_SCALE);
    generate("test/files/source-serif-4-variable-outline-no-hinting.csv", "test/files/SourceSerif4Variable-Roman.otf", OC_LOAD_NO_HINTING);
    generate("test/files/source-serif-4-variable-outline-no-fitting.csv", "test/files/SourceSerif4Variable-Roman.otf", OC_LOAD_NO_FITTING);
    return 0;
}
