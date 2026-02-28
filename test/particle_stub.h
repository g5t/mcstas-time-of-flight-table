/* Stub for particle_getvar_void, the McStas runtime accessor used by
 * tof-table-lib.c to read/write per-particle user fields.
 *
 * Outside of McStas the _struct_particle is defined in tof-table-lib.h with
 * hardcoded field names table_manager_t_9 / _p_9 / _n_9.  Tests must
 * therefore call table_manager_state_finalize(9, "table_manager_t",
 * "table_manager_p", "table_manager_n") so the state's name strings match
 * the struct field names handled here.
 */
#ifndef PARTICLE_STUB_H
#define PARTICLE_STUB_H

#ifndef MCSTAS
#include "tof-table-lib.h"
#include <string.h>

void * particle_getvar_void(_class_particle * p, char * name, int *success);

#endif /* MCSTAS */
#endif /* PARTICLE_STUB_H */
