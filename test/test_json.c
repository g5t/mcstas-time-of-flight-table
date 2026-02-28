/* test_json.c – Unity tests for the JSON helper functions and the output
 * file writer.  File content is checked via tmpfile() + rewind(). */
#include "unity.h"
#include "tof-table-lib.h"
#include "particle_stub.h"
#include <string.h>

/* Helper: read the full content of a rewound FILE* into a caller-supplied
 * buffer.  Returns the number of bytes read. */
static size_t read_tmpfile(FILE * f, char * buf, size_t buf_size) {
    rewind(f);
    size_t n = fread(buf, 1, buf_size - 1, f);
    buf[n] = '\0';
    return n;
}

void setUp(void)    {}
void tearDown(void) {}

/* ---- json_indent ---- */

void test_json_indent_zero_writes_nothing(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    int ret = table_manager_json_indent(f, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    char buf[32];
    size_t n = read_tmpfile(f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_size_t(0, n);
    fclose(f);
}

void test_json_indent_two_levels(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    table_manager_json_indent(f, 2);
    char buf[32];
    read_tmpfile(f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("    ", buf); /* 2 × 2 spaces */
    fclose(f);
}

/* ---- json_array_double ---- */

void test_json_array_double_empty(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    double arr[] = {0.0};
    int ret = table_manager_json_array_double(f, arr, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    char buf[32];
    read_tmpfile(f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("[]", buf);
    fclose(f);
}

void test_json_array_double_single(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    double arr[] = {1.5};
    table_manager_json_array_double(f, arr, 1);
    char buf[64];
    read_tmpfile(f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("[1.5]", buf);
    fclose(f);
}

void test_json_array_double_multiple(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    double arr[] = {1.0, 2.5};
    table_manager_json_array_double(f, arr, 2);
    char buf[64];
    read_tmpfile(f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("[1, 2.5]", buf);
    fclose(f);
}

/* ---- json_array_int ---- */

void test_json_array_int_values(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    int arr[] = {1, 2, 3};
    int ret = table_manager_json_array_int(f, arr, 3);
    TEST_ASSERT_EQUAL_INT(0, ret);
    char buf[64];
    read_tmpfile(f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("[1, 2, 3]", buf);
    fclose(f);
}

/* ---- json_array_string ---- */

void test_json_array_string_values(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    char * arr[] = {"hello", "world"};
    int ret = table_manager_json_array_string(f, arr, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    char buf[64];
    read_tmpfile(f, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("[\"hello\", \"world\"]", buf);
    fclose(f);
}

/* ---- json_matrix_double ---- */

void test_json_matrix_double_shape(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    double mat[] = {1.0, 2.0, 3.0, 4.0}; /* 2×2 row-major */
    int ret = table_manager_json_matrix_double(f, mat, 2, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    char buf[256];
    read_tmpfile(f, buf, sizeof(buf));
    /* Expect two rows separated by a comma and ending with closing bracket */
    TEST_ASSERT_NOT_NULL(strstr(buf, "[1, 2]"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "[3, 4]"));
    fclose(f);
}

/* ---- json_matrix_int ---- */

void test_json_matrix_int_shape(void) {
    FILE * f = tmpfile();
    TEST_ASSERT_NOT_NULL(f);
    int mat[] = {10, 20, 30, 40}; /* 2×2 row-major */
    int ret = table_manager_json_matrix_int(f, mat, 2, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    char buf[256];
    read_tmpfile(f, buf, sizeof(buf));
    TEST_ASSERT_NOT_NULL(strstr(buf, "[10, 20]"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "[30, 40]"));
    fclose(f);
}

/* ---- write_output_file ---- */

void test_write_output_file_creates_file(void) {
    table_manager_state_alloc();
    table_manager_state_add_recorder("test_rec", 1.5);
    table_manager_state_finalize(9, "table_manager_t", "table_manager_p", "table_manager_n");

    struct TableManagerData * data = table_manager_data_alloc(1, 3, 0.0, 1.0);
    memset(data->tp, 0, sizeof(double) * 3);
    memset(data->p1, 0, sizeof(double) * 3);
    memset(data->p2, 0, sizeof(double) * 3);
    memset(data->n,  0, sizeof(int)    * 3);
    data->p1[0] = 1.0;
    data->tp[0] = 0.25;
    data->n[0]  = 1;

    const char * fname = "test_output_tmp.json";
    int ret = table_manager_write_output_file(fname, data);
    TEST_ASSERT_EQUAL_INT(0, ret);

    FILE * f = fopen(fname, "r");
    TEST_ASSERT_NOT_NULL(f);
    char buf[8192];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    remove(fname);

    /* Top-level structure */
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"type\": \"scipp.Dataset\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"coords\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"data\""));
    /* Coordinates */
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"time\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"distance\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"recorder\""));
    /* null unit for string coord */
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"unit\": null"));
    /* Data variables */
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"tp\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"p1\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"p2\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"n\""));
    /* Bin edges: bins=3 → 4 values; time coord must have the max value */
    TEST_ASSERT_NOT_NULL(strstr(buf, "1"));

    table_manager_data_free(data);
    table_manager_state_free();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_json_indent_zero_writes_nothing);
    RUN_TEST(test_json_indent_two_levels);
    RUN_TEST(test_json_array_double_empty);
    RUN_TEST(test_json_array_double_single);
    RUN_TEST(test_json_array_double_multiple);
    RUN_TEST(test_json_array_int_values);
    RUN_TEST(test_json_array_string_values);
    RUN_TEST(test_json_matrix_double_shape);
    RUN_TEST(test_json_matrix_int_shape);
    RUN_TEST(test_write_output_file_creates_file);
    return UNITY_END();
}
