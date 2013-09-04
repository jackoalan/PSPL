//
//  PMDLRuntimeRigging.c
//  PSPL
//
//  Created by Jack Andersen on 8/26/13.
//
//

#include <stdio.h>
#include <stdarg.h>
#include <alloca.h>
#include <math.h>
#include <PMDLRuntime.h>
#include <PMDLRuntimeRigging.h>
#include "PMDLRuntimeProcessing.h"

#pragma pack(1)
struct file_bone {
    uint32_t string_off;
    float bone_head[3];
    int32_t parent_index;
    uint32_t child_count;
    uint32_t child_array[];
} __attribute__((__packed__));
#pragma pack()

#pragma pack(1)
struct file_skin {
    uint32_t bone_count;
    int32_t bone_indices[];
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
    file_cur += sizeof(uint32_t)*bone_count;
    
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
    file_cur += sizeof(uint32_t)*skin_count;
    
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
    file_cur += sizeof(uint32_t)*action_count;
    
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
    
    // Allocate bone base vector block
    // (Separated for SIMD vector alignment reasons)
    pspl_vector4_t* base_vector_block =
    pspl_allocate_media_block(bone_count*sizeof(pspl_vector4_t));
    
    for (i=0 ; i<bone_count ; ++i) {
        
        struct file_bone* bone = (struct file_bone*)(bone_arr + bone_offsets[i]);
        pmdl_bone* target_bone = (pmdl_bone*)&rigging_ctx->bone_array[i];
        
        target_bone->bone_name = bone_string_table + bone->string_off;
        target_bone->bone_index = i;
        
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

        pspl_vector4_t* base_vector = &base_vector_block[i];
        base_vector->f[0] = bone->bone_head[0];
        base_vector->f[1] = bone->bone_head[1];
        base_vector->f[2] = bone->bone_head[2];
        target_bone->base_vector = base_vector;
        
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
                track->scale_x = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->scale_x->keyframe_count + sizeof(uint32_t);
                track->scale_y = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->scale_y->keyframe_count + sizeof(uint32_t);
                track->scale_z = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->scale_z->keyframe_count + sizeof(uint32_t);
            } else {
                track->scale_x = NULL;
                track->scale_y = NULL;
                track->scale_z = NULL;
            }
            
            if (ACTION_ROTATION(track_head)) {
                track->property_count += 4;
                track->rotation_w = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->rotation_w->keyframe_count + sizeof(uint32_t);
                track->rotation_x = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->rotation_x->keyframe_count + sizeof(uint32_t);
                track->rotation_y = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->rotation_y->keyframe_count + sizeof(uint32_t);
                track->rotation_z = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->rotation_z->keyframe_count + sizeof(uint32_t);
            } else {
                track->rotation_w = NULL;
                track->rotation_x = NULL;
                track->rotation_y = NULL;
                track->rotation_z = NULL;
            }
            
            if (ACTION_LOCATION(track_head)) {
                track->property_count += 3;
                track->location_x = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->location_x->keyframe_count + sizeof(uint32_t);
                track->location_y = (const pmdl_curve*)action;
                action += sizeof(pmdl_curve_keyframe)*track->location_y->keyframe_count + sizeof(uint32_t);
                track->location_z = (const pmdl_curve*)action;
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
void pmdl_rigging_destroy(pmdl_rigging_ctx* rig_ctx) {
    pspl_free_media_block((void*)rig_ctx->bone_array[0].base_vector);
    pspl_free_media_block(rig_ctx);
}



/* Routine to init action context */
pmdl_action_ctx* pmdl_action_init(const pmdl_action* action) {
    int i,j;
    if (!action)
        return NULL;
    
    // Determine size of context block
    size_t block_size = sizeof(pmdl_action_ctx);
    
    unsigned curve_instance_count = 0;
    for (i=0 ; i<action->bone_track_count; ++i) {
        const pmdl_action_bone_track* bone_track = &action->bone_track_array[i];
        curve_instance_count += bone_track->property_count;
    }
    
    block_size += sizeof(pmdl_curve_playback)*curve_instance_count;
    
    // Allocate context block
    void* context_block = pspl_allocate_media_block(block_size);
    pmdl_action_ctx* new_ctx = context_block;
    new_ctx->action = action;
    new_ctx->current_time = 0.0;
    new_ctx->curve_instance_count = curve_instance_count;
    new_ctx->curve_instance_array = context_block + sizeof(pmdl_action_ctx);
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
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->scale_x;
        }
        if (bone_track->scale_y) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->scale_y;
        }
        if (bone_track->scale_z) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->scale_z;
        }
        
        if (bone_track->rotation_w) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->rotation_w;
        }
        if (bone_track->rotation_x) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->rotation_x;
        }
        if (bone_track->rotation_y) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->rotation_y;
        }
        if (bone_track->rotation_z) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->rotation_z;
        }
        
        if (bone_track->location_x) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->location_x;
        }
        if (bone_track->location_y) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->location_y;
        }
        if (bone_track->location_z) {
            target_track = &new_ctx->curve_instance_array[j++];
            target_track->bone_index = bone_track->bone_index;
            target_track->curve = bone_track->location_z;
        }
        
    }
    
    
    return new_ctx;
    
}

