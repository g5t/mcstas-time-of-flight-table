#ifndef TOF_TABLE_LIB_H
#include "tof-table-lib.h"
#endif 

int table_manager_json_indent(FILE * f, int level){
  for (int i=0; i < level; ++i) {
    if (fprintf(f, "  ") < 0) {
      return -1;
    }
  }
  return 0;
}

int table_manager_json_array_double(FILE * f, double * x, int n){
  if (fprintf(f, "[") < 0) {
    return -1;
  }
  for (int i=0; i < n; ++i) {
    if (fprintf(f, "%.15g", x[i]) < 0) {
      return -1;
    }
    if (i < n - 1) {
      if (fprintf(f, ", ") < 0) {
        return -1;
      }
    }
  }
  if (fprintf(f, "]") < 0) {
    return -1;
  }
  return 0;
}

int table_manager_json_array_int(FILE * f, int * x, int n){
  if (fprintf(f, "[") < 0) {
    return -1;
  }
  for (int i=0; i < n; ++i) {
    if (fprintf(f, "%d", x[i]) < 0) {
      return -1;
    }
    if (i < n - 1) {
      if (fprintf(f, ", ") < 0) {
        return -1;
      }
    }
  }
  if (fprintf(f, "]") < 0) {
    return -1;
  }
  return 0;
}

int table_manager_json_array_string(FILE * f, char ** x, int n){
  if (fprintf(f, "[") < 0) {
    return -1;
  }
  for (int i=0; i < n; ++i) {
    if (fprintf(f, "\"%s\"", x[i]) < 0) {
      return -1;
    }
    if (i < n - 1) {
      if (fprintf(f, ", ") < 0) {
        return -1;
      }
    }
  }
  if (fprintf(f, "]") < 0) {
    return -1;
  }
  return 0;
}

int table_manager_json_matrix_double(FILE * f, double * x, int m, int n, int indent_level){
  if (fprintf(f, "[\n") < 0) {
    return -1;
  }
  for (int i=0; i < m; ++i) {
    if (table_manager_json_indent(f, indent_level + 1) < 0) {
      return -1;
    }
    if (table_manager_json_array_double(f, &x[i*n], n) < 0) {
      return -1;
    }
    if (i < m - 1) {
      if (fprintf(f, ",\n") < 0) {
        return -1;
      }
    } else {
      if (fprintf(f, "\n") < 0) {
        return -1;
      }
    }
  }
  if (table_manager_json_indent(f, indent_level) < 0) {
    return -1;
  }
  if (fprintf(f, "]") < 0) {
    return -1;
  }
  return 0;
}

int table_manager_json_matrix_int(FILE * f, int * x, int m, int n, int indent_level){
  if (fprintf(f, "[\n") < 0) {
    return -1;
  }
  for (int i=0; i < m; ++i) {
    if (table_manager_json_indent(f, indent_level + 1) < 0) {
      return -1;
    }
    if (table_manager_json_array_int(f, &x[i*n], n) < 0) {
      return -1;
    }
    if (i < m - 1) {
      if (fprintf(f, ",\n") < 0) {
        return -1;
      }
    } else {
      if (fprintf(f, "\n") < 0) {
        return -1;
      }
    }
  }
  if (table_manager_json_indent(f, indent_level) < 0) {
    return -1;
  }
  if (fprintf(f, "]") < 0) {
    return -1;
  }
  return 0;
}

int table_manager_json_recorder_info(FILE * f, int indent_level) {
  if (!_tof_table_manager_state) {
    fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before writing recorder info to output file.\n");
    return -1;
  }
  if (table_manager_json_indent(f, indent_level) < 0 ||
      fprintf(f, "\"recorders\": [\n") < 0) {
    return -1;
  }
  struct TableManagerLinkedListNode * node = _tof_table_manager_state->recorders.head;
  for (int i=0; i < _tof_table_manager_state->n_recorders; ++i) {
    if (!node) {
      fprintf(stderr, "TableManager ERROR: Recorder linked lists are shorter than n_recorders in state.\n");
      return -1;
    }
    if (table_manager_json_indent(f, indent_level + 1) < 0 ||
        fprintf(f, "{ \"name\": \"%s\", \"distance\": %.15g }", node->name, node->distance) < 0) {
      return -1;
    }
    if (i < _tof_table_manager_state->n_recorders - 1) {
      if (fprintf(f, ",\n") < 0) {
        return -1;
      }
    } else {
      if (fprintf(f, "\n") < 0) {
        return -1;
      }
    }
    node = node->next;
  }
  if (table_manager_json_indent(f, indent_level) < 0 ||
      fprintf(f, "]") < 0) {
    return -1;
  }
  return 0;
}

