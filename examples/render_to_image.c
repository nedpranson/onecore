#define ONECORE_LOADER_IMPLEMENTATION
#include <onecore.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main() {
    // c is renderer weirdly on dwrite
    const char* message = "Travis Scott!";

    oc_error err;
    oc_library library;
    if ((err = oc_init_library(&library))) {
        printf("oc_init_library: %s\n", oc_strerror(err));
        return 1;
    }

    oc_face face;
    oc_open_params params = { 0 };
    params.dpi = 96;

    if ((err = ocl_open_face(&library, "test/files/arial.ttf", &params, &face))) {
        oc_free_library(&library);
        printf("ocl_open_face: %s\n", oc_strerror(err));
        return 1;
    }

    uint8_t canvas[64 * 128];
    memset(canvas, 0, sizeof(canvas));

    oc_26p6 baseline = 38;
    oc_26p6 advance = 0;

    const char* ch = message;
    for (; *ch; ch++) {
        uint16_t index = ocl_get_char_index(&face, *ch);
        if (index == 0)
            continue;

        oc_glyph_metrics metrics;
        ocl_get_glyph_metrics(&face, index, OC_LOAD_DEFAULT, &metrics);

        oc_extent extent;
        uint8_t bitmap[32 * 32];
        if ((err = ocl_render_glyph(&face, index, &extent, bitmap, sizeof(bitmap)))) {
            ocl_free_face(&face);
            oc_free_library(&library);
            printf("ocl_render_glyph: %s\n", oc_strerror(err));
            return 1;
        }

        for (int32_t row = 0; row < (int32_t)extent.rows; row++) {
            for (int32_t col = 0; col < (int32_t)extent.cols; col++) {
                uint32_t y = row + baseline - (metrics.bearing_y >> 6);
                uint32_t x = col + advance + (metrics.bearing_x >> 6);

                uint8_t src = bitmap[row * extent.cols + col];
                uint8_t* dst = &canvas[y * 128 + x];

                *dst = src + (*dst * (255 - src) / 255);
            }
        }

        advance += metrics.advance >> 6;
    }

    stbi_write_png("output.png", 128, 64, 1, canvas, 128);

    ocl_free_face(&face);
    oc_free_library(&library);

    return 0;
}
