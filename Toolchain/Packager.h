//
//  Packager.h
//  PSPL
//
//  Created by Jack Andersen on 4/28/13.
//
//

#ifndef PSPL_Packager_h
#define PSPL_Packager_h
#ifdef PSPL_INTERNAL

#include "Driver.h"

/* Write out to PSPLP file */
void pspl_packager_write_psplp(pspl_indexer_context_t* ctx,
                               FILE* psplp_file_out);

#endif // PSPL_INTERNAL
#endif

