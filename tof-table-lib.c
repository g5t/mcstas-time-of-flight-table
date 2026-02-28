/* McStas inlines both files verbatim into a single translation unit, so the
 * header content is already present and the #include must be skipped to avoid
 * a missing-file error.  In normal compilation the guard is not yet defined
 * and the #include is processed as usual. */
#ifndef TOF_TABLE_LIB_H
#include "tof-table-lib.h"
#endif

/* ---------------------------------------------------------------------------
 * Internal types
 * ------------------------------------------------------------------------- */

struct TableManagerLinkedListNode {
    char * name;
    double distance;
    struct TableManagerLinkedListNode * next;
};

struct TableManagerLinkedList {
    struct TableManagerLinkedListNode * head;
    struct TableManagerLinkedListNode * tail;
};

/* Offsets into _struct_particle for the per-particle arrays.
 * Computed once at state_finalize time; the hot-path accessors use these
 * directly instead of calling particle_getvar_void on every particle. */
struct TableManagerState {
    int n_recorders;
    int offsets_set;             /* 1 after state_finalize succeeds          */
    struct TableManagerLinkedList recorders;
    ptrdiff_t t_offset;          /* byte offset of table_manager_t_N field   */
    ptrdiff_t p_offset;          /* byte offset of table_manager_p_N field   */
    ptrdiff_t n_offset;          /* byte offset of table_manager_n_N field   */
};

static struct TableManagerState * _tof_table_manager_state = NULL;

/* ---------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

static struct TableManagerState * _table_manager_state_alloc(void) {
    struct TableManagerState * state =
        (struct TableManagerState *) malloc(sizeof(struct TableManagerState));
    if (!state) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for TableManagerState struct.\n");
        return NULL;
    }
    state->n_recorders = 0;
    state->offsets_set = 0;
    state->recorders = (struct TableManagerLinkedList) {0};
    state->t_offset = 0;
    state->p_offset = 0;
    state->n_offset = 0;
    return state;
}

static void _table_manager_state_free(struct TableManagerState * state) {
    if (state) {
        struct TableManagerLinkedListNode * node = state->recorders.head;
        while (node) {
            struct TableManagerLinkedListNode * next = node->next;
            free(node->name);
            free(node);
            node = next;
        }
        free(state);
    }
}

/* Builds "base_name_index" into a freshly allocated string (caller frees). */
static char * _build_suffixed_name(const char * base_name, int index) {
    int len = snprintf(NULL, 0, "%s_%d", base_name, index);
    char * name = (char *) malloc((size_t)(len + 1));
    if (!name) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for suffixed name.\n");
        return NULL;
    }
    sprintf(name, "%s_%d", base_name, index);
    return name;
}

/* ---------------------------------------------------------------------------
 * Data lifetime
 * ------------------------------------------------------------------------- */

struct TableManagerData * table_manager_data_alloc(int recorders, int bins,
                                                   double t_min, double t_max) {
    struct TableManagerData * data =
        (struct TableManagerData *) malloc(sizeof(struct TableManagerData));
    if (!data) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for TableManagerData struct.\n");
        return NULL;
    }
    data->recorders = recorders;
    data->bins = bins;
    data->t_min = t_min;
    data->t_max = t_max;
    data->tp = (double *) calloc((size_t)(recorders * bins), sizeof(double));
    data->p1 = (double *) calloc((size_t)(recorders * bins), sizeof(double));
    data->p2 = (double *) calloc((size_t)(recorders * bins), sizeof(double));
    data->n  = (int *)    calloc((size_t)(recorders * bins), sizeof(int));
    if (!data->tp || !data->p1 || !data->p2 || !data->n) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for TableManagerData arrays.\n");
        table_manager_data_free(data);
        return NULL;
    }
    return data;
}

void table_manager_data_free(struct TableManagerData * data) {
    if (data) {
        free(data->tp);
        free(data->p1);
        free(data->p2);
        free(data->n);
        free(data);
    }
}

