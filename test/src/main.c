#include <onecore.h>
#include <unity.h>

void setUp(void) { }

void tearDown(void) { }

void test_oc_library_init(void) {
    oc_library lib;
    oc_error err;

    err = oc_library_init(&lib);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_library_free(lib);

    err = oc_library_init(NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);
}

void test_oc_face_new(void) {
    oc_library lib;
    oc_face face;
    oc_error err;

    err = oc_library_init(&lib);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    err = oc_face_new(lib, "test/files/arial.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_face_free(face);

    err = oc_face_new(lib, "test/files/arial.idk", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_face_free(face);

    err = oc_face_new(lib, "test/files/arial.otf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_face_free(face);

    err = oc_face_new(lib, "test/files/arial", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    oc_face_free(face);

    err = oc_face_new(lib, "test/files/arial.ttf", 0, NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_face_new(lib, NULL, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_face_new(lib, "test/files/arial.ttf", 10, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_face_new(lib, "non_existing.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    char path[8192 + 1];
    memset(path, 'a', sizeof(path));
    path[8188] = '.';
    path[8189] = 't';
    path[8190] = 't';
    path[8191] = 'f';
    path[8192] = '\0';

    err = oc_face_new(lib, path, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    const char ipath[] = { 0xC0, 0xAF, 0x00 };
    err = oc_face_new(lib, ipath, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_face_new(lib, "", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    oc_library_free(lib);
}

void test_oc_face_get_char_index(void) {
    oc_library lib;
    oc_face face;
    oc_error err;
    uint16_t idx;

    err = oc_library_init(&lib);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    err = oc_face_new(lib, "test/files/arial.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    idx = oc_face_get_char_index(face, 'A');
    TEST_ASSERT_EQUAL_INT16(36, idx);

    idx = oc_face_get_char_index(face, 0);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_face_get_char_index(face, 0xE000);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_face_get_char_index(face, 0x110000);
    TEST_ASSERT_EQUAL_INT16(0, idx);

    idx = oc_face_get_char_index(face, 0xFFFFFFFF);
    TEST_ASSERT_EQUAL_INT16(0, idx);
    oc_face_free(face);

    err = oc_face_new(lib, "test/files/emoji.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    idx = oc_face_get_char_index(face, 0x1F600);
    TEST_ASSERT_EQUAL_INT16(1076, idx);

    oc_face_free(face);
    oc_library_free(lib);
}

void test_oc_face_get_sfnt_table(void) {
    oc_library lib;
    oc_face face;
    oc_table table;
    oc_error err;

    err = oc_library_init(&lib);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    err = oc_face_new(lib, "test/files/arial.ttf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    err = oc_face_get_sfnt_table(face, OC_MAKE_TAG('c', 'm', 'a', 'p'), NULL);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    err = oc_face_get_sfnt_table(face, OC_MAKE_TAG('c', 'm', 'a', 'p'), &table);
    TEST_ASSERT_EQUAL(oc_error_ok, err);
    TEST_ASSERT_EQUAL_size_t(5994, table.size);

    oc_table_free(table);

    err = oc_face_get_sfnt_table(face, OC_MAKE_TAG('u', 'n', 'k', 'n'), &table);
    TEST_ASSERT_EQUAL(oc_error_table_missing, err);

    oc_face_free(face);
    oc_library_free(lib);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_oc_library_init);
    RUN_TEST(test_oc_face_new);
    RUN_TEST(test_oc_face_get_char_index);
    RUN_TEST(test_oc_face_get_sfnt_table);

    UNITY_END();
    return 0;
}