/* Routine to destroy action context */
void pmdl_action_destroy(pmdl_action_ctx* ctx_ptr) {
    pspl_free_media_block(ctx_ptr);
}

/* Evaluate cubic bézier given T value */
static inline void evaluate_bezier(float2 result,
                                   double t,
                                   const pmdl_curve_keyframe* left_kf,
                                   const pmdl_curve_keyframe* right_kf) {
    
    double t1m = 1.0-t;
    double t1msq = t1m*t1m;
    double tsq = t*t;
    
    float a = (float)(t1msq*t1m);
    float b = (float)(3*t1msq*t);
    float c = (float)(3*t1m*tsq);
    float d = (float)(tsq*t);
    

    result[0] = (a*left_kf->main_handle[0]) + (b*left_kf->right_handle[0]) +
                (c*right_kf->left_handle[0]) + (d*right_kf->main_handle[0]);
    result[1] = (a*left_kf->main_handle[1]) + (b*left_kf->right_handle[1]) +
                (c*right_kf->left_handle[1]) + (d*right_kf->main_handle[1]);
    
}

/* Solve *fcurve* bézier Y (value) for X (time) by approaching 
 * acceptable limit error */
#define LIMIT_ERROR 0.01
static inline float solve_bezier(double time,
                                 const pmdl_curve_keyframe* left_kf,
                                 const pmdl_curve_keyframe* right_kf) {
    
    double err;
    double epsilon = 0.5;
    double t = 0.5;
    float2 result;
    for (;;) {
        evaluate_bezier(result, t, left_kf, right_kf);
        err = fabs(result[0] - time);
        if (err < LIMIT_ERROR)
            break;
        epsilon /= 2;
        if (result[0] > time)
            t -= epsilon;
        else
            t += epsilon;
    }
    
    return result[1];
    
}

