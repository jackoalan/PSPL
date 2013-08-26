//
//  PMDLRuntimeRigging.h
//  PSPL
//
//  Created by Jack Andersen on 8/26/13.
//
//

#ifndef PSPL_PMDLRuntimeRigging_h
#define PSPL_PMDLRuntimeRigging_h

#include <PMDLRuntime.h>


typedef struct {
    
    
    
} pmdl_animation_action;


typedef struct {
    
} pmdl_animation_ctx;


typedef struct {
    
    // Name in PMDL string table
    const char* bone_name;
    
    // Currently
    
} pmdl_bone;


typedef struct {
    
    // Dynamic bone context array
    uint32_t bone_count;
    pmdl_bone* bone_array;
    
    // Current animation context
    pmdl_animation_ctx* anim_ctx;
    
} pmdl_rigging_ctx;

/* Routine to init rigging context */
void pmdl_rigging_init(pmdl_rigging_ctx* rig_ctx);

/* Routine to destroy rigging context */
void pmdl_rigging_destroy(pmdl_rigging_ctx* rig_ctx);


/* Routine to look up bone structure */
pmdl_bone* pmdl_lookup_bone(pmdl_rigging_ctx* rig_ctx);


#endif
