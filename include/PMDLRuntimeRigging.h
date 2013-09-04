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

typedef float float2[2];

struct pmdl_rigging_ctx;
struct pmdl_action_ctx;

#pragma mark Skeleton

/* In-memory bone representation with internal tree structures */
typedef struct pmdl_bone {
    
    // Name in PMDL string table
    const char* bone_name;
    
    // Original bone index
    unsigned bone_index;
    
    // Parent bone
    const struct pmdl_bone* parent;
    
    // Child bones
    unsigned child_count;
    const struct pmdl_bone** child_arr;
    
    // Base vector (Copied from file for alignment purposes)
    const pspl_vector4_t* base_vector;
    
} pmdl_bone;


#pragma mark Skinning

/* In-memory skin entry representation */
typedef struct {
    
    // Bone array (with indexing utilised by geometry data)
    unsigned bone_count;
    const pmdl_bone** bone_array;
    
} pmdl_skin_entry;


#pragma mark Animation

/* In-memory curve keyframe representation (matches file structure) */
typedef struct {
    
    float2 left_handle;
    float2 main_handle;
    float2 right_handle;
    
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
    unsigned property_count;
    
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


#pragma mark Action API Context

/* Curve context representation (for animating bézier curve instances) */
typedef struct {
    
    // Index of the bone being animated
    unsigned bone_index;
    
    // Curve being animated
    const pmdl_curve* curve;
    
    // Cached value computed with bézier function at last advance
    float cached_value;
    
} pmdl_curve_playback;

/* Action event function type */
typedef void(*pmdl_animation_event)(struct pmdl_action_ctx* animation_ctx);

/* Structure used to playback actions
 * Multiple contexts may be established for instanced playback */
typedef struct pmdl_action_ctx {
    
    // Selected action to playback (do not change!!!)
    const pmdl_action* action;
    
    
    // Current time value for action (in original time units)
    double current_time;
    
    // Curve playback instance array (indexing corresponds to active bone tracks in `pmdl_action`)
    unsigned curve_instance_count;
    pmdl_curve_playback* curve_instance_array;
    
    
    // Loop flag (set to automatically loop action)
    int loop_flag;
    
    
    // Event callbacks
    pmdl_animation_event anim_loop_point_cb;
    pmdl_animation_event anim_end_cb;
    
} pmdl_action_ctx;

/* Routine to init action context */
pmdl_action_ctx* pmdl_action_init(const pmdl_action* action);

/* Routine to destroy action context */
void pmdl_action_destroy(pmdl_action_ctx* ctx_ptr);

/* Routine to advance action context */
void pmdl_action_advance(pmdl_action_ctx* ctx_ptr, double time_delta);

/* Routine to rewind action context (call advance afterwards before drawing) */
#define pmdl_action_rewind(ctx_ptr) (ctx_ptr)->have_abs_time = 0


#pragma mark Animation API Context

/* Referenced within `fk_playback` to compose individual actions together */
typedef struct {
    
    // Bone animation track from action data
    const pmdl_action_bone_track* bone_anim_track;
    
    // First curve-playback instance for this bone within animation context
    const pmdl_curve_playback* first_curve_instance;
    
} pmdl_fk_action_playback;

/* Forward kinematic representation (for converting evaluated curves to bone matrices) */
typedef struct pmdl_fk_playback {
    
    // Original bone data
    const pmdl_bone* bone;
    
    // Array of per-action animation data references
    const pmdl_fk_action_playback* action_playback_array;
    
    // Parent FK instance (or NULL if none)
    struct pmdl_fk_playback* parent_fk;
    
    // Blended values
    int is_animated;
    pspl_vector3_t scale_blend;
    pspl_vector4_t rotation_blend;
    pspl_vector3_t location_blend;
    
    // Cumulative bone transformation matrix
    pspl_matrix34_t* bone_matrix;
    
    // Evaluation flip-bit (alternates 0/1 to synchronise with other FK instances)
    char eval_flip_bit;
    
} pmdl_fk_playback;

/* Structure to represent animation context
 * An animation context mixes together a set of action contexts
 * for cumulative skeletal animation compositions */
typedef struct {
    
    // Common parent rigging context
    const pmdl_rigging_ctx* parent_ctx;
    
    // Action context array
    unsigned action_ctx_count;
    const pmdl_action_ctx** action_ctx_array;
    
    // Forward-kinematic rigging instance array (indexing corresponds to original bone indexing from file)
    unsigned fk_instance_count;
    pmdl_fk_playback* fk_instance_array;
    
    // Master FK flip-bit
    char master_fk_flip_bit;
    
} pmdl_animation_ctx;


/* Routine to init animation context */
pmdl_animation_ctx* pmdl_animation_init(unsigned action_ctx_count,
                                        const pmdl_action_ctx** action_ctx_array);
/* Variadic version of the array initialiser above (please NULL terminate) */
pmdl_animation_ctx* pmdl_animation_initv(const pmdl_action_ctx* first_action_ctx, ...);

/* Composites and evaluates bone transformation matrices from contained action contexts
 * Call *before* drawing a rigged model */
void pmdl_animation_evaluate(pmdl_animation_ctx* ctx_ptr);

/* Routine to destroy animation context */
void pmdl_animation_destroy(pmdl_animation_ctx* ctx_ptr);
 
 
#endif