int table_manager_json_time_info(FILE * f, int indent_level, struct TableManagerData * data) {
  if (table_manager_json_indent(f, indent_level) < 0 ||
      fprintf(f, "\"time\": { \"min\": %.15g, \"max\": %.15g, \"bins\": %d }", data->t_min, data->t_max, data->bins) < 0) {
    return -1;
  }
  return 0;
}


double ** table_manager_particle_t_array_ptr(_class_particle * p) {
  if (!_tof_table_manager_state) {
    fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before accessing the t array.\n");
    return NULL;
  }
  if (!_tof_table_manager_state->t_name) {
    fprintf(stderr, "TableManager ERROR: t_name must be set in the state before accessing the t array.\n");
    return NULL;
  }
  return (double **) particle_getvar_void(p, _tof_table_manager_state->t_name, 0x0);
}
double ** table_manager_particle_p_array_ptr(_class_particle * p) {
  if (!_tof_table_manager_state) {
    fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before accessing the p array.\n");
    return NULL;
  }
  if (!_tof_table_manager_state->p_name) {
    fprintf(stderr, "TableManager ERROR: p_name must be set in the state before accessing the p array.\n");
    return NULL;
  }
  return (double **) particle_getvar_void(p, _tof_table_manager_state->p_name, 0x0);
}
int * table_manager_particle_n_ptr(_class_particle * p) {
  if (!_tof_table_manager_state) {
    fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before accessing the n array.\n");
    return NULL;
  }
  if (!_tof_table_manager_state->n_name) {
    fprintf(stderr, "TableManager ERROR: n_name must be set in the state before accessing the n array.\n");
    return NULL;
  }
  return (int *) particle_getvar_void(p, _tof_table_manager_state->n_name, 0x0);
}



struct TableManagerData * table_manager_data_alloc(int recorders, int bins, double t_min, double t_max){
    struct TableManagerData * data = (struct TableManagerData *) malloc(sizeof(struct TableManagerData));
    if (!data) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for TableManagerData struct.\n");
        return NULL;
    }
    data->recorders = recorders;
    data->bins = bins;
    data->t_min = t_min;
    data->t_max = t_max;
    data->tp = (double *) malloc(sizeof(double) * recorders * bins);
    data->p1 = (double *) malloc(sizeof(double) * recorders * bins);
    data->p2 = (double *) malloc(sizeof(double) * recorders * bins);
    data->n = (int *) malloc(sizeof(int) * recorders * bins);
    if (!data->tp || !data->p1 || !data->p2 || !data->n) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for TableManagerData arrays.\n");
        table_manager_data_free(data);
        return NULL;
    }
    return data;
}

void table_manager_data_free(struct TableManagerData * data){
    if (data) {
        free(data->tp);
        free(data->p1);
        free(data->p2);
        free(data->n);
        free(data);
    }
}

struct TableManagerState * _table_manager_state_alloc(){
    struct TableManagerState * state = (struct TableManagerState *) malloc(sizeof(struct TableManagerState));
    if (!state) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for TableManagerState struct.\n");
        return NULL;
    }
    state->n_recorders = 0;
    state->recorders = (struct TableManagerLinkedList) {0};
    state->t_name = NULL;
    state->p_name = NULL;
    state->n_name = NULL;
    return state;
}

void _table_manager_state_free(struct TableManagerState * state){
    if (state) {
        // Free the recorder linked list
        struct TableManagerLinkedListNode * node = state->recorders.head;
        while (node) {
            struct TableManagerLinkedListNode * next_node = node->next;
            free(node->name);
            free(node);
            node = next_node;
        }
        free(state->t_name);
        free(state->p_name);
        free(state->n_name);
        free(state);
    }
}

void table_manager_state_alloc() {
    if (_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: global state is already allocated.\n");
        return;
    }
    _tof_table_manager_state = _table_manager_state_alloc();
    if (!_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate global state.\n");
    }
}
void table_manager_state_free() {
    _table_manager_state_free(_tof_table_manager_state);
    _tof_table_manager_state = NULL;
}

