//
//  PMDLRuntimeProcessing.h
//  PSPL
//
//  Created by Jack Andersen on 7/31/13.
//
//

#ifndef PSPL_PMDLRuntimeProcessing_h
#define PSPL_PMDLRuntimeProcessing_h

/* Used by PMDL runtime extension to initialise and destroy PMDLs */
int pmdl_init(const pspl_runtime_arc_file_t* pmdl_file);
void pmdl_destroy(const pspl_runtime_arc_file_t* pmdl_file);

#endif
