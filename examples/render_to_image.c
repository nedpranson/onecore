#define ONECORE_IMPLEMENTATION
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

    oc_open_params open_params = { 0 };
    open_params.desired_size = 12 << 6;
    open_params.dpi = 96;

    oc_face face;
    if ((err = oc_open_face(library, "test/files/arial.ttf", &open_params, &face))) {
        oc_free_library(library);
        printf("oc_open_face: %s\n", oc_strerror(err));
        return 1;
    }

    uint8_t canvas[64 * 128];
    memset(canvas, 0, sizeof(canvas));

    oc_26p6 baseline = 38;
    oc_26p6 advance = 0;

    const char* ch = message;
    for (; *ch; ch++) {
        uint16_t index = oc_get_char_index(face, *ch);
        if (index == 0)
            continue;

        oc_glyph_metrics metrics;
        oc_get_glyph_metrics(face, index, 0, &metrics);

        oc_size size;
        uint8_t bitmap[32 * 32];
        if ((err = oc_render_glyph(face, index, &size, bitmap, sizeof(bitmap)))) {
            oc_free_face(face);
            oc_free_library(library);
            printf("oc_render_glyph: %s\n", oc_strerror(err));
            return 1;
        }

        for (int32_t row = 0; row < (int32_t)size.rows; row++) {
            for (int32_t col = 0; col < (int32_t)size.cols; col++) {
                uint32_t y = row + baseline - (metrics.bearing_y >> 6);
                uint32_t x = col + advance + (metrics.bearing_x >> 6);

                uint8_t src = bitmap[row * size.cols + col];
                uint8_t* dst = &canvas[y * 128 + x];

                *dst = src + (*dst * (255 - src) / 255);
            }
        }

        advance += metrics.advance >> 6;
    }

    stbi_write_png("output.png", 128, 64, 1, canvas, 128);

    oc_free_face(face);
    oc_free_library(library);

    return 0;
}
