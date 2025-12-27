#include <unity.h>
#include <onecore.h>

void setUp(void) {}

void tearDown(void) {}

void test_oc_library_init(void) {
  oc_library lib;
  oc_error err;

  err = oc_library_init(&lib);
  TEST_ASSERT_EQUAL(err, oc_error_ok);
  oc_library_free(lib);

  err = oc_library_init(NULL);
  TEST_ASSERT_EQUAL(err, oc_error_invalid_param);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_oc_library_init);
    UNITY_END();
    return 0;
}
