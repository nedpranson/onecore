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

    // idk intresting
    err = oc_open_memory_face(g_library, data, size - 20, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_free_face(face);

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

    oc_free_face(g_arial_ttf);
    oc_free_library(g_library);

    UNITY_END();
    return 0;
}
