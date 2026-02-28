#ifndef TOF_TABLE_LIB_H
#define TOF_TABLE_LIB_H

#ifndef MCSTAS
#include <stdlib.h>
#include <stdio.h>
struct _struct_particle {
    double t;
    double p;
    double * table_manager_t_9;
    double * table_manager_p_9;
    int table_manager_n_9;
};
typedef struct _struct_particle _class_particle;
#endif /* MCSTAS */

struct TableManagerLinkedListNode {
    char * name;
    double distance;
    struct TableManagerLinkedListNode * next;
};
struct TableManagerLinkedList {
    struct TableManagerLinkedListNode * head;
    struct TableManagerLinkedListNode * tail;
    int size;
};

struct TableManagerState {
  int n_recorders;
  struct TableManagerLinkedList recorders;
  char * t_name;
  char * p_name;
  char * n_name;
};

static struct TableManagerState * _tof_table_manager_state = NULL;

struct TableManagerData {
  int bins;
  int recorders;
  double t_min;
  double t_max;
  double * tp;
  double * p1;
  double * p2;
  int * n;
};

struct TableManagerData * table_manager_data_alloc(int recorders, int bins, double t_min, double t_max);
void table_manager_data_free(struct TableManagerData * data);

void table_manager_state_alloc();
void table_manager_state_free();

int table_manager_state_exists();
int table_manager_state_add_recorder(const char * name, double distance);

int table_manager_json_indent(FILE * f, int level);

int table_manager_json_array_double(FILE * f, double * x, int n);

int table_manager_json_array_int(FILE * f, int * x, int n);

int table_manager_json_array_string(FILE * f, char ** x, int n);

int table_manager_json_matrix_double(FILE * f, double * x, int m, int n, int indent_level);

int table_manager_json_matrix_int(FILE * f, int * x, int m, int n, int indent_level);

double ** table_manager_particle_t_array_ptr(_class_particle * p);
double ** table_manager_particle_p_array_ptr(_class_particle * p);
int * table_manager_particle_n_ptr(_class_particle * p);

void table_manager_particle_alloc(_class_particle * p, double t_zero);

int table_manager_particle_record(_class_particle * p, int recorder_index);

int table_manager_particle_to_table(_class_particle * p, struct TableManagerData * data);

int table_manager_particle_free(_class_particle * p);
#endif