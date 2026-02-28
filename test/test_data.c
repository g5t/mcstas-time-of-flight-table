/* test_data.c – Unity tests for TableManagerData allocation and
 * deallocation. */
#include "unity.h"
#include "tof-table-lib.h"
#include "particle_stub.h"

void setUp(void)    {}
void tearDown(void) {}

void test_data_alloc_returns_non_null(void) {
    struct TableManagerData * data = table_manager_data_alloc(2, 10, 0.0, 1.0);
    TEST_ASSERT_NOT_NULL(data);
    table_manager_data_free(data);
}

void test_data_alloc_sets_correct_fields(void) {
    struct TableManagerData * data = table_manager_data_alloc(3, 8, 0.5, 2.5);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_INT(3, data->recorders);
    TEST_ASSERT_EQUAL_INT(8, data->bins);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.5, data->t_min);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 2.5, data->t_max);
    table_manager_data_free(data);
}

void test_data_alloc_arrays_are_non_null(void) {
    struct TableManagerData * data = table_manager_data_alloc(2, 5, 0.0, 1.0);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_NOT_NULL(data->tp);
    TEST_ASSERT_NOT_NULL(data->p1);
    TEST_ASSERT_NOT_NULL(data->p2);
    TEST_ASSERT_NOT_NULL(data->n);
    table_manager_data_free(data);
}

void test_data_free_null_does_not_crash(void) {
    table_manager_data_free(NULL); /* must be safe */
}

void test_data_alloc_single_recorder_single_bin(void) {
    struct TableManagerData * data = table_manager_data_alloc(1, 1, 0.0, 1.0);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_INT(1, data->recorders);
    TEST_ASSERT_EQUAL_INT(1, data->bins);
    table_manager_data_free(data);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_data_alloc_returns_non_null);
    RUN_TEST(test_data_alloc_sets_correct_fields);
    RUN_TEST(test_data_alloc_arrays_are_non_null);
    RUN_TEST(test_data_free_null_does_not_crash);
    RUN_TEST(test_data_alloc_single_recorder_single_bin);
    return UNITY_END();
}