void _set_name_with_suffix_index(char ** name_field, const char * base_name, int index) {
    size_t base_len = strlen(base_name);
    size_t suffix_len = (size_t) (1 + (size_t) log10((double)index + 1.0)); // Calculate length of suffix based on index value
    char * new_name = (char *) malloc(base_len + suffix_len + 2); // +1 for underscore, +1 for null terminator
    if (!new_name) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for name with suffix.\n");
        return;
    }
    sprintf(new_name, "%s_%d", base_name, index);
    free(*name_field);
    *name_field = new_name;
}

void table_manager_state_finalize(int manager_index, const char * t_name, const char * p_name, const char * n_name){
    if (!_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before finalizing.\n");
        return;
    }
    _set_name_with_suffix_index(&_tof_table_manager_state->t_name, t_name, manager_index);
    _set_name_with_suffix_index(&_tof_table_manager_state->p_name, p_name, manager_index);
    _set_name_with_suffix_index(&_tof_table_manager_state->n_name, n_name, manager_index);
}

void table_manager_particle_alloc(_class_particle * p, double t_zero) {
  double ** tof_t_ptr = table_manager_particle_t_array_ptr(p);
  double ** tof_p_ptr = table_manager_particle_p_array_ptr(p);
  int * tof_n_ptr = table_manager_particle_n_ptr(p);
  if (!tof_t_ptr || !tof_p_ptr || !tof_n_ptr) {
    fprintf(stderr, "TableManager ERROR: Failed to access per-particle time or probability arrays for allocation.\n");
    return;
  }
  // Allocate the arrays with initial size 0. TableManager will realloc as needed.
  *tof_t_ptr = NULL;
  *tof_p_ptr = NULL;
  *tof_n_ptr = 0;

  if (_tof_table_manager_state->n_recorders > 0) {
    *tof_t_ptr = (double *) malloc(sizeof(double) * _tof_table_manager_state->n_recorders);
    *tof_p_ptr = (double *) malloc(sizeof(double) * _tof_table_manager_state->n_recorders);
    *tof_n_ptr = _tof_table_manager_state->n_recorders;
    if (!*tof_t_ptr || !*tof_p_ptr || !*tof_n_ptr) {
      fprintf(stderr, "TableManager ERROR: Failed to allocate memory for per-particle time or probability arrays.\n");
      free(*tof_t_ptr);
      free(*tof_p_ptr);
      *tof_t_ptr = NULL;
      *tof_p_ptr = NULL;
      *tof_n_ptr = 0;
    }
  }
  if (*tof_t_ptr && *tof_p_ptr && *tof_n_ptr > 0) {
    for (int i = 0; i < *tof_n_ptr; i++) {
      (*tof_t_ptr)[i] = t_zero;
      (*tof_p_ptr)[i] = 0.0;
    }
  } 
}


int table_manager_state_add_recorder(const char * name, double distance){
    if (!_tof_table_manager_state) {
        fprintf(stderr, "TableManager ERROR: state must be allocated by TableSetup before adding recorders.\n");
        return -1;
    }
    // Extend the linked lists with the new recorder info
    struct TableManagerLinkedListNode * tail, * node;
    tail = _tof_table_manager_state->recorders.tail;
    node = (struct TableManagerLinkedListNode *) malloc(sizeof(struct TableManagerLinkedListNode));
    if (!node) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for new recorder linked list nodes.\n");
        free(node);
        return -1;
    }
    node->name = malloc(strlen(name) + 1);
    if (!node->name) {
        fprintf(stderr, "TableManager ERROR: Failed to allocate memory for new recorder name string.\n");
        free(node);
        return -1;
    }
    sprintf(node->name, "%s", name);
    node->distance = distance;
    node->next = NULL;
    if (tail) {
        tail->next = node;
    } else {
        _tof_table_manager_state->recorders.head = node;
    }
    _tof_table_manager_state->recorders.tail = node;
    return _tof_table_manager_state->n_recorders++;
}


int table_manager_state_exists(){
    return _tof_table_manager_state != NULL;
}

int table_manager_particle_record(_class_particle * p, int recorder_index){
    double * tof_t_ptr = *table_manager_particle_t_array_ptr(p);
    double * tof_p_ptr = *table_manager_particle_p_array_ptr(p);
    int * tof_n_ptr = table_manager_particle_n_ptr(p);
    if (!tof_t_ptr || !tof_p_ptr || !tof_n_ptr) {
        fprintf(stderr, "TableManager ERROR: Failed to access per-particle time or probability arrays for recording.\n");
        return -1;
    }
    if (recorder_index < 0 || recorder_index >= *tof_n_ptr) {
        fprintf(stderr, "TableManager ERROR: Recorder index out of bounds when recording particle data.\n");
        return -1;
    }
    // Record the time and probability at this recorder index. This example just sets them to dummy values.
    tof_t_ptr[recorder_index] = p->t; // Replace with actual time calculation
    tof_p_ptr[recorder_index] = p->p; // Replace with actual probability calculation
    return 0;
}