/* Routine to recursively evaluate FK sub-chain */
static void recursive_fk_evaluate(pmdl_fk_playback* fk, unsigned action_count, char flip_bit) {
    int i = 0;
    int j;
    
    if (fk->eval_flip_bit != flip_bit) {
        fk->eval_flip_bit = flip_bit;
        if (fk->parent_fk)
            recursive_fk_evaluate(fk->parent_fk, action_count, flip_bit);
        
        
        // Clear blended values
        fk->is_animated = 0;
        
        fk->scale_blend.f[0] = 1.0;
        fk->scale_blend.f[1] = 1.0;
        fk->scale_blend.f[2] = 1.0;
    
        fk->rotation_blend.f[3] = 1.0;
        fk->rotation_blend.f[0] = 0.0;
        fk->rotation_blend.f[1] = 0.0;
        fk->rotation_blend.f[2] = 0.0;
    
        fk->location_blend.f[0] = 0.0;
        fk->location_blend.f[1] = 0.0;
        fk->location_blend.f[2] = 0.0;
        
        
        // Blend values together for animation composition
        for (j=0 ; j<action_count ; ++j) {
            const pmdl_fk_action_playback* action_playback = &fk->action_playback_array[j];
            
            if (action_playback->bone_anim_track) {
                fk->is_animated = 1;
                
                // Scale blend
                if (action_playback->bone_anim_track->scale_x)
                    fk->scale_blend.f[0] *= action_playback->first_curve_instance[i++].cached_value;
                if (action_playback->bone_anim_track->scale_y)
                    fk->scale_blend.f[1] *= action_playback->first_curve_instance[i++].cached_value;
                if (action_playback->bone_anim_track->scale_z)
                    fk->scale_blend.f[2] *= action_playback->first_curve_instance[i++].cached_value;
                
                // Rotation blend
                pspl_vector4_t rotation_quat = {.f[0]=0, .f[1]=0, .f[2]=0, .f[3]=1};
                if (action_playback->bone_anim_track->rotation_w)
                    rotation_quat.f[3] = action_playback->first_curve_instance[i++].cached_value;
                if (action_playback->bone_anim_track->rotation_x)
                    rotation_quat.f[0] = action_playback->first_curve_instance[i++].cached_value;
                if (action_playback->bone_anim_track->rotation_y)
                    rotation_quat.f[1] = action_playback->first_curve_instance[i++].cached_value;
                if (action_playback->bone_anim_track->rotation_z)
                    rotation_quat.f[2] = action_playback->first_curve_instance[i++].cached_value;
                pmdl_quat_mul(&fk->rotation_blend, &rotation_quat, &fk->rotation_blend);
                
                // Location blend
                if (action_playback->bone_anim_track->location_x)
                    fk->location_blend.f[0] += action_playback->first_curve_instance[i++].cached_value;
                if (action_playback->bone_anim_track->location_y)
                    fk->location_blend.f[1] += action_playback->first_curve_instance[i++].cached_value;
                if (action_playback->bone_anim_track->location_z)
                    fk->location_blend.f[2] += action_playback->first_curve_instance[i++].cached_value;

                
                
            }
            
        }
        
        // Convert blended values to transformation matrices
        if (fk->is_animated && fk->parent_fk) {
            // Animated child bone
            
            // Scale transform
            pspl_matrix34_t scale_matrix;
            pmdl_matrix34_cpy(fk->parent_fk->bone_matrix->v, scale_matrix.v);
            scale_matrix.m[0][0] *= fk->scale_blend.f[0];
            scale_matrix.m[1][1] *= fk->scale_blend.f[1];
            scale_matrix.m[2][2] *= fk->scale_blend.f[2];
            scale_matrix.m[0][3] = 0.0f;
            scale_matrix.m[1][3] = 0.0f;
            scale_matrix.m[2][3] = 0.0f;
            
            // Rotation transform
            pspl_matrix34_t rotation_location_matrix;
            pspl_vector4_t rotation_quat;
            rotation_quat.f[3] = fk->rotation_blend.f[3];
            rotation_quat.f[0] = fk->rotation_blend.f[0];
            rotation_quat.f[2] = fk->rotation_blend.f[1];
            rotation_quat.f[1] = -fk->rotation_blend.f[2];
            pmdl_matrix34_quat(&rotation_location_matrix, &rotation_quat);
            
            // Location transform
            pspl_vector4_t parent_base_vector;
            pmdl_vector4_sub(fk->bone->base_vector->v, fk->parent_fk->bone->base_vector->v, parent_base_vector.v);
            pmdl_vector3_matrix_mul(fk->parent_fk->bone_matrix,
                                    (pspl_vector3_t*)&parent_base_vector, (pspl_vector3_t*)&parent_base_vector);
            rotation_location_matrix.m[0][3] = parent_base_vector.f[0];
            rotation_location_matrix.m[1][3] = parent_base_vector.f[1];
            rotation_location_matrix.m[2][3] = parent_base_vector.f[2];
            rotation_location_matrix.m[0][3] += fk->location_blend.f[0];
            rotation_location_matrix.m[1][3] += fk->location_blend.f[1];
            rotation_location_matrix.m[2][3] += fk->location_blend.f[2];
            
            // Concatenate transforms
            pmdl_matrix34_mul(&scale_matrix, &rotation_location_matrix, fk->bone_matrix);
            
            
        } else if (fk->is_animated) {
            // Animated root bone
            
            // Scale transform
            pspl_matrix34_t scale_matrix;
            pmdl_matrix34_identity(&scale_matrix);
            scale_matrix.m[0][0] = fk->scale_blend.f[0];
            scale_matrix.m[1][1] = fk->scale_blend.f[1];
            scale_matrix.m[2][2] = fk->scale_blend.f[2];
            
            // Rotation transform
            pspl_matrix34_t rotation_location_matrix;
            pspl_vector4_t rotation_quat;
            rotation_quat.f[3] = fk->rotation_blend.f[3];
            rotation_quat.f[0] = fk->rotation_blend.f[0];
            rotation_quat.f[2] = fk->rotation_blend.f[1];
            rotation_quat.f[1] = -fk->rotation_blend.f[2];
            pmdl_matrix34_quat(&rotation_location_matrix, &rotation_quat);
            
            // Location transform
            rotation_location_matrix.m[0][3] = (fk->bone->base_vector)->f[0];
            rotation_location_matrix.m[1][3] = (fk->bone->base_vector)->f[1];
            rotation_location_matrix.m[2][3] = (fk->bone->base_vector)->f[2];
            rotation_location_matrix.m[0][3] += fk->location_blend.f[0];
            rotation_location_matrix.m[1][3] += fk->location_blend.f[1];
            rotation_location_matrix.m[2][3] += fk->location_blend.f[2];
            
            // Concatenate transforms
            pmdl_matrix34_mul(&scale_matrix, &rotation_location_matrix, fk->bone_matrix);
            
            
        } else if (fk->parent_fk) {
            // Non-animated child bone
            
            pmdl_matrix34_cpy(fk->parent_fk->bone_matrix->v, fk->bone_matrix->v);
            pspl_vector4_t parent_base_vector;
            pmdl_vector4_sub(fk->bone->base_vector->v, fk->parent_fk->bone->base_vector->v, parent_base_vector.v);
            pmdl_vector3_matrix_mul(fk->parent_fk->bone_matrix,
                                    (pspl_vector3_t*)&parent_base_vector, (pspl_vector3_t*)&parent_base_vector);
            fk->bone_matrix->m[0][3] = parent_base_vector.f[0];
            fk->bone_matrix->m[1][3] = parent_base_vector.f[1];
            fk->bone_matrix->m[2][3] = parent_base_vector.f[2];
            
            
        } else {
            // Non-animated root bone
            
            pmdl_matrix34_identity(fk->bone_matrix);
            fk->bone_matrix->m[0][3] = (fk->bone->base_vector)->f[0];
            fk->bone_matrix->m[1][3] = (fk->bone->base_vector)->f[1];
            fk->bone_matrix->m[2][3] = (fk->bone->base_vector)->f[2];
            
            
        }
        
    }
}