/* ---------------------------------------------------------------------------
 * Global state lifetime
 * ------------------------------------------------------------------------- */

void table_manager_state_alloc(void) {
    if (_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: global state is already allocated.\n");
        return;
    }
    _tof_table_manager_state = _table_manager_state_alloc();
    if (!_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate global state.\n");
    }
}

void table_manager_state_free(void) {
    _table_manager_state_free(_tof_table_manager_state);
    _tof_table_manager_state = NULL;
}

int table_manager_state_exists(void) {
    return _tof_table_manager_state != NULL;
}

int table_manager_state_n_recorders(void) {
    return _tof_table_manager_state ? _tof_table_manager_state->n_recorders : 0;
}

int table_manager_state_add_recorder(const char * name, double distance) {
    if (!_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before adding recorders.\n");
        return -1;
    }
    struct TableManagerLinkedListNode * node =
        (struct TableManagerLinkedListNode *) malloc(sizeof(struct TableManagerLinkedListNode));
    if (!node) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for new recorder linked list node.\n");
        return -1;
    }
    node->name = (char *) malloc(strlen(name) + 1);
    if (!node->name) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for new recorder name string.\n");
        free(node);
        return -1;
    }
    strcpy(node->name, name);
    node->distance = distance;
    node->next = NULL;
    struct TableManagerLinkedListNode * tail = _tof_table_manager_state->recorders.tail;
    if (tail) {
        tail->next = node;
    } else {
        _tof_table_manager_state->recorders.head = node;
    }
    _tof_table_manager_state->recorders.tail = node;
    return _tof_table_manager_state->n_recorders++;
}

void table_manager_state_finalize(int manager_index,
                                  const char * t_name,
                                  const char * p_name,
                                  const char * n_name) {
    if (!_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before finalizing.\n");
        return;
    }
    /* Build suffixed names (e.g. "table_manager_t_9"), resolve their byte
     * offsets in _struct_particle via a one-shot particle_getvar_void call,
     * then discard the strings.  Hot-path accessors use the offsets directly
     * and never call particle_getvar_void at TRACE time. */
    char * tname = _build_suffixed_name(t_name, manager_index);
    char * pname = _build_suffixed_name(p_name, manager_index);
    char * nname = _build_suffixed_name(n_name, manager_index);
    if (!tname || !pname || !nname) {
        free(tname); free(pname); free(nname);
        return;
    }
    _class_particle dummy = {0};
    int s_t = 1, s_p = 1, s_n = 1;
    void * t_ptr = particle_getvar_void(&dummy, tname, &s_t);
    void * p_ptr = particle_getvar_void(&dummy, pname, &s_p);
    void * n_ptr = particle_getvar_void(&dummy, nname, &s_n);
    free(tname); free(pname); free(nname);
    if (s_t != 0 || s_p != 0 || s_n != 0 || !t_ptr || !p_ptr || !n_ptr) {
        fprintf(stderr, "TableManager ERROR: Failed to resolve particle field offsets for manager index %d.\n",
                manager_index);
        return;
    }
    _tof_table_manager_state->t_offset = (ptrdiff_t)((char *)t_ptr - (char *)&dummy);
    _tof_table_manager_state->p_offset = (ptrdiff_t)((char *)p_ptr - (char *)&dummy);
    _tof_table_manager_state->n_offset = (ptrdiff_t)((char *)n_ptr - (char *)&dummy);
    _tof_table_manager_state->offsets_set = 1;
}

/* ---------------------------------------------------------------------------
 * Per-particle accessors
 * ------------------------------------------------------------------------- */

double ** table_manager_particle_t_array_ptr(_class_particle * p) {
    if (!_tof_table_manager_state || !_tof_table_manager_state->offsets_set) {
        fprintf(stderr, "TableManager ERROR: state must be allocated and finalized before accessing the t array.\n");
        return NULL;
    }
    return (double **)((char *)p + _tof_table_manager_state->t_offset);
}

