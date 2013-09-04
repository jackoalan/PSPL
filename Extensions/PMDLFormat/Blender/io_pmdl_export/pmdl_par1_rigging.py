'''
PMDL Export Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file defines the `pmdl_par1_rigging` class to iteratively construct
a Skinning Info Section for `PAR1` PMDL files. Used by draw-format 
generators to select an optimal skin entry for a draw primitive,
or have a new one established.
'''

import struct
import re
import bpy

# Round up to nearest 32 multiple
def ROUND_UP_32(num):
    if num%32:
        return ((num>>5)<<5)+32
    else:
        return num

# Regex RNA path matchers
scale_matcher = re.compile(r'pose.bones\["(\S+)"\].scale')
rotation_matcher = re.compile(r'pose.bones\["(\S+)"\].rotation_quaternion')
location_matcher = re.compile(r'pose.bones\["(\S+)"\].location')

# Appends an array of fcurve keyframes to byte stream
def add_fcurve_to_bytestream(bytes, endian_char, fcurve):
    scene_fps = bpy.context.scene.render.fps
    count = 0
    if fcurve:
        count = len(fcurve.keyframe_points)
    bytes += struct.pack(endian_char + 'I', count)
    if fcurve:
        for keyframe in fcurve.keyframe_points:
            bytes += struct.pack(endian_char + 'ffffff',
                                 (keyframe.handle_left[0]/scene_fps),
                                 keyframe.handle_left[1],
                                 (keyframe.co[0]/scene_fps),
                                 keyframe.co[1],
                                 (keyframe.handle_right[0]/scene_fps),
                                 keyframe.handle_right[1])

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
        
        # Identity blend value (remainder blend factor)
        identity_blend = 1.0
        
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
                identity_blend -= group_elem.weight
        
            else:

                # Record bone weight
                weight_array[bone_array.index(vertex_group.name)] = group_elem.weight
                identity_blend -= group_elem.weight
            

        # If we get here, no overflows; augment bone array and return weight array
        bone_array.extend(new_bones)
        return_weights = [identity_blend]
        return_weights.extend(weight_array)
        #print(return_weights)
        return return_weights


    # Generate Skeleton Info structure
    def generate_skeleton_info(self, pmdl, endian_char, psize):
        info_bytes = bytearray()
        
        bones = []
        for bone in self.armature.data.bones:
            bone_bytes = bytearray()
            
            bone_off = pmdl.get_bone_offset(bone.name, psize)
            bone_bytes += struct.pack(endian_char + 'I', bone_off)
            
            for comp in bone.head_local:
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
                bone_idx = self.armature.data.bones.find(bone)
                skin_bytes += struct.pack(endian_char + 'i', bone_idx)
            skin_entries.append(skin_bytes)
        
        info_bytes += struct.pack(endian_char + 'I', len(skin_entries))
        
        cur_offset = 0
        for entry in skin_entries:
            info_bytes += struct.pack(endian_char + 'I', cur_offset)
            cur_offset += len(entry)

        for entry in skin_entries:
            info_bytes += entry
    
        section_length = len(info_bytes) + 4
        return (struct.pack(endian_char + 'I', section_length) + info_bytes)


    # Generate animation info
    def generate_animation_info(self, pmdl, endian_char, psize):
        info_bytes = bytearray()
        anim_data = self.armature.animation_data

        # Build set of all actions contained within armature
        added_actions = set()
        
        if anim_data.action:
            added_actions.add(anim_data.action)
        
        for nla_track in anim_data.nla_tracks:
            for nla_strip in nla_track.strips:
                action = nla_strip.action
                added_actions.add(action)
    
        # Build animation data set
        action_bytes = []
        for action in added_actions:
        
            # Set of unique bone names
            bone_set = set()
            
            # Scan through all fcurves to build animated bone set
            for fcurve in action.fcurves:
                data_path = fcurve.data_path
                scale_match = scale_matcher.match(data_path)
                rotation_match = rotation_matcher.match(data_path)
                location_match = location_matcher.match(data_path)

                if scale_match:
                    bone_set.add(scale_match.group(1))
                elif rotation_match:
                    bone_set.add(rotation_match.group(1))
                elif location_match:
                    bone_set.add(location_match.group(1))


            # Relate fcurves per-bone and assemble data
            keyframe_stream = bytearray()
            keyframe_stream += struct.pack(endian_char + 'I', len(bone_set))
            duration = action.frame_range[1] / bpy.context.scene.render.fps
            keyframe_stream += struct.pack(endian_char + 'f', duration)
            for bone in bone_set:
                
                property_bits = 0
                
                # Scale curves
                x_scale_curve = None
                y_scale_curve = None
                z_scale_curve = None
                for fcurve in action.fcurves:
                    if fcurve.data_path == 'pose.bones["'+bone+'"].scale':
                        property_bits |= 1
                        if fcurve.array_index == 0:
                            x_scale_curve = fcurve
                        elif fcurve.array_index == 1:
                            y_scale_curve = fcurve
                        elif fcurve.array_index == 2:
                            z_scale_curve = fcurve


                # Rotation curves
                w_rot_curve = None
                x_rot_curve = None
                y_rot_curve = None
                z_rot_curve = None
                for fcurve in action.fcurves:
                    if fcurve.data_path == 'pose.bones["'+bone+'"].rotation_quaternion':
                        property_bits |= 2
                        if fcurve.array_index == 0:
                            w_rot_curve = fcurve
                        elif fcurve.array_index == 1:
                            x_rot_curve = fcurve
                        elif fcurve.array_index == 2:
                            y_rot_curve = fcurve
                        elif fcurve.array_index == 3:
                            z_rot_curve = fcurve


                # Location curves
                x_loc_curve = None
                y_loc_curve = None
                z_loc_curve = None
                for fcurve in action.fcurves:
                    if fcurve.data_path == 'pose.bones["'+bone+'"].location':
                        property_bits |= 4
                        if fcurve.array_index == 0:
                            x_loc_curve = fcurve
                        elif fcurve.array_index == 1:
                            y_loc_curve = fcurve
                        elif fcurve.array_index == 2:
                            z_loc_curve = fcurve


                # Assemble curves into data stream
                bone_idx = self.armature.data.bones.find(bone)
                bone_idx |= (property_bits << 29)
                keyframe_stream += struct.pack(endian_char + 'I', bone_idx)

                # Scale keyframes
                if property_bits & 1:
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, x_scale_curve)
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, y_scale_curve)
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, z_scale_curve)

                # Rotation keyframes
                if property_bits & 2:
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, w_rot_curve)
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, x_rot_curve)
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, y_rot_curve)
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, z_rot_curve)

                # Location keyframes
                if property_bits & 4:
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, x_loc_curve)
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, y_loc_curve)
                    add_fcurve_to_bytestream(keyframe_stream, endian_char, z_loc_curve)


            # Save keyframe stream
            action_bytes.append(keyframe_stream)

        all_action_bytes = bytearray()
        for action in action_bytes:
            all_action_bytes += action

        # Generate action offset table
        action_idx = 0
        action_off = 0
        action_table = bytearray()
        for action in added_actions:
            action_data = action_bytes[action_idx]
            action_table += struct.pack(endian_char + 'I', action_off)
            action_off += len(action_data)
            action_idx += 1


        # Generate action string table
        string_table_offsets = bytearray()
        string_table = bytearray()
        for action in added_actions:
            string_table_offsets += struct.pack(endian_char + 'I', len(string_table))
            string_table += action.name.encode('utf-8')
            string_table.append(0)


        # Generate final bytes
        info_bytes += struct.pack(endian_char + 'I', len(added_actions))
        info_bytes += struct.pack(endian_char + 'I', 8 + len(action_table) + len(all_action_bytes))
        info_bytes += action_table
        info_bytes += all_action_bytes
        info_bytes += string_table_offsets
        info_bytes += string_table

        return info_bytes



