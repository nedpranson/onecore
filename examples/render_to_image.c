#define ONECORE_LOADER_IMPLEMENTATION
#include <onecore.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WIDTH 128
#define HEIGHT 32

int main() {
    // c is renderer weirdly on dwrite
    const char* message = "Travis Scott!!!";

    oc_error   err;
    oc_library* library;
    if ((err = oc_init_library(&library))) {
        printf("oc_init_library: %s\n", oc_strerror(err));
        return 1;
    }

    oc_face        face;
    oc_open_params params = { 0 };
    params.dpi = 96;

    if ((err = ocl_open_face(library, "internal/test/files/arial.ttf", &params, &face))) {
        oc_free_library(library);
        printf("ocl_open_face: %s\n", oc_strerror(err));
        return 1;
    }

    uint8_t canvas[HEIGHT * WIDTH];
    memset(canvas, 0, sizeof(canvas));

    // tood: we need scaled ones
    oc_26p6 baseline = HEIGHT - ((oc_mul_16p16(face.descent, face.size.scale) + 63) >> 6);
    oc_26p6 advance = 0;

    const char* ch = message;
    for (; *ch; ch++) {
        uint16_t index = ocl_get_char_index(&face, *ch);
        if (index == 0)
            continue;

        oc_glyph_metrics metrics;
        ocl_get_glyph_metrics(&face, index, OC_LOAD_DEFAULT, &metrics);

        oc_extent extent;
        size_t off_x = advance + (metrics.bearing_x >> 6) + 12;
        size_t off_y = baseline - (metrics.bearing_y >> 6);

        if ((err = ocl_render_glyph(&face, index, &extent, canvas + off_y * WIDTH + off_x, WIDTH))) {
            ocl_free_face(&face);
            oc_free_library(library);
            printf("ocl_render_glyph: %s\n", oc_strerror(err));
            return 1;
        }

        advance += metrics.advance >> 6;
    }

    stbi_write_png("output.png", WIDTH, HEIGHT, 1, canvas, WIDTH);

    ocl_free_face(&face);
    oc_free_library(library);

    return 0;
}
