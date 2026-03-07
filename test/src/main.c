#include <stdlib.h>
#include <string.h>
#include <unity.h>

#define ONECORE_IMPLEMENTATION
#include <onecore.h>

oc_library g_library;
oc_face g_arial_ttf;

void setUp(void) { }

void tearDown(void) { }

void test_math(void) {
    TEST_ASSERT_EQUAL_INT32(2147483647, oc_div_16p16(0, 0));
    TEST_ASSERT_EQUAL_INT32(5 << 16, oc_div_16p16(10, 2));
    TEST_ASSERT_EQUAL_INT32(1 << 16, oc_div_16p16(-2147483648, -2147483648));
    TEST_ASSERT_EQUAL_INT32(2147483647, oc_div_16p16(2147483647, 1 << 16));
    TEST_ASSERT_EQUAL_INT32(2147483646, oc_div_16p16(1073741823, 1 << 15));

    // overflow checks
    TEST_ASSERT_EQUAL_INT32(-2147483648, oc_div_16p16(1073741824, 1 << 15));
    TEST_ASSERT_EQUAL_INT32(-2, oc_div_16p16(2147483647, 1 << 15));
}

void test_oc_init_library(void) {
    oc_library lib;
    oc_error err;

    err = oc_init_library(&lib);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_library(&lib);

    err = oc_init_library(NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);
}