/* Routine to advance action context */
void pmdl_action_advance(pmdl_action_ctx* ctx_ptr, double time_delta) {
    int i,j;
    
    // Calculate current time in animation
    double current_time = ctx_ptr->current_time + time_delta;
    
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

        for (j=0 ; j<curve_inst->curve->keyframe_count ; ++j) {
            const pmdl_curve_keyframe* kf = &curve_inst->curve->keyframe_array[j];
            
            if (current_time > kf->main_handle[0])
                left_kf = kf;
            else {
                right_kf = kf;
                break;
            }
            
        }
        if (!right_kf)
            right_kf = &curve_inst->curve->keyframe_array[curve_inst->curve->keyframe_count - 1];
        if (!left_kf)
            left_kf = right_kf;
        
        // If outside keyframe bounds, simply cache extrema value
        if (left_kf == right_kf) {
            curve_inst->cached_value = left_kf->main_handle[1];
            continue;
        }
        
        // Otherwise, solve cubic polynomial
        curve_inst->cached_value = solve_bezier(current_time, left_kf, right_kf);
        
    }
    
}


/* Routine to init animation context */
pmdl_animation_ctx* pmdl_animation_init(unsigned action_ctx_count,
                                        const pmdl_action_ctx** action_ctx_array) {
    int i,j,k,l;
    if (!action_ctx_count)
        return NULL;
    
    // Ensure all action contexts have been initialised from the same rigging context
    for (i=1 ; i<action_ctx_count ; ++i) {
        if (action_ctx_array[i]->action->parent_ctx != action_ctx_array[0]->action->parent_ctx)
            return NULL;
    }
    
    // Common rigging context
    const pmdl_rigging_ctx* parent_ctx = action_ctx_array[0]->action->parent_ctx;
    
    // Allocate context block
    size_t block_size = sizeof(pmdl_animation_ctx);
    block_size += sizeof(pmdl_action_ctx*)*action_ctx_count;
    block_size += sizeof(pmdl_fk_playback)*parent_ctx->bone_count;
    block_size += sizeof(pmdl_fk_action_playback)*action_ctx_count*parent_ctx->bone_count;
    void* context_block = pspl_allocate_media_block(block_size);
    pmdl_animation_ctx* new_ctx = context_block;
    
    // Populate main members
    new_ctx->parent_ctx = parent_ctx;
    new_ctx->action_ctx_count = action_ctx_count;
    new_ctx->action_ctx_array = context_block + sizeof(pmdl_animation_ctx);
    for (i=0 ; i<action_ctx_count ; ++i)
        new_ctx->action_ctx_array[i] = action_ctx_array[i];
    new_ctx->fk_instance_count = parent_ctx->bone_count;
    new_ctx->fk_instance_array = context_block + sizeof(pmdl_animation_ctx) +
    sizeof(pmdl_action_ctx*)*action_ctx_count;
    new_ctx->master_fk_flip_bit = 0;
    
    // Allocate bone matrix array
    // (separate block on heap due to SIMD vector alignment requirements)
    pspl_matrix34_t* matrix_array_block =
    pspl_allocate_media_block(parent_ctx->bone_count*sizeof(pspl_matrix34_t));
    
    // Populate FK instance array
    void* cur_action_playback = context_block + sizeof(pmdl_animation_ctx) +
    sizeof(pmdl_action_ctx*)*action_ctx_count + sizeof(pmdl_fk_playback)*parent_ctx->bone_count;
    for (i=0 ; i<parent_ctx->bone_count ; ++i) {
        
        const pmdl_bone* bone = &parent_ctx->bone_array[i];
        pmdl_fk_playback* target_fk = &new_ctx->fk_instance_array[i];
        
        target_fk->bone = bone;
        
        target_fk->action_playback_array = cur_action_playback;
        for (l=0 ; l<action_ctx_count ; ++l) {
            const pmdl_action_ctx* action_ctx = action_ctx_array[l];
            pmdl_fk_action_playback* target_fk_action = (pmdl_fk_action_playback*)&target_fk->action_playback_array[l];
            
            target_fk_action->bone_anim_track = NULL;
            target_fk_action->first_curve_instance = NULL;
            for (j=0 ; j<action_ctx->action->bone_track_count ; ++j) {
                const pmdl_action_bone_track* bone_track = &action_ctx->action->bone_track_array[j];
                if (bone_track->bone_index == i) {
                    target_fk_action->bone_anim_track = bone_track;
                    for (k=0 ; k<action_ctx->curve_instance_count ; ++k) {
                        pmdl_curve_playback* curve_pb = &action_ctx->curve_instance_array[k];
                        if (curve_pb->bone_index == i) {
                            target_fk_action->first_curve_instance = curve_pb;
                            break;
                        }
                    }
                    break;
                }
            }
            
        }
        cur_action_playback += sizeof(pmdl_fk_action_playback)*action_ctx_count;
        
        if (bone->parent)
            target_fk->parent_fk = &new_ctx->fk_instance_array[bone->parent->bone_index];
        else
            target_fk->parent_fk = NULL;
        
        
        target_fk->bone_matrix = &matrix_array_block[i];
        pmdl_matrix34_identity(target_fk->bone_matrix);
        target_fk->bone_matrix->m[0][3] = (bone->base_vector)->f[0];
        target_fk->bone_matrix->m[1][3] = (bone->base_vector)->f[1];
        target_fk->bone_matrix->m[2][3] = (bone->base_vector)->f[2];
        
        target_fk->eval_flip_bit = 0;
        
    }
    
    return new_ctx;
    
}