double ** table_manager_particle_p_array_ptr(_class_particle * p) {
    if (!_tof_table_manager_state || !_tof_table_manager_state->offsets_set) {
        fprintf(stderr, "TableManager ERROR: state must be allocated and finalized before accessing the p array.\n");
        return NULL;
    }
    return (double **)((char *)p + _tof_table_manager_state->p_offset);
}

int * table_manager_particle_n_ptr(_class_particle * p) {
    if (!_tof_table_manager_state || !_tof_table_manager_state->offsets_set) {
        fprintf(stderr, "TableManager ERROR: state must be allocated and finalized before accessing the n array.\n");
        return NULL;
    }
    return (int *)((char *)p + _tof_table_manager_state->n_offset);
}

/* ---------------------------------------------------------------------------
 * Per-particle operations
 * ------------------------------------------------------------------------- */

void table_manager_particle_alloc(_class_particle * p, double t_zero) {
    double ** tof_t_ptr = table_manager_particle_t_array_ptr(p);
    double ** tof_p_ptr = table_manager_particle_p_array_ptr(p);
    int *     tof_n_ptr = table_manager_particle_n_ptr(p);
    if (!tof_t_ptr || !tof_p_ptr || !tof_n_ptr) {
        fprintf(stderr, "TableManager ERROR: Failed to access per-particle time or probability arrays for allocation.\n");
        return;
    }
    *tof_t_ptr = NULL;
    *tof_p_ptr = NULL;
    *tof_n_ptr = 0;
    if (_tof_table_manager_state->n_recorders > 0) {
        int n = _tof_table_manager_state->n_recorders;
        *tof_t_ptr = (double *) malloc(sizeof(double) * n);
        *tof_p_ptr = (double *) malloc(sizeof(double) * n);
        if (!*tof_t_ptr || !*tof_p_ptr) {
            fprintf(stderr, "TableManager ERROR: Failed to allocate memory for per-particle time or probability arrays.\n");
            free(*tof_t_ptr);
            free(*tof_p_ptr);
            *tof_t_ptr = NULL;
            *tof_p_ptr = NULL;
            return;
        }
        *tof_n_ptr = n;
        for (int i = 0; i < n; i++) {
            (*tof_t_ptr)[i] = t_zero;
            (*tof_p_ptr)[i] = 0.0;
        }
    }
}

int table_manager_particle_record(_class_particle * p, int recorder_index) {
    double * tof_t_ptr = *table_manager_particle_t_array_ptr(p);
    double * tof_p_ptr = *table_manager_particle_p_array_ptr(p);
    int *    tof_n_ptr =  table_manager_particle_n_ptr(p);
    if (!tof_t_ptr || !tof_p_ptr || !tof_n_ptr) {
        fprintf(stderr, "TableManager ERROR: Failed to access per-particle time or probability arrays for recording.\n");
        return -1;
    }
    if (recorder_index < 0 || recorder_index >= *tof_n_ptr) {
        fprintf(stderr, "TableManager ERROR: Recorder index out of bounds when recording particle data.\n");
        return -1;
    }
    tof_t_ptr[recorder_index] = p->t;
    tof_p_ptr[recorder_index] = p->p;
    return 0;
}

int table_manager_particle_to_table(_class_particle * p, struct TableManagerData * data) {
    double * tof_t_ptr = *table_manager_particle_t_array_ptr(p);
    double * tof_p_ptr = *table_manager_particle_p_array_ptr(p);
    int *    tof_n_ptr =  table_manager_particle_n_ptr(p);
    if (!tof_t_ptr || !tof_p_ptr || !tof_n_ptr) {
        fprintf(stderr, "TableManager ERROR: Failed to access per-particle time or probability arrays for transfer to table.\n");
        return -1;
    }
    if (*tof_n_ptr != data->recorders) {
        fprintf(stderr, "TableManager ERROR: Number of recorders in particle data does not match number of recorders in table data during transfer.\n");
        return -1;
    }
    #pragma omp critical
    {
        for (int i = 0; i < data->recorders; ++i) {
            double t = tof_t_ptr[i];
            double p = tof_p_ptr[i];
            int j = (int) ((t - data->t_min) / (data->t_max - data->t_min) * data->bins);
            if (j < 0 || j >= data->bins)
                continue;
            int idx = i * data->bins + j;
            data->p1[idx] += p;
            data->p2[idx] += p * p;
            data->tp[idx] += t * p;
            data->n[idx]  += 1;
        }
    }
    return 0;
}

