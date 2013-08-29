//
//  PMDLRuntimeRigging.c
//  PSPL
//
//  Created by Jack Andersen on 8/26/13.
//
//

#include <stdio.h>
#include <math.h>
#include <PMDLRuntimeRigging.h>
#include "PMDLRuntimeProcessing.h"

#pragma pack(1)
struct file_bone {
    uint32_t string_off;
    pspl_vector3_t bone_head;
    int32_t parent_index;
    uint32_t child_count;
    uint32_t child_array[];
} __attribute__((__packed__));
#pragma pack()

#pragma pack(1)
struct file_skin {
    uint32_t bone_count;
    uint32_t bone_indices[];
} __attribute__((__packed__));
#pragma pack()

#pragma pack(1)
struct file_action {
    uint32_t bone_count;
    float action_duration;
} __attribute__((__packed__));
#pragma pack()

#define ACTION_BONE_IDX(val) ((val)&0x1FFFFFFF)
#define ACTION_SCALE(val) ((val)&0x20000000)
#define ACTION_ROTATION(val) ((val)&0x40000000)
#define ACTION_LOCATION(val) ((val)&0x80000000)


/* Routine to init rigging context */
void pmdl_rigging_init(pmdl_rigging_ctx** rig_ctx, const void* file_data, const char* bone_string_table) {
    const void* file_cur = file_data;
    int i,j;
    
    // Establish skeleton section
    uint32_t skel_section_length = *(uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    
    uint32_t bone_count = *(uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    
    uint32_t* bone_offsets = (uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    
    const void* bone_arr = file_cur;
    
    // Count root bones and add up child array sizes
    unsigned root_bone_count = 0;
    unsigned child_array_counts = 0;
    for (i=0 ; i<bone_count ; ++i) {
        struct file_bone* bone = (struct file_bone*)(bone_arr + bone_offsets[i]);
        if (bone->parent_index < 0)
            ++root_bone_count;
        child_array_counts += bone->child_count;
    }
    
    
    // Establish skinning section
    const void* skinning_section = file_data + skel_section_length;
    file_cur = skinning_section;
    uint32_t skinning_section_length = *(uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    
    uint32_t skin_count = *(uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    
    uint32_t* skin_offsets = (uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    
    const void* skin_arr = file_cur;
    
    // Add up bone array sizes
    unsigned skin_bone_array_count = 0;
    for (i=0 ; i<skin_count ; ++i) {
        struct file_skin* skin = (struct file_skin*)(skin_arr + skin_offsets[i]);
        skin_bone_array_count += skin->bone_count;
    }
    
    
    // Establish animation section
    const void* animation_section = skinning_section + skinning_section_length;
    file_cur = animation_section;
    
    uint32_t action_count = *(uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    
    uint32_t action_strings_off = *(uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    const uint32_t* action_string_offsets = animation_section + action_strings_off;
    const void* action_strings = animation_section + action_strings_off + (sizeof(uint32_t)*action_count);
    
    uint32_t* action_offsets = (uint32_t*)(file_cur);
    file_cur += sizeof(uint32_t);
    
    const void* action_arr = file_cur;
    
    // Add up bone track count
    unsigned bone_track_count = 0;
    for (i=0 ; i<action_count ; ++i) {
        struct file_action* action = (struct file_action*)(action_arr + action_offsets[i]);
        bone_track_count += action->bone_count;
    }
    
    
    // Allocate context block
    size_t block_size = sizeof(pmdl_rigging_ctx);
    block_size += sizeof(pmdl_bone)*bone_count;
    block_size += sizeof(pmdl_bone*)*child_array_counts;
    block_size += sizeof(pmdl_bone*)*root_bone_count;
    block_size += sizeof(pmdl_skin_entry)*skin_count;
    block_size += sizeof(pmdl_bone*)*skin_bone_array_count;
    block_size += sizeof(pmdl_action)*action_count;
    block_size += sizeof(pmdl_action_bone_track)*bone_track_count;
    
    void* context_block = pspl_allocate_media_block(block_size);
    void* context_cur = context_block;
    
    // Incorporate intra-block references and data
    pmdl_rigging_ctx* rigging_ctx = context_cur;
    context_cur += sizeof(pmdl_rigging_ctx);
    
    // Bones
    rigging_ctx->bone_count = bone_count;
    rigging_ctx->bone_array = context_cur;
    context_cur += sizeof(pmdl_bone)*bone_count;
    
    for (i=0 ; i<bone_count ; ++i) {
        
        struct file_bone* bone = (struct file_bone*)(bone_arr + bone_offsets[i]);
        pmdl_bone* target_bone = (pmdl_bone*)&rigging_ctx->bone_array[i];
        
        target_bone->bone_name = bone_string_table + bone->string_off;
        
        if (bone->parent_index < 0)
            target_bone->parent = NULL;
        else
            target_bone->parent = &rigging_ctx->bone_array[bone->parent_index];
        
        target_bone->child_count = bone->child_count;
        target_bone->child_arr = context_cur;
        pmdl_bone** child_arr_writer = context_cur;
        for (j=0 ; j<bone->child_count ; ++j)
            child_arr_writer[j] = (pmdl_bone*)&rigging_ctx->bone_array[bone->child_array[j]];
        context_cur += sizeof(pmdl_bone*)*bone->child_count;

        target_bone->base_vector = &bone->bone_head;
        
    }
    
    rigging_ctx->root_bone_count = root_bone_count;
    rigging_ctx->root_bone_array = context_cur;
    pmdl_bone** root_bone_arr_writer = context_cur;
    j=0;
    for (i=0 ; i<bone_count ; ++i) {
        struct file_bone* bone = (struct file_bone*)(bone_arr + bone_offsets[i]);
        if (bone->parent_index < 0)
            root_bone_arr_writer[j++] = (pmdl_bone*)&rigging_ctx->bone_array[i];
    }
    context_cur += sizeof(pmdl_bone*)*root_bone_count;

    
    // Skin Entries
    rigging_ctx->skin_entry_count = skin_count;
    rigging_ctx->skin_entry_array = context_cur;
    context_cur += sizeof(pmdl_skin_entry)*skin_count;
    
    for (i=0 ; i<skin_count ; ++i) {
        
        struct file_skin* skin = (struct file_skin*)(skin_arr + skin_offsets[i]);
        pmdl_skin_entry* target_skin = (pmdl_skin_entry*)&rigging_ctx->skin_entry_array[i];
        
        target_skin->bone_count = skin->bone_count;
        target_skin->bone_array = context_cur;
        pmdl_bone** bone_arr_writer = context_cur;
        for (j=0 ; j<skin->bone_count ; ++j)
            bone_arr_writer[j] = (pmdl_bone*)&rigging_ctx->bone_array[skin->bone_indices[j]];
        context_cur += sizeof(pmdl_bone*)*skin->bone_count;
        
    }
    
    
    // Animation entries
    rigging_ctx->action_count = action_count;
    rigging_ctx->action_array = context_cur;
    context_cur += sizeof(pmdl_action)*action_count;
    
    for (i=0 ; i<action_count ; ++i) {
        
        const void* action = (const void*)(action_arr + action_offsets[i]);
        struct file_action* action_head = (struct file_action*)(action);
        pmdl_action* target_action = (pmdl_action*)&rigging_ctx->action_array[i];
        action += sizeof(struct file_action);
        
        target_action->action_name = action_strings + action_string_offsets[i];
        
        target_action->action_duration = action_head->action_duration;
        
        target_action->parent_ctx = rigging_ctx;
        
        target_action->bone_track_count = action_head->bone_count;
        target_action->bone_track_array = context_cur;
        pmdl_action_bone_track* bone_arr_writer = context_cur;
        for (j=0 ; j<action_head->bone_count ; ++j) {
            
            pmdl_action_bone_track* track = &bone_arr_writer[j];
            
            uint32_t track_head = *(uint32_t*)action;
            action += sizeof(uint32_t);
            
            track->bone_index = ACTION_BONE_IDX(track_head);
            track->property_count = 0;
            
            if (ACTION_SCALE(track_head)) {
                track->property_count += 3;
                track->scale_x = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->scale_x->keyframe_count + sizeof(uint32_t);
                track->scale_y = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->scale_y->keyframe_count + sizeof(uint32_t);
                track->scale_z = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->scale_z->keyframe_count + sizeof(uint32_t);
            } else {
                track->scale_x = NULL;
                track->scale_y = NULL;
                track->scale_z = NULL;
            }
            
            if (ACTION_ROTATION(track_head)) {
                track->property_count += 4;
                track->rotation_w = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->rotation_w->keyframe_count + sizeof(uint32_t);
                track->rotation_x = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->rotation_x->keyframe_count + sizeof(uint32_t);
                track->rotation_y = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->rotation_y->keyframe_count + sizeof(uint32_t);
                track->rotation_z = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->rotation_z->keyframe_count + sizeof(uint32_t);
            } else {
                track->rotation_w = NULL;
                track->rotation_x = NULL;
                track->rotation_y = NULL;
                track->rotation_z = NULL;
            }
            
            if (ACTION_LOCATION(track_head)) {
                track->property_count += 3;
                track->location_x = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->location_x->keyframe_count + sizeof(uint32_t);
                track->location_y = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->location_y->keyframe_count + sizeof(uint32_t);
                track->location_z = (const pmdl_curve*)&action;
                action += sizeof(pmdl_curve_keyframe)*track->location_z->keyframe_count + sizeof(uint32_t);
            } else {
                track->location_x = NULL;
                track->location_y = NULL;
                track->location_z = NULL;
            }
            
        }
        context_cur += sizeof(pmdl_action_bone_track)*action_head->bone_count;
        
    }

    
    *rig_ctx = rigging_ctx;
    
}

/* Routine to destroy rigging context */
void pmdl_rigging_destroy(pmdl_rigging_ctx** rig_ctx) {
    pspl_free_media_block(*rig_ctx);
}



/* Routine to init animation context */
pmdl_animation_ctx* pmdl_animation_init(const pmdl_action* action) {
    int i,j;
    if (!action)
        return NULL;
    
    // Determine size of context block
    size_t block_size = sizeof(pmdl_animation_ctx);
    
    unsigned curve_instance_count = 0;
    for (i=0 ; i<action->bone_track_count; ++i) {
        const pmdl_action_bone_track* bone_track = &action->bone_track_array[i];
        curve_instance_count += bone_track->property_count;
    }
    
    block_size += sizeof(pmdl_curve_playback)*curve_instance_count;
    
    // Allocate context block
    void* context_block = pspl_allocate_media_block(block_size);
    pmdl_animation_ctx* new_ctx = context_block;
    new_ctx->action = action;
    new_ctx->playback_rate = 1.0;
    new_ctx->have_abs_time = 0;
    new_ctx->current_time = 0.0;
    new_ctx->curve_instance_count = curve_instance_count;
    new_ctx->curve_instance_array = context_block + sizeof(pmdl_animation_ctx);
    new_ctx->loop_flag = 0;
    new_ctx->anim_loop_point_cb = NULL;
    new_ctx->anim_end_cb = NULL;
    
    // Populate curve instance array
    j=0;
    for (i=0 ; i<action->bone_track_count; ++i) {
        
        const pmdl_action_bone_track* bone_track = &action->bone_track_array[i];
        pmdl_curve_playback* target_track = NULL;
        
        if (bone_track->scale_x) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->scale_x;
        }
        if (bone_track->scale_y) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->scale_y;
        }
        if (bone_track->scale_z) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->scale_z;
        }
        
        if (bone_track->rotation_w) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->rotation_w;
        }
        if (bone_track->rotation_x) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->rotation_x;
        }
        if (bone_track->rotation_y) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->rotation_y;
        }
        if (bone_track->rotation_z) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->rotation_z;
        }
        
        if (bone_track->location_x) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->location_x;
        }
        if (bone_track->location_y) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->location_y;
        }
        if (bone_track->location_z) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->prev_kf_idx = -1;
            target_track->curve = bone_track->location_z;
        }
        
    }
    
    return new_ctx;
    
}

