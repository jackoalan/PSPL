//
//  PMDLRuntimeProcessing.h
//  PSPL
//
//  Created by Jack Andersen on 7/31/13.
//
//

#ifndef PSPL_PMDLRuntimeProcessing_h
#define PSPL_PMDLRuntimeProcessing_h

#include <PMDLRuntimeRigging.h>

/* Used by PMDL runtime extension to initialise and destroy PMDLs */
int pmdl_init(const pspl_runtime_arc_file_t* pmdl_file, pmdl_rigging_ctx** rigging_ptr);
void pmdl_destroy(const pspl_runtime_arc_file_t* pmdl_file, pmdl_rigging_ctx** rigging_ptr);

/* Rigging context */
void pmdl_rigging_init(pmdl_rigging_ctx** rig_ctx, const void* file_data, const char* bone_string_table);
void pmdl_rigging_destroy(pmdl_rigging_ctx** rig_ctx);

#endif
