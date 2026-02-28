#include "particle_stub.h"

void * particle_getvar_void(_class_particle *p, char *name, int *success){
#ifndef OPENACC
#define str_comp strcmp
#endif
    int s=1;
    void* rval=0;
    if (!str_comp("t", name)){rval=(void * ) & (p->t);s=0;}
    if (!str_comp("p", name)){rval=(void * ) & (p->p);s=0;}
    if (!str_comp("table_manager_t_9", name)){rval=(void * ) & (p->table_manager_t_9);s=0;}
    if (!str_comp("table_manager_p_9", name)){rval=(void * ) & (p->table_manager_p_9);s=0;}
    if (!str_comp("table_manager_n_9", name)){rval=(void * ) & (p->table_manager_n_9);s=0;}
    if (success!=0x0) {*success=s;}
    return rval;
}