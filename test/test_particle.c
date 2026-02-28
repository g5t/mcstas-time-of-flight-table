/* test_particle.c – Unity tests for per-particle data functions.
 *
 * setUp/tearDown creates a one-recorder state, finalized for index 9 so that
 * the particle_getvar_void stub resolves the correct _struct_particle fields. */
#include "unity.h"
#include "tof-table-lib.h"
#include "particle_stub.h"

/* Manager index must match the hardcoded field names in _struct_particle. */
#define TEST_MANAGER_IDX  9
#define T_BASE  "table_manager_t"
#define P_BASE  "table_manager_p"
#define N_BASE  "table_manager_n"

void setUp(void) {
    table_manager_state_alloc();
    table_manager_state_add_recorder("rec0", 1.0);
    table_manager_state_finalize(TEST_MANAGER_IDX, T_BASE, P_BASE, N_BASE);
}

void tearDown(void) {
    table_manager_state_free();
}

/* ---- particle_alloc ---- */

void test_particle_alloc_sets_array_size(void) {
    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    TEST_ASSERT_EQUAL_INT(1, p.table_manager_n_9);
    table_manager_particle_free(&p);
}

void test_particle_alloc_initialises_t_to_t_zero(void) {
    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.42);
    TEST_ASSERT_NOT_NULL(p.table_manager_t_9);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.42, p.table_manager_t_9[0]);
    table_manager_particle_free(&p);
}

void test_particle_alloc_initialises_p_to_zero(void) {
    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    TEST_ASSERT_NOT_NULL(p.table_manager_p_9);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, p.table_manager_p_9[0]);
    table_manager_particle_free(&p);
}

void test_particle_alloc_zero_recorders_sets_null_arrays(void) {
    /* Re-initialise state with zero recorders. */
    table_manager_state_free();
    table_manager_state_alloc();
    table_manager_state_finalize(TEST_MANAGER_IDX, T_BASE, P_BASE, N_BASE);

    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    TEST_ASSERT_NULL(p.table_manager_t_9);
    TEST_ASSERT_NULL(p.table_manager_p_9);
    TEST_ASSERT_EQUAL_INT(0, p.table_manager_n_9);
    /* No free needed – arrays are NULL */
}

/* ---- particle_record ---- */

void test_particle_record_stores_t_and_p(void) {
    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    p.t = 0.123;
    p.p = 0.987;
    int ret = table_manager_particle_record(&p, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.123, p.table_manager_t_9[0]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.987, p.table_manager_p_9[0]);
    table_manager_particle_free(&p);
}

void test_particle_record_negative_index_returns_error(void) {
    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    int ret = table_manager_particle_record(&p, -1);
    TEST_ASSERT_EQUAL_INT(-1, ret);
    table_manager_particle_free(&p);
}

void test_particle_record_out_of_bounds_index_returns_error(void) {
    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    int ret = table_manager_particle_record(&p, 99);
    TEST_ASSERT_EQUAL_INT(-1, ret);
    table_manager_particle_free(&p);
}

/* ---- particle_to_table ---- */

void test_particle_to_table_bins_time_correctly(void) {
    /* t_min=0, t_max=1, bins=10 → bin index = floor(t * 10) */
    struct TableManagerData * data = table_manager_data_alloc(1, 10, 0.0, 1.0);
    memset(data->tp, 0, sizeof(double) * 10);
    memset(data->p1, 0, sizeof(double) * 10);
    memset(data->p2, 0, sizeof(double) * 10);
    memset(data->n,  0, sizeof(int)    * 10);

    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    p.t = 0.25;   /* bin 2 (index = floor(0.25 * 10) = 2) */
    p.p = 1.0;
    table_manager_particle_record(&p, 0);
    table_manager_particle_to_table(&p, data);

    TEST_ASSERT_EQUAL_INT(1,   data->n[2]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 1.0,  data->p1[2]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 1.0,  data->p2[2]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.25, data->tp[2]);
    /* Other bins should be untouched */
    TEST_ASSERT_EQUAL_INT(0, data->n[0]);
    TEST_ASSERT_EQUAL_INT(0, data->n[5]);

    table_manager_particle_free(&p);
    table_manager_data_free(data);
}

void test_particle_to_table_skips_out_of_range_time(void) {
    struct TableManagerData * data = table_manager_data_alloc(1, 10, 0.0, 1.0);
    memset(data->n, 0, sizeof(int) * 10);
    memset(data->p1, 0, sizeof(double) * 10);
    memset(data->p2, 0, sizeof(double) * 10);
    memset(data->tp, 0, sizeof(double) * 10);

    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    p.t = 5.0;   /* outside [0, 1) */
    p.p = 1.0;
    table_manager_particle_record(&p, 0);
    int ret = table_manager_particle_to_table(&p, data);
    TEST_ASSERT_EQUAL_INT(0, ret);
    for (int i = 0; i < 10; ++i) {
        TEST_ASSERT_EQUAL_INT(0, data->n[i]);
    }

    table_manager_particle_free(&p);
    table_manager_data_free(data);
}

/* ---- particle_free ---- */

void test_particle_free_nullifies_pointers(void) {
    _class_particle p = {0};
    table_manager_particle_alloc(&p, 0.0);
    TEST_ASSERT_NOT_NULL(p.table_manager_t_9);
    int ret = table_manager_particle_free(&p);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_NULL(p.table_manager_t_9);
    TEST_ASSERT_NULL(p.table_manager_p_9);
    TEST_ASSERT_EQUAL_INT(0, p.table_manager_n_9);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_particle_alloc_sets_array_size);
    RUN_TEST(test_particle_alloc_initialises_t_to_t_zero);
    RUN_TEST(test_particle_alloc_initialises_p_to_zero);
    RUN_TEST(test_particle_alloc_zero_recorders_sets_null_arrays);
    RUN_TEST(test_particle_record_stores_t_and_p);
    RUN_TEST(test_particle_record_negative_index_returns_error);
    RUN_TEST(test_particle_record_out_of_bounds_index_returns_error);
    RUN_TEST(test_particle_to_table_bins_time_correctly);
    RUN_TEST(test_particle_to_table_skips_out_of_range_time);
    RUN_TEST(test_particle_free_nullifies_pointers);
    return UNITY_END();
}
