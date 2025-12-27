#include <unity.h>
#include <onecore.h>

void setUp(void) {}

void tearDown(void) {}

void test_function_should_doBlahAndBlah(void) {
  oc_library lib;
  oc_library_init(&lib);
  oc_library_free(lib);
}

void test_function_should_doAlsoDoBlah(void) {
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_function_should_doBlahAndBlah);
    RUN_TEST(test_function_should_doAlsoDoBlah);
    return UNITY_END();
}
