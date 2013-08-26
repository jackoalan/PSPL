'''
PMDL Export Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file defines the `pmdl_par1_rigging` class to iteratively construct
a Skinning Info Section for `PAR1` PMDL files. Used by draw-format 
generators to select an optimal skin entry for a draw primitive,
or have a new one established.
'''

import struct

class pmdl_par1_rigging:

    # Set up with PMDL containing ARMATURE `object`
    def __init__(self, max_bone_count, armature):
        self.armature = armature
        self.max_bone_count = max_bone_count
        self.mesh_vertex_groups = None
    
        self.bone_arrays = []


    # Augment bone array with loop vert and return weight array
    # Returns 'None' if bone overflow
    def augment_bone_array_with_lv(self, mesh_obj, bone_array, loop_vert):
        vertex = mesh_obj.data.vertices[loop_vert[0].loop.vertex_index]
        
        # Loop-vert weight array
        weight_array = []
        for i in range(len(bone_array)):
            weight_array.append(0.0)
        
        # Tentative bone additions
        new_bones = []
        
        # Determine which bones (vertex groups) belong to loop_vert
        for group_elem in vertex.groups:
            vertex_group = self.mesh_vertex_groups[group_elem.group]

            if vertex_group.name not in bone_array:
                
                # Detect bone overflow
                if len(bone_array) + len(new_bones) >= self.max_bone_count:
                    return None

                # Add to array otherwise
                new_bones.append(vertex_group.name)
        
                # Record bone weight
                weight_array.append(group_elem.weight)
        
            else:

                # Record bone weight
                weight_array[bone_array.index(vertex_group.name)] = group_elem.weight
            

        # If we get here, no overflows; augment bone array and return weight array
        bone_array.extend(new_bones)
        return weight_array


    # Generate Skeleton Info structure
    def generate_skeleton_info(self, pmdl, endian_char, psize):
        info_bytes = bytearray()
        
        bones = []
        for bone in self.armature.data.bones:
            bone_bytes = bytearray()
            
            bone_off = pmdl.get_bone_offset(bone.name, psize)
            bone_bytes += struct.pack(endian_char + 'I', bone_off)
            
            for comp in bone.tail_local:
                bone_bytes += struct.pack(endian_char + 'f', comp)

            parent_idx = -1
            if bone.parent:
                parent_idx = self.armature.data.bones.find(bone.parent.name)
            bone_bytes += struct.pack(endian_char + 'i', parent_idx)

            bone_bytes += struct.pack(endian_char + 'I', len(bone.children))

            for child in bone.children:
                child_idx = self.armature.data.bones.find(child.name)
                bone_bytes += struct.pack(endian_char + 'I', child_idx)
                    
            bones.append(bone_bytes)

        info_bytes += struct.pack(endian_char + 'I', len(bones))
        
        cur_offset = 0
        for bone in bones:
            info_bytes += struct.pack(endian_char + 'I', cur_offset)
            cur_offset += len(bone)
                
        for bone in bones:
            info_bytes += bone
                
        section_length = len(info_bytes) + 4
        return (struct.pack(endian_char + 'I', section_length) + info_bytes)


    # Augment triangle-strip bone array to rigging info
    def augment_skin(self, bone_array):
        if bone_array not in self.bone_arrays:
            self.bone_arrays.append(bone_array)
            return (len(self.bone_arrays)-1)
        return self.bone_arrays.index(bone_array)


    # Generate Rigging Info structure
    def generate_rigging_info(self, pmdl, endian_char, psize):
        info_bytes = bytearray()
        
        skin_entries = []
        for bone_array in self.bone_arrays:
            skin_bytes = bytearray()
            skin_bytes += struct.pack(endian_char + 'I', len(bone_array))
            for bone in bone_array:
                bone_off = pmdl.get_bone_offset(bone, psize)
                skin_bytes += struct.pack(endian_char + 'I', bone_off)
            skin_entries.append(skin_bytes)
        
        info_bytes += struct.pack(endian_char + 'I', len(skin_entries))
        
        cur_offset = 0
        for entry in skin_entries:
            info_bytes += struct.pack(endian_char + 'I', cur_offset)
            cur_offset += len(entry)

        for entry in skin_entries:
            info_bytes += entry

        return info_bytes