int table_manager_particle_free(_class_particle * p) {
    double ** tof_t_ptr = table_manager_particle_t_array_ptr(p);
    double ** tof_p_ptr = table_manager_particle_p_array_ptr(p);
    int *     tof_n_ptr = table_manager_particle_n_ptr(p);
    if (!tof_t_ptr || !tof_p_ptr || !tof_n_ptr) {
        fprintf(stderr, "TableManager ERROR: Failed to access per-particle time or probability arrays for freeing.\n");
        return -1;
    }
    free(*tof_t_ptr);
    free(*tof_p_ptr);
    *tof_t_ptr = NULL;
    *tof_p_ptr = NULL;
    *tof_n_ptr = 0;
    return 0;
}

/* ---------------------------------------------------------------------------
 * JSON helpers
 * ------------------------------------------------------------------------- */

int table_manager_json_indent(FILE * f, int level) {
    for (int i = 0; i < level; ++i) {
        if (fprintf(f, "  ") < 0)
            return -1;
    }
    return 0;
}

int table_manager_json_array_double(FILE * f, double * x, int n) {
    if (fprintf(f, "[") < 0)
        return -1;
    for (int i = 0; i < n; ++i) {
        if (fprintf(f, "%.15g", x[i]) < 0)
            return -1;
        if (i < n - 1 && fprintf(f, ", ") < 0)
            return -1;
    }
    if (fprintf(f, "]") < 0)
        return -1;
    return 0;
}

int table_manager_json_array_int(FILE * f, int * x, int n) {
    if (fprintf(f, "[") < 0)
        return -1;
    for (int i = 0; i < n; ++i) {
        if (fprintf(f, "%d", x[i]) < 0)
            return -1;
        if (i < n - 1 && fprintf(f, ", ") < 0)
            return -1;
    }
    if (fprintf(f, "]") < 0)
        return -1;
    return 0;
}

int table_manager_json_array_string(FILE * f, char ** x, int n) {
    if (fprintf(f, "[") < 0)
        return -1;
    for (int i = 0; i < n; ++i) {
        if (fprintf(f, "\"%s\"", x[i]) < 0)
            return -1;
        if (i < n - 1 && fprintf(f, ", ") < 0)
            return -1;
    }
    if (fprintf(f, "]") < 0)
        return -1;
    return 0;
}

int table_manager_json_matrix_double(FILE * f, double * x, int m, int n,
                                     int indent_level) {
    if (fprintf(f, "[\n") < 0)
        return -1;
    for (int i = 0; i < m; ++i) {
        if (table_manager_json_indent(f, indent_level + 1) < 0)
            return -1;
        if (table_manager_json_array_double(f, &x[i * n], n) < 0)
            return -1;
        if (fprintf(f, i < m - 1 ? ",\n" : "\n") < 0)
            return -1;
    }
    if (table_manager_json_indent(f, indent_level) < 0)
        return -1;
    if (fprintf(f, "]") < 0)
        return -1;
    return 0;
}

int table_manager_json_matrix_int(FILE * f, int * x, int m, int n,
                                  int indent_level) {
    if (fprintf(f, "[\n") < 0)
        return -1;
    for (int i = 0; i < m; ++i) {
        if (table_manager_json_indent(f, indent_level + 1) < 0)
            return -1;
        if (table_manager_json_array_int(f, &x[i * n], n) < 0)
            return -1;
        if (fprintf(f, i < m - 1 ? ",\n" : "\n") < 0)
            return -1;
    }
    if (table_manager_json_indent(f, indent_level) < 0)
        return -1;
    if (fprintf(f, "]") < 0)
        return -1;
    return 0;
}