void test_oc_open_face(void) {
    oc_face face;
    oc_error err;

    err = oc_open_face(&g_library, "test/files/arial.ttf", NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    oc_free_face(&face);

    err = oc_open_face(&g_library, "test/files/arial.idk", NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(&face);

    err = oc_open_face(&g_library, "test/files/arial.otf", NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(&face);

    err = oc_open_face(&g_library, "test/files/arial", NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(&face);

    err = oc_open_face(&g_library, "test/files/arial.ttf", NULL, NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_open_face(&g_library, NULL, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    oc_open_params params = { 0 };
    params.face_index = 10;
    err = oc_open_face(&g_library, "test/files/arial.ttf", &params, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_open_face(&g_library, "non_existing.ttf", NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    char path[8192 + 1];
    memset(path, 'a', sizeof(path));
    path[8188] = '.';
    path[8189] = 't';
    path[8190] = 't';
    path[8191] = 'f';
    path[8192] = '\0';

    err = oc_open_face(&g_library, path, NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    const char ipath[] = { 0xC0, 0xAF, 0x00 };
    err = oc_open_face(&g_library, ipath, NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_open_face(&g_library, "", NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);
}

void test_oc_open_memory_face(void) {
    oc_face face;
    oc_error err;

    err = oc_open_memory_face(&g_library, NULL, 0, NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_open_memory_face(&g_library, NULL, 5, NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    char buf[1024];
    memset(buf, 'a', sizeof(buf));

    err = oc_open_memory_face(&g_library, buf, sizeof(buf), NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_open_memory_face(&g_library, buf, sizeof(buf), NULL, NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    FILE* file = fopen("test/files/arial.ttf", "rb");
    TEST_ASSERT_NOT_NULL(file);

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    TEST_ASSERT_EQUAL_INT(367112, size);

    char* data = malloc(size);
    TEST_ASSERT_NOT_NULL(data);

    size_t nread = fread(data, 1, size, file);
    fclose(file);

    TEST_ASSERT_EQUAL_INT(size, nread);

    err = oc_open_memory_face(&g_library, data, size, NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(&face);

    err = oc_open_memory_face(&g_library, data, size - 20, NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(&face);

    oc_open_params params = { 0 };
    params.face_index = 10;
    err = oc_open_memory_face(&g_library, data, 0, &params, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_open_memory_face(&g_library, data, 0, NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_open_memory_face(&g_library, data, size, &params, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    free(data);
}

void test_oc_test_sizes(void) {
    oc_error err;
    oc_face face;
    uint16_t idx;
    oc_extent extent;
    uint8_t bitmap_a[13 * 12];
    uint8_t bitmap_b[13 * 12];

    err = oc_open_face(&g_library, "test/files/arial.otf", NULL, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(12, face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(24576, face.metrics.scale);

    err = oc_set_size(&face, 16 << 6, 128);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(28, face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(58240, face.metrics.scale);

    err = oc_set_size(&face, 10.5f * 64, 96);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(14, face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(28672, face.metrics.scale);

    err = oc_set_size(&face, 12 << 6, -1);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_set_size(&face, 12 << 6, 0);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(12, face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(24576, face.metrics.scale);

    err = oc_set_size(&face, 12 << 6, 72);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(12, face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(24576, face.metrics.scale);

    err = oc_set_size(&face, -10.5f * 64, 96);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_set_size(&face, 0, 72);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_set_size(&face, 1 << 6, 72);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(1, face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(2048, face.metrics.scale);

    err = oc_set_size(&face, 2 << 6, 72);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(2, face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(4096, face.metrics.scale);

    err = oc_set_size(&face, 65535 << 6, 0);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(65535 , face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(134215680, face.metrics.scale);

    err = oc_set_size(&face, 65536 << 6, 0);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_set_size(&face, 12 << 6, 96);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(16, face.metrics.ppem);
    TEST_ASSERT_EQUAL_INT32(32768, face.metrics.scale);

    idx = oc_get_char_index(&g_arial_ttf, 'G');
    TEST_ASSERT_EQUAL_INT16(42, idx);

    oc_render_glyph(&g_arial_ttf, idx, &extent, bitmap_a, sizeof(bitmap_a));
    TEST_ASSERT_EQUAL_UINT32(13, extent.rows);
    TEST_ASSERT_EQUAL_UINT32(12, extent.cols);

    idx = oc_get_char_index(&face, 'G');
    TEST_ASSERT_EQUAL_INT16(42, idx);

    oc_render_glyph(&face, idx, &extent, bitmap_b, sizeof(bitmap_b));
    TEST_ASSERT_EQUAL_UINT32(13, extent.rows);
    TEST_ASSERT_EQUAL_UINT32(12, extent.cols);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(bitmap_a, bitmap_b, extent.rows * extent.cols);

    oc_free_face(&face);
}

void test_oc_get_char_index(void) {
    oc_face face;
    oc_error err;
    uint16_t idx;

    err = oc_open_face(&g_library, "test/files/arial.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    idx = oc_get_char_index(&face, 'A');
    TEST_ASSERT_EQUAL_INT16(36, idx);

    idx = oc_get_char_index(&face, 0);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_get_char_index(&face, 0xE000);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_get_char_index(&face, 0x110000);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_get_char_index(&face, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_INT16(0, idx);
    oc_free_face(&face);

    err = oc_open_face(&g_library, "test/files/emoji.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    idx = oc_get_char_index(&face, 0x1F600);
    TEST_ASSERT_EQUAL_INT16(1076, idx);

    oc_free_face(&face);
}

void test_oc_get_sfnt_table(void) {
    oc_table table;
    oc_error err;
    void* context;

    err = oc_get_sfnt_table(&g_arial_ttf, OC_MAKE_TAG('c', 'm', 'a', 'p'), NULL, NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_get_sfnt_table(&g_arial_ttf, OC_MAKE_TAG('c', 'm', 'a', 'p'), &table, NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_get_sfnt_table(&g_arial_ttf, OC_MAKE_TAG('c', 'm', 'a', 'p'), NULL, &context);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_get_sfnt_table(&g_arial_ttf, OC_MAKE_TAG('c', 'm', 'a', 'p'), &table, &context);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_size_t(5994, table.size);
    oc_free_table(&g_arial_ttf, context);

    err = oc_get_sfnt_table(&g_arial_ttf, OC_MAKE_TAG('u', 'n', 'k', 'n'), &table, &context);
    TEST_ASSERT_EQUAL(oc_error_table_missing, err);
}

void test_oc_font_metrics(void) {
    oc_face face;
    oc_error err;

    err = oc_open_face(&g_library, "test/files/arial.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    TEST_ASSERT_EQUAL_UINT16(2048, face.metrics.upem);
    TEST_ASSERT_EQUAL_UINT16(1854, face.metrics.ascent);
    TEST_ASSERT_EQUAL_UINT16(434, face.metrics.descent);
    TEST_ASSERT_EQUAL_INT16(67, face.metrics.leading);
    TEST_ASSERT_EQUAL_INT16(-217, face.metrics.underline_position);
    TEST_ASSERT_EQUAL_UINT16(150, face.metrics.underline_thickness);
    oc_free_face(&face);

    err = oc_open_face(&g_library, "test/files/source-serif.otf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    TEST_ASSERT_EQUAL_UINT16(1000, face.metrics.upem);
    TEST_ASSERT_EQUAL_UINT16(1036, face.metrics.ascent);
    TEST_ASSERT_EQUAL_UINT16(335, face.metrics.descent);
    TEST_ASSERT_EQUAL_INT16(0, face.metrics.leading);
    TEST_ASSERT_EQUAL_INT16(-50, face.metrics.underline_position);
    TEST_ASSERT_EQUAL_UINT16(50, face.metrics.underline_thickness);
    oc_free_face(&face);

    err = oc_open_face(&g_library, "test/files/roman.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    TEST_ASSERT_EQUAL_UINT16(1000, face.metrics.upem);
    TEST_ASSERT_EQUAL_UINT16(878, face.metrics.ascent);
    TEST_ASSERT_EQUAL_UINT16(250, face.metrics.descent);
    TEST_ASSERT_EQUAL_INT16(0, face.metrics.leading);
    TEST_ASSERT_EQUAL_INT16(-100, face.metrics.underline_position);
    TEST_ASSERT_EQUAL_UINT16(50, face.metrics.underline_thickness);
    oc_free_face(&face);
}

void test_oc_get_glyph_metrics(void) {
    uint16_t idx;
    oc_glyph_metrics metrics;

    idx = oc_get_char_index(&g_arial_ttf, 'y');
    TEST_ASSERT_EQUAL_INT16(92, idx);

    oc_get_glyph_metrics(&g_arial_ttf, idx, OC_LOAD_NO_SCALE, NULL);

    oc_get_glyph_metrics(&g_arial_ttf, idx, OC_LOAD_NO_SCALE, &metrics);

    TEST_ASSERT_EQUAL_UINT32(973, metrics.width);
    TEST_ASSERT_EQUAL_UINT32(1493, metrics.height);
    TEST_ASSERT_EQUAL_INT32(33, metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(1062, metrics.bearing_y);
    TEST_ASSERT_EQUAL_UINT32(1024, metrics.advance);

    idx = oc_get_char_index(&g_arial_ttf, 'g');
    TEST_ASSERT_EQUAL_INT16(74, idx);

    oc_get_glyph_metrics(&g_arial_ttf, idx, OC_LOAD_NO_SCALE, &metrics);

    TEST_ASSERT_EQUAL_UINT32(936, metrics.width);
    TEST_ASSERT_EQUAL_UINT32(1517, metrics.height);
    TEST_ASSERT_EQUAL_INT32(66, metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(1086, metrics.bearing_y);
    TEST_ASSERT_EQUAL_UINT32(1139, metrics.advance);

    idx = oc_get_char_index(&g_arial_ttf, 'M');
    TEST_ASSERT_EQUAL_INT16(48, idx);

    oc_get_glyph_metrics(&g_arial_ttf, idx, OC_LOAD_NO_SCALE, &metrics);

    TEST_ASSERT_EQUAL_UINT32(1399, metrics.width);
    TEST_ASSERT_EQUAL_UINT32(1466, metrics.height);
    TEST_ASSERT_EQUAL_INT32(152, metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(1466, metrics.bearing_y);
    TEST_ASSERT_EQUAL_UINT32(1706, metrics.advance);

    oc_get_glyph_metrics(&g_arial_ttf, 4444, OC_LOAD_NO_SCALE, &metrics);
    TEST_ASSERT_EQUAL_UINT32(0, metrics.width);
    TEST_ASSERT_EQUAL_UINT32(0, metrics.height);
    TEST_ASSERT_EQUAL_INT32(0, metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(0, metrics.bearing_y);
    TEST_ASSERT_EQUAL_UINT32(0, metrics.advance);

    oc_face face;
    oc_open_params face_params = { 0 };
    face_params.desired_size = 16 << 6;
    face_params.dpi = 96;

    oc_error err = oc_open_face(&g_library, "test/files/arial.ttf", &face_params, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    idx = oc_get_char_index(&g_arial_ttf, 'e');
    TEST_ASSERT_EQUAL_INT16(72, idx);

    oc_get_glyph_metrics(&face, idx, OC_LOAD_NO_SCALE, &metrics);
    TEST_ASSERT_EQUAL_UINT32(979, metrics.width);
    TEST_ASSERT_EQUAL_UINT32(1110, metrics.height);
    TEST_ASSERT_EQUAL_INT32(75, metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(1086, metrics.bearing_y);
    TEST_ASSERT_EQUAL_UINT32(1139, metrics.advance);
}

void test_oc_get_glyph_metrics_scaled(void) {
    uint16_t idx;
    oc_glyph_metrics metrics;

    idx = oc_get_char_index(&g_arial_ttf, '_');
    TEST_ASSERT_EQUAL_INT16(66, idx);

    oc_get_glyph_metrics(&g_arial_ttf, idx, OC_LOAD_DEFAULT, &metrics);
    TEST_ASSERT_EQUAL_INT32(597, metrics.width);
    TEST_ASSERT_EQUAL_INT32(65, metrics.height);
    TEST_ASSERT_EQUAL_INT32(-16, metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(-139, metrics.bearing_y);
    TEST_ASSERT_EQUAL_INT32(570, metrics.advance);

    idx = oc_get_char_index(&g_arial_ttf, 'M');
    TEST_ASSERT_EQUAL_INT16(48, idx);

    oc_get_glyph_metrics(&g_arial_ttf, idx, OC_LOAD_DEFAULT, &metrics);
    TEST_ASSERT_EQUAL_INT32(700, metrics.width);
    TEST_ASSERT_EQUAL_INT32(733, metrics.height);
    TEST_ASSERT_EQUAL_INT32(76, metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(733, metrics.bearing_y);
    TEST_ASSERT_EQUAL_INT32(853, metrics.advance);

    oc_get_glyph_metrics(&g_arial_ttf, 4444, OC_LOAD_DEFAULT, &metrics);
    TEST_ASSERT_EQUAL_INT32(0, metrics.width);
    TEST_ASSERT_EQUAL_INT32(0, metrics.height);
    TEST_ASSERT_EQUAL_INT32(0, metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(0, metrics.bearing_y);
    TEST_ASSERT_EQUAL_INT32(0, metrics.advance);
}

// void test_oc_get_glyph_metrics_hinted(void) {
//     uint16_t idx;
//     oc_glyph_metrics metrics;
//
//     idx = oc_get_char_index(g_arial_ttf, '_');
//     TEST_ASSERT_EQUAL_INT16(66, idx);
//
//     oc_get_glyph_metrics(g_arial_ttf, idx, OC_LOAD_DEFAULT, &metrics);
//     TEST_ASSERT_EQUAL_INT32(704, metrics.width);
//     TEST_ASSERT_EQUAL_INT32(64, metrics.height);
//     TEST_ASSERT_EQUAL_INT32(-64, metrics.bearing_x);
//     TEST_ASSERT_EQUAL_INT32(-128, metrics.bearing_y);
//     TEST_ASSERT_EQUAL_INT32(576, metrics.advance);
//
//     idx = oc_get_char_index(g_arial_ttf, 'M');
//     TEST_ASSERT_EQUAL_INT16(48, idx);
//
//     oc_get_glyph_metrics(g_arial_ttf, idx, OC_LOAD_DEFAULT, &metrics);
//     TEST_ASSERT_EQUAL_INT32(768, metrics.width);
//     TEST_ASSERT_EQUAL_INT32(768, metrics.height);
//     TEST_ASSERT_EQUAL_INT32(64, metrics.bearing_x);
//     TEST_ASSERT_EQUAL_INT32(768, metrics.bearing_y);
//     TEST_ASSERT_EQUAL_INT32(832, metrics.advance);
//
//     oc_get_glyph_metrics(g_arial_ttf, 4444, OC_LOAD_DEFAULT, &metrics);
//     TEST_ASSERT_EQUAL_INT32(0, metrics.width);
//     TEST_ASSERT_EQUAL_INT32(0, metrics.height);
//     TEST_ASSERT_EQUAL_INT32(0, metrics.bearing_x);
//     TEST_ASSERT_EQUAL_INT32(0, metrics.bearing_y);
//     TEST_ASSERT_EQUAL_INT32(0, metrics.advance);
// }

void test_oc_get_glyph_cbox(void) {
    uint16_t idx;
    oc_bbox bbox;

    idx = oc_get_char_index(&g_arial_ttf, '_');
    TEST_ASSERT_EQUAL_INT16(66, idx);

    oc_get_glyph_cbox(&g_arial_ttf, idx, OC_LOAD_DEFAULT, &bbox);
    TEST_ASSERT_EQUAL_INT32(-16, bbox.min_x);
    TEST_ASSERT_EQUAL_INT32(-204, bbox.min_y);
    TEST_ASSERT_EQUAL_INT32(581, bbox.max_x);
    TEST_ASSERT_EQUAL_INT32(-139, bbox.max_y);

    idx = oc_get_char_index(&g_arial_ttf, 'e');
    TEST_ASSERT_EQUAL_INT16(72, idx);

    oc_get_glyph_cbox(&g_arial_ttf, idx, OC_LOAD_DEFAULT, &bbox);
    TEST_ASSERT_EQUAL_INT32(38, bbox.min_x);
    TEST_ASSERT_EQUAL_INT32(-12, bbox.min_y);
    TEST_ASSERT_EQUAL_INT32(527, bbox.max_x);
    TEST_ASSERT_EQUAL_INT32(543, bbox.max_y);
}

// void test_oc_get_glyph_bbox_hinted(void) {
//     uint16_t idx;
//     oc_bbox bbox;
//
//     idx = oc_get_char_index(g_arial_ttf, '_');
//     TEST_ASSERT_EQUAL_INT16(66, idx);
//
//     oc_get_glyph_cbox(g_arial_ttf, idx, OC_LOAD_DEFAULT, &bbox);
//     TEST_ASSERT_EQUAL_INT32(-16, bbox.min_x);
//     TEST_ASSERT_EQUAL_INT32(-192, bbox.min_y);
//     TEST_ASSERT_EQUAL_INT32(581, bbox.max_x);
//     TEST_ASSERT_EQUAL_INT32(-128, bbox.max_y);
//
//     idx = oc_get_char_index(g_arial_ttf, 'e');
//     TEST_ASSERT_EQUAL_INT16(72, idx);
//
//     oc_get_glyph_cbox(g_arial_ttf, idx, OC_LOAD_DEFAULT, &bbox);
//     TEST_ASSERT_EQUAL_INT32(38, bbox.min_x);
//     TEST_ASSERT_EQUAL_INT32(0, bbox.min_y);
//     TEST_ASSERT_EQUAL_INT32(527, bbox.max_x);
//     TEST_ASSERT_EQUAL_INT32(576, bbox.max_y);
// }

typedef struct outline_end_check {
    oc_point* line_point;
    oc_point* figure_point;
    oc_point* cubic_point;
} outline_end_check;

typedef struct outline_context {
    oc_point* line_points;
    oc_point* line_points_end;

    oc_point* figure_points;
    oc_point* figure_points_end;

    oc_point* cubic_points;
    oc_point* cubic_points_end;

    outline_end_check* checks;
    outline_end_check* checks_end;
} outline_context;

static void start_figure(oc_point at, void* context) {
    outline_context* ctx = (outline_context*)context;
    TEST_ASSERT_NOT_EQUAL(ctx->line_points_end, ctx->line_points);

    oc_point test_figure = *ctx->figure_points++;
    TEST_ASSERT_INT16_WITHIN(1, test_figure.x, at.x);
    TEST_ASSERT_INT16_WITHIN(1, test_figure.y, at.y);
}

static void end_figure(void* context) {
    outline_context* ctx = (outline_context*)context;
    TEST_ASSERT_NOT_EQUAL(ctx->checks_end, ctx->checks);

    outline_end_check check = *(ctx->checks++);
    TEST_ASSERT_EQUAL(check.line_point, ctx->line_points);
    TEST_ASSERT_EQUAL(check.figure_point, ctx->figure_points);
    TEST_ASSERT_EQUAL(check.cubic_point, ctx->cubic_points);
}

static void line_to(oc_point to, void* context) {
    outline_context* ctx = (outline_context*)context;
    TEST_ASSERT_NOT_EQUAL(ctx->line_points_end, ctx->line_points);

    oc_point test_to = *ctx->line_points++;
    TEST_ASSERT_INT16_WITHIN(1, test_to.x, to.x);
    TEST_ASSERT_INT16_WITHIN(1, test_to.y, to.y);
}

static void
cubic_to(oc_point c1, oc_point c2, oc_point to, void* context) {
    outline_context* ctx = (outline_context*)context;
    TEST_ASSERT_NOT_EQUAL(ctx->cubic_points_end, ctx->cubic_points);
    // printf("cubic_to: c1(%d %d) c2(%d %d) to(%d %d)\n",
    // c1.x, c1.y, c2.x, c2.y, to.x, to.y);

    oc_point test_c1 = *ctx->cubic_points++;
    TEST_ASSERT_INT16_WITHIN(1, test_c1.x, c1.x);
    TEST_ASSERT_INT16_WITHIN(1, test_c1.y, c1.y);

    oc_point test_c2 = *ctx->cubic_points++;
    TEST_ASSERT_INT16_WITHIN(1, test_c2.x, c2.x);
    TEST_ASSERT_INT16_WITHIN(1, test_c2.y, c2.y);

    oc_point test_to = *ctx->cubic_points++;
    TEST_ASSERT_INT16_WITHIN(1, test_to.x, to.x);
    TEST_ASSERT_INT16_WITHIN(1, test_to.y, to.y);
}

void test_oc_get_outline(void) {
    static const oc_outline_funcs funcs = {
        start_figure,
        end_figure,
        line_to,
        cubic_to
    };

    uint16_t idx;
    bool ok;
    outline_context ctx;

    idx = oc_get_char_index(&g_arial_ttf, 'i');
    TEST_ASSERT_EQUAL_INT16(76, idx);

    ok = oc_get_outline(&g_arial_ttf, idx, NULL, NULL);
    TEST_ASSERT_EQUAL(ok, false);

    ok = oc_get_outline(&g_arial_ttf, 4444, NULL, NULL);
    TEST_ASSERT_EQUAL(ok, false);

    oc_point line_points1[8] = {
        { 136, 1466 },
        { 316, 1466 },
        { 316, 1259 },
        { 136, 1259 },

        { 136, 1062 },
        { 316, 1062 },
        { 316, 0 },
        { 136, 0 }
    };

    oc_point figure_points1[2] = {
        { 136, 1259 },
        { 136, 0 },
    };

    outline_end_check checks1[2] = {
        { (line_points1 + 4), (figure_points1 + 1), NULL },
        { (line_points1 + 8), (figure_points1 + 2), NULL },
    };

    memset(&ctx, 0, sizeof(ctx));
    ctx.line_points = line_points1;
    ctx.line_points_end = line_points1 + 8;
    ctx.figure_points = figure_points1;
    ctx.figure_points_end = figure_points1 + 2;
    ctx.checks = checks1;
    ctx.checks_end = checks1 + 2;

    ok = oc_get_outline(&g_arial_ttf, idx, &funcs, &ctx);
    TEST_ASSERT_EQUAL(ok, true);
    TEST_ASSERT_EQUAL(ctx.checks_end, ctx.checks);

    idx = oc_get_char_index(&g_arial_ttf, 'S');
    TEST_ASSERT_EQUAL_INT16(54, idx);

    oc_point line_points2[2] = {
        { 275, 487 },
        { 1029, 1039 }
    };

    oc_point figure_points2[1] = {
        { 92, 471 },
    };

    oc_point cubic_points2[33 * 3] = {
        { 283, 413 }, { 303, 353 }, { 335, 306 },
        { 367, 259 }, { 416, 221 }, { 483, 192 },
        { 549, 163 }, { 624, 149 }, { 708, 149 },
        { 782, 149 }, { 847, 160 }, { 904, 182 },
        { 960, 204 }, { 1002, 234 }, { 1030, 272 },
        { 1058, 310 }, { 1072, 352 }, { 1072, 398 },
        { 1072, 444 }, { 1058, 484 }, { 1032, 518 },
        { 1005, 552 }, { 961, 581 }, { 900, 605 },
        { 860, 620 }, { 773, 644 }, { 639, 676 },
        { 504, 708 }, { 410, 739 }, { 356, 768 },
        { 286, 804 }, { 233, 850 }, { 199, 904 },
        { 165, 958 }, { 148, 1019 }, { 148, 1087 },
        { 148, 1161 }, { 169, 1230 }, { 211, 1294 },
        { 253, 1358 }, { 314, 1407 }, { 395, 1441 },
        { 475, 1474 }, { 565, 1491 }, { 664, 1491 },
        { 772, 1491 }, { 868, 1473 }, { 951, 1438 },
        { 1034, 1403 }, { 1098, 1352 }, { 1143, 1284 },
        { 1187, 1216 }, { 1211, 1139 }, { 1215, 1053 },
        { 1019, 1131 }, { 985, 1201 }, { 927, 1249 },
        { 869, 1296 }, { 784, 1320 }, { 672, 1320 },
        { 554, 1320 }, { 469, 1298 }, { 415, 1255 },
        { 361, 1212 }, { 335, 1160 }, { 335, 1100 },
        { 335, 1047 }, { 354, 1004 }, { 392, 970 },
        { 429, 936 }, { 526, 901 }, { 684, 865 },
        { 842, 829 }, { 950, 798 }, { 1009, 772 },
        { 1094, 732 }, { 1157, 682 }, { 1198, 622 },
        { 1238, 562 }, { 1259, 492 }, { 1259, 414 },
        { 1259, 336 }, { 1236, 262 }, { 1192, 193 },
        { 1147, 124 }, { 1083, 70 }, { 999, 32 },
        { 915, -5 }, { 821, -25 }, { 717, -25 },
        { 584, -25 }, { 473, -5 }, { 383, 33 },
        { 293, 71 }, { 223, 129 }, { 172, 207 },
        { 121, 285 }, { 94, 373 }, { 92, 471 }
    };

    outline_end_check checks2[1] = {
        { line_points2 + 2, figure_points2 + 1, cubic_points2 + 33 * 3 },
    };

    memset(&ctx, 0, sizeof(ctx));
    ctx.line_points = line_points2;
    ctx.line_points_end = line_points2 + 2;
    ctx.figure_points = figure_points2;
    ctx.figure_points_end = figure_points2 + 1;
    ctx.checks = checks2;
    ctx.checks_end = checks2 + 1;
    ctx.cubic_points = cubic_points2;
    ctx.cubic_points_end = cubic_points2 + 33 * 3;

    ok = oc_get_outline(&g_arial_ttf, idx, &funcs, &ctx);
    TEST_ASSERT_EQUAL(ok, true);
    TEST_ASSERT_EQUAL(ctx.checks_end, ctx.checks);
}

// todo: make everything backend indipendent!
// todo: perhaps we should render glyphs like in macos
//       specify origins, allow for a matrix if no matrix is passed we can use default (0, 0) point rendering
void test_oc_render_glyph(void) {
    uint16_t idx;
    oc_extent size;
    oc_error err;

    uint8_t buffer[12 * 3];

    idx = oc_get_char_index(&g_arial_ttf, '!');
    TEST_ASSERT_EQUAL_INT16(4, idx);

    err = oc_render_glyph(&g_arial_ttf, idx, &size, NULL, 0);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT32(12, size.rows);
    TEST_ASSERT_EQUAL_UINT32(3, size.cols);

    err = oc_render_glyph(&g_arial_ttf, idx, &size, buffer, 10);
    TEST_ASSERT_EQUAL(oc_error_insufficient_buffer, err);

    err = oc_render_glyph(&g_arial_ttf, idx, NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    size.rows = 0;
    size.cols = 0;

    err = oc_render_glyph(&g_arial_ttf, idx, &size, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT32(12, size.rows);
    TEST_ASSERT_EQUAL_UINT32(3, size.cols);

    idx = oc_get_char_index(&g_arial_ttf, ' ');
    TEST_ASSERT_EQUAL_INT16(3, idx);

    err = oc_render_glyph(&g_arial_ttf, idx, &size, NULL, 0);
    TEST_ASSERT_EQUAL(err, oc_error_ok);
    TEST_ASSERT_EQUAL_UINT32(0, size.rows);
    TEST_ASSERT_EQUAL_UINT32(0, size.cols);

    uint8_t buf[8];
    err = oc_render_glyph(&g_arial_ttf, idx, &size, buf, 0);
    TEST_ASSERT_EQUAL(err, oc_error_ok);

    err = oc_render_glyph(&g_arial_ttf, idx, &size, buf, sizeof(buf));
    TEST_ASSERT_EQUAL(err, oc_error_ok);

    err = oc_render_glyph(&g_arial_ttf, 4444, &size, NULL, 0);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    idx = oc_get_char_index(&g_arial_ttf, 'l');
    TEST_ASSERT_EQUAL_INT16(79, idx);

    err = oc_render_glyph(&g_arial_ttf, idx, &size, NULL, 0);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT32(12, size.rows);
    TEST_ASSERT_EQUAL_UINT32(2, size.cols);

    idx = oc_get_char_index(&g_arial_ttf, 'c');
    TEST_ASSERT_EQUAL_INT16(70, idx);

    err = oc_render_glyph(&g_arial_ttf, idx, &size, NULL, 0);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT32(10, size.rows);
    TEST_ASSERT_EQUAL_UINT32(8, size.cols);
}

int main(void) {
    UNITY_BEGIN();

    oc_error err;

    err = oc_init_library(&g_library);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    oc_open_params face_params = { 0 };
    face_params.dpi = 96;

    err = oc_open_face(&g_library, "test/files/arial.ttf", &face_params, &g_arial_ttf);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_UINT16(16, g_arial_ttf.metrics.ppem);

    RUN_TEST(test_math);
    RUN_TEST(test_oc_init_library);
    RUN_TEST(test_oc_open_face);
    RUN_TEST(test_oc_open_memory_face);
    RUN_TEST(test_oc_test_sizes);
    RUN_TEST(test_oc_get_char_index);
    RUN_TEST(test_oc_get_sfnt_table);
    RUN_TEST(test_oc_font_metrics);
    RUN_TEST(test_oc_get_glyph_metrics);
    RUN_TEST(test_oc_get_glyph_metrics_scaled);
    //RUN_TEST(test_oc_get_glyph_metrics_hinted);
    RUN_TEST(test_oc_get_glyph_cbox);
    //RUN_TEST(test_oc_get_glyph_bbox_hinted);
    RUN_TEST(test_oc_get_outline);
    RUN_TEST(test_oc_render_glyph);

    oc_free_face(&g_arial_ttf);
    oc_free_library(&g_library);

    UNITY_END();
    return 0;
}
