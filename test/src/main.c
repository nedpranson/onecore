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

    err = oc_face_new(lib, "test/files/arial.otf", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

    err = oc_face_new(lib, "test/files/arial", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_ok, err);

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

    err = oc_face_new(lib, "", 0, &face);
    TEST_ASSERT_EQUAL(oc_error_failed_to_open, err);

    err = oc_face_new(lib, NULL, 0, &face);
    TEST_ASSERT_EQUAL(oc_error_invalid_param, err);

    oc_library_free(lib);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_oc_library_init);
    RUN_TEST(test_oc_face_new);
    UNITY_END();
    return 0;
}