int table_manager_particle_to_table(_class_particle * p, struct TableManagerData * data) {
    double * tof_t_ptr = *table_manager_particle_t_array_ptr(p);
    double * tof_p_ptr = *table_manager_particle_p_array_ptr(p);
    int * tof_n_ptr = table_manager_particle_n_ptr(p);
    if (!tof_t_ptr || !tof_p_ptr || !tof_n_ptr) {
        fprintf(stderr, "TableManager ERROR: Failed to access per-particle time or probability arrays for transfer to table.\n");
        return -1;
    }
    if (*tof_n_ptr != data->recorders) {
        fprintf(stderr, "TableManager ERROR: Number of recorders in particle data does not match number of recorders in table data during transfer.\n");
        return -1;
    }
    #pragma omp critical
    {    // Transfer the time and probability data from the particle arrays to the appropriate place in the table data.
        for (int i=0; i< data->recorders; ++i){
            double t = tof_t_ptr[i];
            double p = tof_p_ptr[i];
            int j = (int) ((t - data->t_min) / (data->t_max - data->t_min) * data->bins);
            if (j < 0 || j >= data->bins){
                continue; // Skip out-of-bounds times
            }
            int index = i * data->bins + j;
            data->p1[index] += p;
            data->p2[index] += p * p;
            data->tp[index] += t * p;
            data->n[index] += 1;
        }
    }
    return 0;
}

int table_manager_particle_free(_class_particle * p) {
    double ** tof_t_ptr = table_manager_particle_t_array_ptr(p);
    double ** tof_p_ptr = table_manager_particle_p_array_ptr(p);
    int * tof_n_ptr = table_manager_particle_n_ptr(p);
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


int table_manager_write_output_file(const char * filename, struct TableManagerData * data) {
    FILE * f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "TableManager ERROR: Failed to open file '%s' for writing.\n", filename);
        return -1;
    }
    // Write the data in JSON format
    if (fprintf(f, "{\n") < 0) {
        fprintf(stderr, "TableManager ERROR: Failed to write to file '%s'.\n", filename);
        fclose(f);
        return -1;
    }
    if (table_manager_json_indent(f, 1) < 0 ||
        fprintf(f, "\"dims\": {\"time\": %d, \"recorders\": %d},\n", data->bins, data->recorders) < 0 ||
        table_manager_json_indent(f, 1) < 0 ||
        fprintf(f, "\"coords\": [\n") < 0 ||
        table_manager_json_recorder_info(f, 2) < 0 ||
        fprintf(f, ",\n") < 0 ||
        // table_manager_json_indent(f, 1) < 0 ||
        table_manager_json_time_info(f, 2, data) < 0 ||
        fprintf(f, ",\n") < 0 ||
        table_manager_json_indent(f, 1) < 0 ||
        fprintf(f, "],\n") < 0 ||
        table_manager_json_indent(f, 1) < 0 ||
        fprintf(f, "\"tp\": ") < 0 ||
        table_manager_json_matrix_double(f, data->tp, data->recorders, data->bins, 2) < 0 ||
        fprintf(f, ",\n") < 0 ||
        table_manager_json_indent(f, 1) < 0 ||
        fprintf(f, "\"p1\": ") < 0 ||
        table_manager_json_matrix_double(f, data->p1, data->recorders, data->bins, 2) < 0 ||
        fprintf(f, ",\n") < 0 ||
        table_manager_json_indent(f, 1) < 0 ||
        fprintf(f, "\"p2\": ") < 0 ||
        table_manager_json_matrix_double(f, data->p2, data->recorders, data->bins, 2) < 0 ||
        fprintf(f, ",\n") < 0 ||
        table_manager_json_indent(f, 1) < 0 ||
        fprintf(f, "\"n\": ") < 0 ||
        table_manager_json_matrix_int(f, data->n, data->recorders, data->bins, 2) < 0 ||
        fprintf(f, "\n}\n") < 0) {
            fprintf(stderr, "TableManager ERROR: Failed to write to file '%s'.\n", filename);
            fclose(f);
            return -1;
    }
    fclose(f);
    return 0;
}