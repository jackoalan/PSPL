//
//  PMDLRuntimeRigging.h
//  PSPL
//
//  Created by Jack Andersen on 8/26/13.
//
//

#ifndef PSPL_PMDLRuntimeRigging_h
#define PSPL_PMDLRuntimeRigging_h

#include <PSPLRuntime.h>

struct pmdl_rigging_ctx;
struct pmdl_animation_ctx;

#pragma mark Skeleton

/* In-memory bone representation with internal tree structures */
typedef struct pmdl_bone {
    
    // Name in PMDL string table
    const char* bone_name;
    
    // Parent bone
    const struct pmdl_bone* parent;
    
    // Child bones
    unsigned child_count;
    const struct pmdl_bone** child_arr;
    
    // Base vector
    const pspl_vector3_t* base_vector;
    
} pmdl_bone;


#pragma mark Skinning

/* In-memory skin entry representation */
typedef struct {
    
    // Bone array (with indexing utilised by geometry data)
    unsigned bone_count;
    const pmdl_bone* bone_array;
    
} pmdl_skin_entry;


#pragma mark Animation

/* In-memory curve keyframe representation (matches file structure) */
typedef struct {
    
    float left_handle_time;
    float left_handle_value;
    float main_handle_time;
    float main_handle_value;
    float right_handle_time;
    float right_handle_value;
    
} pmdl_curve_keyframe;

/* In-memory curve representation (matches file structure) */
typedef struct {
    
    uint32_t keyframe_count;
    const pmdl_curve_keyframe keyframe_array[];
    
} pmdl_curve;

/* In-memory bone animation track representation */
typedef struct {
    
    // Index of bone being adjusted (original indexing space)
    unsigned bone_index;
    
    // Property curves being animated (unavailable curves set to NULL)
    const pmdl_curve* scale_x;
    const pmdl_curve* scale_y;
    const pmdl_curve* scale_z;
    
    const pmdl_curve* rotation_w;
    const pmdl_curve* rotation_x;
    const pmdl_curve* rotation_y;
    const pmdl_curve* rotation_z;
    
    const pmdl_curve* location_x;
    const pmdl_curve* location_y;
    const pmdl_curve* location_z;
    
} pmdl_action_bone_track;

/* In-memory action representation */
typedef struct {
    
    // Name in PMDL string table
    const char* action_name;
    
    // Action duration (in original time units)
    float action_duration;
    
    // Parent rigging context
    const struct pmdl_rigging_ctx* parent_ctx;
    
    // Animation tracks for indexed bones
    unsigned bone_track_count;
    const pmdl_action_bone_track* bone_track_array;
    
} pmdl_action;


#pragma mark Root Rigging Context

/* In-memory rigging context (one for each loaded PAR1 PMDL) */
typedef struct pmdl_rigging_ctx {
    
    // Bone array (with original indexing from file)
    unsigned bone_count;
    const pmdl_bone* bone_array;
    
    // Root bone array (bones without parents)
    unsigned root_bone_count;
    const pmdl_bone** root_bone_array;
    
    // Skin entry array (with indexing utilised by geometry data)
    unsigned skin_entry_count;
    const pmdl_skin_entry* skin_entry_array;
    
    // Action array (with original indexing from file)
    unsigned action_count;
    const pmdl_action* action_array;
    
} pmdl_rigging_ctx;


/* Routine to look up bone structure */
pmdl_bone* pmdl_lookup_bone(pmdl_rigging_ctx* rig_ctx);


#pragma mark Animation API Context

/* Curve context representation (for animating bézier curve instances) */
typedef struct {
    
    // Curve being animated
    const pmdl_curve* curve;
    
    // Last-processed keyframe index to left of playback position
    // Set to -1 by default or if no keyframe to left
    int prev_kf_idx;
    
    // Cached value computed with bézier function at last advance
    float cached_value;
    
} pmdl_curve_playback;

/* Action event function type */
typedef void(*pmdl_animation_event)(struct pmdl_animation_ctx* animation_ctx);

/* Application-allocatable structure used to playback actions
 * Multiple contexts may be established for instanced playback */
typedef struct pmdl_animation_ctx {
    
    // Selected action to playback
    const pmdl_action* action;
    
    
    // Playback-rate (original time units per second; negative to reverse)
    float playback_rate;
    
    // Previous absolute time (to calculate advancement delta)
    int have_abs_time;
    float previous_abs_time;
    
    // Current time value for animation (in original time units)
    float current_time;
    
    // Curve playback instance array (indexing corresponds to active bone tracks in `pmdl_action`)
    pmdl_curve_playback* curve_instance_array;
    
    
    // Loop flag (set to automatically loop action)
    int loop_flag;
    
    
    // Event callbacks
    pmdl_animation_event pre_loop_point_cb;
    pmdl_animation_event post_loop_point_cb;
    
} pmdl_animation_ctx;

/* Routine to init animation context */
void pmdl_animation_init(pmdl_animation_ctx* ctx_ptr, const pmdl_action* action);

/* Routine to advance animation context */
void pmdl_animation_advance(pmdl_animation_ctx* ctx_ptr);
 
 
 #endif
