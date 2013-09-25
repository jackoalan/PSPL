'''
PSPL Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file re-indexes UV generators for runtime-compatible materials
with valid texture slots.
'''


# Blend types supported by PSPL
pspl_compatible_blend_types = ['MIX', 'MULTIPLY', 'ADD', 'SUBTRACT']

# UV generator types types supported by PSPL
pspl_compatible_uvgen_types = ['ORCO', 'UV', 'NORMAL']

# Filter compatible texture slots
def pspl_filter_texture_slots(material):
    tex_slots = []
    for i in range(len(material.texture_slots)):
        slot = material.texture_slots[i]
        
        if slot is None:
            continue
        
        # Ensure PSPL compatibility
        if not slot.use:
            continue
        if not (slot.use_map_color_diffuse or slot.use_map_alpha):
            continue
        if slot.blend_type not in pspl_compatible_blend_types:
            continue
        if slot.texture_coords not in pspl_compatible_uvgen_types:
            continue
        if slot.texture.type != 'IMAGE':
            continue
        
        tex_slots.append(i)
    
    return tex_slots


# Determine PSPL UV index
def pspl_uv_index(material, tex_name):
    
    tex_slots = pspl_filter_texture_slots(material)
    
    this_tex_slot_idx = material.texture_slots.find(tex_name)
    if this_tex_slot_idx >= 0:
        if this_tex_slot_idx not in tex_slots:
            return -1
    else:
        return -1
    this_tex_slot = material.texture_slots[this_tex_slot_idx]
    
    return_uv_idx = 0
    for tex_slot_idx in tex_slots:
        tex_slot = material.texture_slots[tex_slot_idx]
        
        if tex_slot.texture.pspl_uv_share_texture and tex_slot.texture.pspl_uv_share_texture != tex_slot.name:
            if tex_slot == this_tex_slot:
                child_slot = pspl_uv_index(material, tex_slot.texture.pspl_uv_share_texture)
                if child_slot >= 0:
                    return child_slot
            else:
                continue
        
        if tex_slot == this_tex_slot:
            return return_uv_idx
        
        return_uv_idx += 1
    
    return -1