static int _table_manager_json_recorder_info(FILE * f, int indent_level) {
    if (!_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before writing recorder info to output file.\n");
        return -1;
    }
    if (table_manager_json_indent(f, indent_level) < 0 ||
        fprintf(f, "\"recorders\": [\n") < 0)
        return -1;
    struct TableManagerLinkedListNode * node = _tof_table_manager_state->recorders.head;
    for (int i = 0; i < _tof_table_manager_state->n_recorders; ++i) {
        if (!node) {
            fprintf(stderr, "TableManager ERROR: Recorder linked list is shorter than n_recorders.\n");
            return -1;
        }
        if (table_manager_json_indent(f, indent_level + 1) < 0 ||
            fprintf(f, "{ \"name\": \"%s\", \"distance\": %.15g }",
                    node->name, node->distance) < 0)
            return -1;
        if (fprintf(f, i < _tof_table_manager_state->n_recorders - 1 ? ",\n" : "\n") < 0)
            return -1;
        node = node->next;
    }
    if (table_manager_json_indent(f, indent_level) < 0 || fprintf(f, "]") < 0)
        return -1;
    return 0;
}

static int _table_manager_json_time_info(FILE * f, int indent_level,
                                         struct TableManagerData * data) {
    if (table_manager_json_indent(f, indent_level) < 0 ||
        fprintf(f, "\"time\": { \"min\": %.15g, \"max\": %.15g, \"bins\": %d }",
                data->t_min, data->t_max, data->bins) < 0)
        return -1;
    return 0;
}

/* ---------------------------------------------------------------------------
 * Output
 * ------------------------------------------------------------------------- */

int table_manager_write_output_file(const char * filename,
                                    struct TableManagerData * data) {
    FILE * f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "TableManager ERROR: Failed to open file '%s' for writing.\n", filename);
        return -1;
    }
    int ok =
        fprintf(f, "{\n") > 0 &&
        table_manager_json_indent(f, 1) == 0 &&
        fprintf(f, "\"dims\": {\"time\": %d, \"recorders\": %d},\n",
                data->bins, data->recorders) > 0 &&
        table_manager_json_indent(f, 1) == 0 &&
        fprintf(f, "\"coords\": [\n") > 0 &&
        _table_manager_json_recorder_info(f, 2) == 0 &&
        fprintf(f, ",\n") > 0 &&
        _table_manager_json_time_info(f, 2, data) == 0 &&
        fprintf(f, "\n") > 0 &&
        table_manager_json_indent(f, 1) == 0 &&
        fprintf(f, "],\n") > 0 &&
        table_manager_json_indent(f, 1) == 0 &&
        fprintf(f, "\"tp\": ") > 0 &&
        table_manager_json_matrix_double(f, data->tp, data->recorders, data->bins, 2) == 0 &&
        fprintf(f, ",\n") > 0 &&
        table_manager_json_indent(f, 1) == 0 &&
        fprintf(f, "\"p1\": ") > 0 &&
        table_manager_json_matrix_double(f, data->p1, data->recorders, data->bins, 2) == 0 &&
        fprintf(f, ",\n") > 0 &&
        table_manager_json_indent(f, 1) == 0 &&
        fprintf(f, "\"p2\": ") > 0 &&
        table_manager_json_matrix_double(f, data->p2, data->recorders, data->bins, 2) == 0 &&
        fprintf(f, ",\n") > 0 &&
        table_manager_json_indent(f, 1) == 0 &&
        fprintf(f, "\"n\": ") > 0 &&
        table_manager_json_matrix_int(f, data->n, data->recorders, data->bins, 2) == 0 &&
        fprintf(f, "\n}\n") > 0;
    if (!ok) {
        fprintf(stderr, "TableManager ERROR: Failed to write to file '%s'.\n", filename);
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}