/* Routine to destroy aimation context */
void pmdl_animation_destroy(pmdl_animation_ctx* ctx_ptr) {
    pspl_free_media_block(ctx_ptr);
}

/* Evaluate cubic bézier given T value */
static inline float2 evaluate_bezier(double t,
                                     const pmdl_curve_keyframe* left_kf,
                                     const pmdl_curve_keyframe* right_kf) {
    
    double t1m = 1.0-t;
    double t1m2 = t1m*t1m;
    double tsq = t*t;
    
    float a = (float)(t1m2*t1m);
    float b = (float)(3*t1m2*t);
    float c = (float)(3*t1m*tsq);
    float d = (float)(tsq*t);
    
#   if __has_extension(attribute_ext_vector_type)
    return (a*left_kf->main_handle) + (b*left_kf->right_handle) +
           (c*right_kf->left_handle) + (d*right_kf->main_handle);
#   else
    float2 result;
    result[0] = (a*left_kf->main_handle[0]) + (b*left_kf->right_handle[0]) +
                (c*right_kf->left_handle[0]) + (d*right_kf->main_handle[0]);
    result[1] = (a*left_kf->main_handle[1]) + (b*left_kf->right_handle[1]) +
                (c*right_kf->left_handle[1]) + (d*right_kf->main_handle[1]);
    return result;
#   endif
    
}

