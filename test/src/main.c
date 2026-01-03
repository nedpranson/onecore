#include <onecore.h>
#include <unity.h>

oc_library g_library;
oc_face g_arial_ttf;

void setUp(void) { }

void tearDown(void) { }

void test_oc_init_library(void) {
    oc_library lib;
    oc_error err;

    err = oc_init_library(&lib);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_library(lib);

    err = oc_init_library(NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);
}

void test_oc_open_face(void) {
    oc_face face;
    oc_error err;

    err = oc_open_face(g_library, "test/files/arial.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(face);

    err = oc_open_face(g_library, "test/files/arial.idk", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(face);

    err = oc_open_face(g_library, "test/files/arial.otf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(face);

    err = oc_open_face(g_library, "test/files/arial", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(face);

    err = oc_open_face(g_library, "test/files/arial.ttf", 0, NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_open_face(g_library, NULL, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_open_face(g_library, "test/files/arial.ttf", 10, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_open_face(g_library, "non_existing.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    char path[8192 + 1];
    memset(path, 'a', sizeof(path));
    path[8188] = '.';
    path[8189] = 't';
    path[8190] = 't';
    path[8191] = 'f';
    path[8192] = '\0';

    err = oc_open_face(g_library, path, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    const char ipath[] = { 0xC0, 0xAF, 0x00 };
    err = oc_open_face(g_library, ipath, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_open_face(g_library, "", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);
}

void test_oc_open_memory_face(void) {
    oc_face face;
    oc_error err;

    err = oc_open_memory_face(g_library, NULL, 0, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_open_memory_face(g_library, NULL, 5, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    char buf[1024];
    memset(buf, 'a', sizeof(buf));

    err = oc_open_memory_face(g_library, buf, sizeof(buf), 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_open_memory_face(g_library, buf, sizeof(buf), 0, NULL);
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

    err = oc_open_memory_face(g_library, data, size, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(face);

    err = oc_open_memory_face(g_library, data, size - 20, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(face);

    err = oc_open_memory_face(g_library, data, 0, 100, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_open_memory_face(g_library, data, 0, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_open_memory_face(g_library, data, size, 10, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    free(data);
}

void test_oc_get_char_index(void) {
    oc_face face;
    oc_error err;
    uint16_t idx;

    err = oc_open_face(g_library, "test/files/arial.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    idx = oc_get_char_index(face, 'A');
    TEST_ASSERT_EQUAL_INT16(36, idx);

    idx = oc_get_char_index(face, 0);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_get_char_index(face, 0xE000);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_get_char_index(face, 0x110000);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_get_char_index(face, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_INT16(0, idx);
    oc_free_face(face);

    err = oc_open_face(g_library, "test/files/emoji.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    idx = oc_get_char_index(face, 0x1F600);
    TEST_ASSERT_EQUAL_INT16(1076, idx);

    oc_free_face(face);
}

void test_oc_get_sfnt_table(void) {
    oc_table table;
    oc_error err;

    err = oc_get_sfnt_table(g_arial_ttf, OC_MAKE_TAG('c', 'm', 'a', 'p'), NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_get_sfnt_table(g_arial_ttf, OC_MAKE_TAG('c', 'm', 'a', 'p'), &table);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_size_t(5994, table.size);
    oc_free_table(g_arial_ttf, table);

    err = oc_get_sfnt_table(g_arial_ttf, OC_MAKE_TAG('u', 'n', 'k', 'n'), &table);
    TEST_ASSERT_EQUAL(oc_error_table_missing, err);
}

void test_oc_get_metrics(void) {
    oc_face face;
    oc_metrics metrics;
    oc_error err;

    err = oc_open_face(g_library, "test/files/arial.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    oc_get_metrics(face, &metrics);
    TEST_ASSERT_EQUAL_UINT16(2048, metrics.units_per_em);
    TEST_ASSERT_EQUAL_UINT16(1854, metrics.ascent);
    TEST_ASSERT_EQUAL_UINT16(434, metrics.descent);
    TEST_ASSERT_EQUAL_INT16(67, metrics.leading);
    TEST_ASSERT_EQUAL_INT16(-217, metrics.underline_position);
    TEST_ASSERT_EQUAL_UINT16(150, metrics.underline_thickness);
    oc_free_face(face);

    err = oc_open_face(g_library, "test/files/source-serif.otf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    oc_get_metrics(face, &metrics);
    TEST_ASSERT_EQUAL_UINT16(1000, metrics.units_per_em);
    TEST_ASSERT_EQUAL_UINT16(1036, metrics.ascent);
    TEST_ASSERT_EQUAL_UINT16(335, metrics.descent);
    TEST_ASSERT_EQUAL_INT16(0, metrics.leading);
    TEST_ASSERT_EQUAL_INT16(-50, metrics.underline_position);
    TEST_ASSERT_EQUAL_UINT16(50, metrics.underline_thickness);
    oc_free_face(face);

    err = oc_open_face(g_library, "test/files/roman.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    oc_get_metrics(face, &metrics);
    TEST_ASSERT_EQUAL_UINT16(1000, metrics.units_per_em);
    TEST_ASSERT_EQUAL_UINT16(878, metrics.ascent);
    TEST_ASSERT_EQUAL_UINT16(250, metrics.descent);
    TEST_ASSERT_EQUAL_INT16(0, metrics.leading);
    TEST_ASSERT_EQUAL_INT16(-100, metrics.underline_position);
    TEST_ASSERT_EQUAL_UINT16(50, metrics.underline_thickness);
    oc_free_face(face);
}

void test_oc_get_glyph_metrics(void) {
    uint16_t idx;
    oc_glyph_metrics glyph_metrics;
    bool ok;

    idx = oc_get_char_index(g_arial_ttf, 'y');
    TEST_ASSERT_EQUAL_INT16(92, idx);

    ok = oc_get_glyph_metrics(g_arial_ttf, idx, NULL);
    TEST_ASSERT_EQUAL(false, ok);

    ok = oc_get_glyph_metrics(g_arial_ttf, idx, &glyph_metrics);
    TEST_ASSERT_EQUAL(true, ok);

    TEST_ASSERT_EQUAL_UINT32(973, glyph_metrics.width);
    TEST_ASSERT_EQUAL_UINT32(1493, glyph_metrics.height);
    TEST_ASSERT_EQUAL_INT32(33, glyph_metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(1062, glyph_metrics.bearing_y);
    TEST_ASSERT_EQUAL_UINT32(1024, glyph_metrics.advance);

    idx = oc_get_char_index(g_arial_ttf, 'g');
    TEST_ASSERT_EQUAL_INT16(74, idx);

    ok = oc_get_glyph_metrics(g_arial_ttf, idx, &glyph_metrics);
    TEST_ASSERT_EQUAL(true, ok);

    TEST_ASSERT_EQUAL_UINT32(936, glyph_metrics.width);
    TEST_ASSERT_EQUAL_UINT32(1517, glyph_metrics.height);
    TEST_ASSERT_EQUAL_INT32(66, glyph_metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(1086, glyph_metrics.bearing_y);
    TEST_ASSERT_EQUAL_UINT32(1139, glyph_metrics.advance);

    idx = oc_get_char_index(g_arial_ttf, 'M');
    TEST_ASSERT_EQUAL_INT16(48, idx);

    ok = oc_get_glyph_metrics(g_arial_ttf, idx, &glyph_metrics);
    TEST_ASSERT_EQUAL(true, ok);

    TEST_ASSERT_EQUAL_UINT32(1399, glyph_metrics.width);
    TEST_ASSERT_EQUAL_UINT32(1466, glyph_metrics.height);
    TEST_ASSERT_EQUAL_INT32(152, glyph_metrics.bearing_x);
    TEST_ASSERT_EQUAL_INT32(1466, glyph_metrics.bearing_y);
    TEST_ASSERT_EQUAL_UINT32(1706, glyph_metrics.advance);

    ok = oc_get_glyph_metrics(g_arial_ttf, 4444, &glyph_metrics);
    TEST_ASSERT_EQUAL(ok, false);
}

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

static void
start_figure(oc_point at, void* context) {
    outline_context* ctx = (outline_context*)context;
    TEST_ASSERT_NOT_EQUAL(ctx->line_points_end, ctx->line_points);

    oc_point test_figure = *ctx->figure_points++;
    TEST_ASSERT_INT16_WITHIN(1, test_figure.x, at.x);
    TEST_ASSERT_INT16_WITHIN(1, test_figure.y, at.y);
}

static void
end_figure(void* context) {
    outline_context* ctx = (outline_context*)context;
    TEST_ASSERT_NOT_EQUAL(ctx->checks_end, ctx->checks);

    outline_end_check check = *(ctx->checks++);
    TEST_ASSERT_EQUAL(check.line_point, ctx->line_points);
    TEST_ASSERT_EQUAL(check.figure_point, ctx->figure_points);
    TEST_ASSERT_EQUAL(check.cubic_point, ctx->cubic_points);
}

static void
line_to(oc_point to, void* context) {
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

    idx = oc_get_char_index(g_arial_ttf, 'i');
    TEST_ASSERT_EQUAL_INT16(76, idx);

    ok = oc_get_outline(g_arial_ttf, idx, NULL, NULL);
    TEST_ASSERT_EQUAL(ok, false);

    ok = oc_get_outline(g_arial_ttf, 4444, NULL, NULL);
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

    ok = oc_get_outline(g_arial_ttf, idx, &funcs, &ctx);
    TEST_ASSERT_EQUAL(ok, true);
    TEST_ASSERT_EQUAL(ctx.checks_end, ctx.checks);

    idx = oc_get_char_index(g_arial_ttf, 'S');
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

    ok = oc_get_outline(g_arial_ttf, idx, &funcs, &ctx);
    TEST_ASSERT_EQUAL(ok, true);
    TEST_ASSERT_EQUAL(ctx.checks_end, ctx.checks);
}

int main(void) {
    UNITY_BEGIN();

    oc_error err;

    err = oc_init_library(&g_library);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    err = oc_open_face(g_library, "test/files/arial.ttf", 0, &g_arial_ttf);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    RUN_TEST(test_oc_init_library);
    RUN_TEST(test_oc_open_face);
    RUN_TEST(test_oc_open_memory_face);
    RUN_TEST(test_oc_get_char_index);
    RUN_TEST(test_oc_get_sfnt_table);
    RUN_TEST(test_oc_get_metrics);
    RUN_TEST(test_oc_get_glyph_metrics);
    RUN_TEST(test_oc_get_outline);

    oc_free_face(g_arial_ttf);
    oc_free_library(g_library);

    UNITY_END();
    return 0;
}
