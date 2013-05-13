//
//  ReferenceGatherer.h
//  PSPL
//
//  Created by Jack Andersen on 5/12/13.
//
//

#ifndef PSPL_ReferenceGatherer_h
#define PSPL_ReferenceGatherer_h
#ifdef PSPL_INTERNAL

/* The Reference Gather stores the necessary state to generate
 * a CMake include file that will augment the CMake dependencies 
 * of a generated PSPLC file */

/* Gatherer Context Type */
typedef struct {
    
    // Path of generated CMake include file
    const char* cmake_path;
    
    // Count, capacity and array of absolute referenced paths
    unsigned int ref_count;
    unsigned int ref_cap;
    const char** ref_array;
    
    // Previous refs (loaded from file)
    unsigned int ref_old_count;
    unsigned int ref_old_cap;
    const char** ref_old_array;
    
    // Changes made (after initial load)
    uint8_t changes_made;
    
} pspl_gatherer_context_t;

/* Init context and load (or stage creation of) include file */
void pspl_gather_load_cmake(pspl_gatherer_context_t* ctx, const char* cmake_path);

/* Add referenced file (absolute path) to gatherer */
void pspl_gather_add_file(pspl_gatherer_context_t* ctx, const char* file);

/* Finish context and write to disk */
void pspl_gather_finish(pspl_gatherer_context_t* ctx, const char* psplc_file);

#endif // PSPL_INTERNAL
#endif