/* Solve *fcurve* bézier Y (value) for X (time) via adaptive precision */
#define SOLVE_ERROR 0.05
static inline float solve_bezier(double time,
                                 const pmdl_curve_keyframe* left_kf,
                                 const pmdl_curve_keyframe* right_kf) {
    
    unsigned err = 1;
    double epsilon = 0.5;
    double t = 0.5;
    float2 result;
    while (1) {
        result = evaluate_bezier(t, left_kf, right_kf);
        err = fabs(result[0] - time);
        if (err < SOLVE_ERROR)
            break;
        epsilon /=2;
        if (result[0] > time)
            t -= epsilon;
        else
            t += epsilon;
    }
    
    return result[1];
    
}

/* Routine to advance animation context */
void pmdl_animation_advance(pmdl_animation_ctx* ctx_ptr, float abs_time) {
    int i,j;
    
    // Calculate current time in animation
    double current_time;
    if (ctx_ptr->have_abs_time) {
        current_time = ((abs_time - ctx_ptr->previous_abs_time) * ctx_ptr->playback_rate) + ctx_ptr->current_time;
        ctx_ptr->previous_abs_time = abs_time;
    } else {
        ctx_ptr->have_abs_time = 1;
        current_time = 0.0;
        ctx_ptr->previous_abs_time = abs_time;
    }
    
    // Determine if animation has ended
    if (current_time > ctx_ptr->action->action_duration) {
        if (ctx_ptr->loop_flag) {
            if (ctx_ptr->anim_loop_point_cb)
                ctx_ptr->anim_loop_point_cb(ctx_ptr);
            current_time -= ctx_ptr->action->action_duration;
        } else {
            if (ctx_ptr->anim_end_cb)
                ctx_ptr->anim_end_cb(ctx_ptr);
            return;
        }
    }
    ctx_ptr->current_time = current_time;
    
    // Iterate curve instances and cache current animation values
    for (i=0 ; i<ctx_ptr->curve_instance_count ; ++i) {
        pmdl_curve_playback* curve_inst = &ctx_ptr->curve_instance_array[i];
        
        // If one keyframe, static pose
        if (curve_inst->curve->keyframe_count == 1) {
            curve_inst->cached_value = curve_inst->curve->keyframe_array[0].main_handle[1];
            continue;
        }
        
        // Find surrounding keyframes
        const pmdl_curve_keyframe* left_kf = NULL;
        const pmdl_curve_keyframe* right_kf = NULL;

        for (j=curve_inst->prev_kf_idx+1 ; j<curve_inst->curve->keyframe_count ; ++j) {
            
            right_kf = &curve_inst->curve->keyframe_array[j];
            if (j)
                left_kf = &curve_inst->curve->keyframe_array[j-1];
            else
                left_kf = right_kf;
            
            if (current_time >= right_kf->main_handle[0]) {
                if (j == curve_inst->curve->keyframe_count-1) {
                    left_kf = right_kf;
                    break;
                }
                continue;
            } else if (left_kf != right_kf && current_time < left_kf->main_handle[0]) {
                j-=2;
                continue;
            }
            curve_inst->prev_kf_idx = j-1;
            break;
            
        }
        
        // If outside keyframe bounds, simply cache extrema value
        if (left_kf == right_kf) {
            curve_inst->cached_value = left_kf->main_handle[1];
            continue;
        }
        
        // Otherwise, compute cubic polynomial
        curve_inst->cached_value = solve_bezier(current_time, left_kf, right_kf);
        
        
    }
    
}

/* Routine to rewind animation context (call advance afterwards before drawing) */
void pmdl_animation_rewind(pmdl_animation_ctx* ctx_ptr) {
    ctx_ptr->have_abs_time = 0;
    ctx_ptr->current_time = 0.0;
}