/* Variadic version of the array initialiser above (please NULL terminate) */
pmdl_animation_ctx* pmdl_animation_initv(const pmdl_action_ctx* first_action_ctx, ...) {
    int i;
    
    if (!first_action_ctx)
        return NULL;
    
    va_list va;
    va_start(va, first_action_ctx);
    unsigned count = 1;
    while (va_arg(va, const pmdl_action_ctx*))
        ++count;
    va_end(va);
    
    const pmdl_action_ctx** ctx_arr = alloca(sizeof(pmdl_action_ctx*)*count);
    ctx_arr[0] = first_action_ctx;
    va_start(va, first_action_ctx);
    for (i=1 ; i<count ; ++i)
        ctx_arr[i] = va_arg(va, const pmdl_action_ctx*);
    va_end(va);
    
    return pmdl_animation_init(count, ctx_arr);
    
}

/* Composites and evaluates bone transformation matrices from contained action contexts
 * Call *before* drawing a rigged model */
void pmdl_animation_evaluate(pmdl_animation_ctx* ctx_ptr) {
    int i;
    
    // Iterate bone structure and perform FK transformations
    ctx_ptr->master_fk_flip_bit ^= 1;
    for (i=0 ; i<ctx_ptr->fk_instance_count ; ++i) {
        pmdl_fk_playback* fk = &ctx_ptr->fk_instance_array[i];
        if (fk->bone)
            recursive_fk_evaluate(fk, ctx_ptr->action_ctx_count, ctx_ptr->master_fk_flip_bit);
    }
    
}


/* Routine to destroy animation context */
void pmdl_animation_destroy(pmdl_animation_ctx* ctx_ptr) {
    pspl_free_media_block(ctx_ptr->fk_instance_array[0].bone_matrix);
    pspl_free_media_block(ctx_ptr);
}

