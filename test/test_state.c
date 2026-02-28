/* test_state.c – Unity tests for TableManagerState lifecycle and recorder
 * registration.  Uses only the public API; never touches the static pointer
 * directly. */
#include "unity.h"
#include "tof-table-lib.h"
#include "particle_stub.h"

void setUp(void)    { /* each test starts with no global state */ }
void tearDown(void) { table_manager_state_free(); }

/* ---- existence checks ---- */

void test_state_does_not_exist_initially(void) {
    TEST_ASSERT_EQUAL_INT(0, table_manager_state_exists());
}

void test_state_exists_after_alloc(void) {
    table_manager_state_alloc();
    TEST_ASSERT_EQUAL_INT(1, table_manager_state_exists());
}

void test_state_does_not_exist_after_free(void) {
    table_manager_state_alloc();
    table_manager_state_free();
    TEST_ASSERT_EQUAL_INT(0, table_manager_state_exists());
}

void test_double_alloc_does_not_crash(void) {
    table_manager_state_alloc();
    table_manager_state_alloc(); /* logs an error but must not crash */
    TEST_ASSERT_EQUAL_INT(1, table_manager_state_exists());
}

void test_free_without_alloc_does_not_crash(void) {
    table_manager_state_free(); /* no state yet – must be safe */
    TEST_ASSERT_EQUAL_INT(0, table_manager_state_exists());
}

/* ---- add_recorder ---- */

void test_add_recorder_without_state_returns_error(void) {
    int ret = table_manager_state_add_recorder("rec0", 1.0);
    TEST_ASSERT_EQUAL_INT(-1, ret);
}

void test_add_first_recorder_returns_zero(void) {
    table_manager_state_alloc();
    int ret = table_manager_state_add_recorder("rec0", 1.0);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

void test_add_recorders_return_sequential_indices(void) {
    table_manager_state_alloc();
    TEST_ASSERT_EQUAL_INT(0, table_manager_state_add_recorder("rec0", 1.0));
    TEST_ASSERT_EQUAL_INT(1, table_manager_state_add_recorder("rec1", 2.0));
    TEST_ASSERT_EQUAL_INT(2, table_manager_state_add_recorder("rec2", 3.0));
}

/* ---- state_finalize – indirect test via particle accessor ---- */

void test_state_finalize_enables_particle_accessor(void) {
    table_manager_state_alloc();
    table_manager_state_add_recorder("rec0", 1.0);
    table_manager_state_finalize(9, "table_manager_t", "table_manager_p", "table_manager_n");

    _class_particle p = {0};
    /* Accessors must return non-NULL once the state has been finalised. */
    TEST_ASSERT_NOT_NULL(table_manager_particle_t_array_ptr(&p));
    TEST_ASSERT_NOT_NULL(table_manager_particle_p_array_ptr(&p));
    TEST_ASSERT_NOT_NULL(table_manager_particle_n_ptr(&p));
}

/* ---- main ---- */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_state_does_not_exist_initially);
    RUN_TEST(test_state_exists_after_alloc);
    RUN_TEST(test_state_does_not_exist_after_free);
    RUN_TEST(test_double_alloc_does_not_crash);
    RUN_TEST(test_free_without_alloc_does_not_crash);
    RUN_TEST(test_add_recorder_without_state_returns_error);
    RUN_TEST(test_add_first_recorder_returns_zero);
    RUN_TEST(test_add_recorders_return_sequential_indices);
    RUN_TEST(test_state_finalize_enables_particle_accessor);
    return UNITY_END();
}
