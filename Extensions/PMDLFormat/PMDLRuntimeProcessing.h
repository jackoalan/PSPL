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
#include <PMDLRuntime.h>

/* Call once at program init time */
void pmdl_master_init();

/* Used by PMDL runtime extension to initialise and destroy PMDLs */
int pmdl_init(pmdl_t* pmdl);
void pmdl_destroy(pmdl_t* pmdl);

/* Rigging context */
void pmdl_rigging_init(pmdl_rigging_ctx** rig_ctx, const void* file_data, const char* bone_string_table);
void pmdl_rigging_destroy(pmdl_rigging_ctx** rig_ctx);

#endif
