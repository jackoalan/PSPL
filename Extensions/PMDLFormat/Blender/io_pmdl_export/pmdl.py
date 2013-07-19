'''
PMDL Export Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file defines the main `pmdl` class used to generate PMDL file instances.
The class is initialised with a MESH or ARMATURE object (ARMATUREs generate `PAR1`).

The provided MESH or ARMATURE may hierarcically contain other MESH objects, 
resulting in an appropriately sub-divided PMDL. The MATERIAL name assigned to
MESH objects is hashed with SHA1 and used to reference a PSPLC implementing 
the appropriate fragment shader.

Once the object is assigned, the appropriate draw-generator is initialised
(with optional indexing parameters set) and loaded into the PMDL instance. 

The actual PMDL generation may then happen with these objects gathered together.
'''

from . import pmdl_par2_octree
from . import pmdl_par1_rigging

import struct
import bpy
import hashlib

def add_vec3(a,b):
    return (a[0]+b[0],a[1]+b[1],a[2]+b[2])

# Round up to nearest 32 multiple
def ROUND_UP_32(num):
    if num%32:
        return ((num>>5)<<5)+32
    else:
        return num
        

class pmdl:
    
    # Expand master bounding box to fit added mesh object
    def _accumulate_bound_box(self, obj):
    
        min_box = obj.bound_box[0]
        max_box = obj.bound_box[6]
        if not self.bound_box_accounted:
            self.bound_box_accounted = True
            for i in range(3):
                self.bound_box_min[i] = min_box[i]
                self.bound_box_max[i] = max_box[i]
        else:
            for i in range(3):
                if self.bound_box_min[i] > min_box[i]:
                    self.bound_box_min[i] = min_box[i]
                if self.bound_box_max[i] < max_box[i]:
                    self.bound_box_max[i] = max_box[i]

    
    # Recursive routine to add all MESH objects as individual PMDL collections
    def _recursive_add_mesh(self, draw_gen, object, root_rel_loc):
        
        # Start with root object (make sure it's a MESH)
        if object.type == 'MESH':
            
            # Only add if there is at least *one* polygon in mesh
            if len(object.data.polygons):
                
                # Copy mesh
                copy_name = object.name + "_pmdltri"
                copy_mesh = bpy.data.meshes.new(copy_name)
                copy_obj = bpy.data.objects.new(copy_name, copy_mesh)
                copy_obj.data = object.data.copy()
                copy_obj.scale = object.scale
                copy_obj.location = root_rel_loc # This is set to be root-object-relative
                bpy.context.scene.objects.link(copy_obj)
                
                # Triangulate mesh
                bpy.context.scene.objects.active = copy_obj
                bpy.ops.object.mode_set(mode='EDIT')
                bpy.ops.mesh.select_all(action='SELECT')
                bpy.ops.mesh.quads_convert_to_tris()
                bpy.ops.object.mode_set(mode='OBJECT')
                
                # Set mesh's coordinates to be root-relative
                for vert in copy_mesh.vertices:
                    for i in range(3):
                        vert.co[i] += root_rel_loc[i]
    
                # Add mesh to draw generator
                draw_gen.add_mesh(self, copy_obj)

                # Delete copied mesh from scene (and add to set to be deleted later)
                bpy.context.scene.objects.unlink(copy_obj)
                self.all_objs.add(copy_obj)
                self.all_meshes.add(copy_obj.data)
                self.all_meshes.add(copy_mesh)
    
                # Account for mesh bounding box
                self._accumulate_bound_box(object)
    

        # Recursively incorporate child meshes
        for child in object.children:
            if child.type == 'MESH':
                self._recursive_add_mesh(draw_gen, child, add_vec3(root_rel_loc, child.location))

    
    # PMDL Constructor, given a blender object for initialisation
    def __init__(self, object, draw_gen):
        
        self.object = object
        
        # Set of *all* included objects and their meshes
        self.all_objs = set()
        self.all_meshes = set()
        
        # Array of shader PSPLC hashes
        self.shader_hashes = []
        
        # Array of bone names
        self.bone_names = []
        
        # Bound box of all meshes
        self.bound_box_accounted = False
        self.bound_box_min = [0,0,0]
        self.bound_box_max = [0,0,0]
        
        self.sub_type = 'PAR0'
        if object.type == 'ARMATURE':
            # Create a PAR1 file if given an ARMATURE
            self.sub_type = 'PAR1'
            self.rigging = pmdl_par1_rigging.pmdl_par1_rigging(self)
        
        
        # Break down blender object into collections
        self.collections = set()
        self._recursive_add_mesh(draw_gen, object, (0,0,0))


    # Get shader index
    def get_shader_index(self, shader_name):
        name_hash = hashlib.sha1(shader_name.encode('utf-8')).digest()
        for i in range(len(self.shader_hashes)):
            if name_hash == exist_hash[i]:
                return i
        self.shader_hashes.append(name_hash)
        return len(self.shader_hashes) - 1

    # Generate hash ref table
    def gen_shader_refs(self):
        table = bytearray()
        for hash in self.shader_hashes:
            table += hash
        return table


    # Get bone offset
    def get_bone_offset(self, new_name):
        offset = 0
        for bone_name in self.bone_names:
            if bone_name == new_name:
                return offset
            offset += len(bone_name) + 1
        self.bone_names.append(new_name)
        return offset


    # Generate bone string table
    def gen_bone_table(self):
        table = bytearray()
        for bone in self.bone_names:
            table += bone
            table.append(0)
        return table


    # When initialised with a MESH hierarchy,
    # a PMDL may be made as a `PAR2` when this is called.
    # `levels` is the count of non-leaf octree levels that will be produced;
    # it must be at least 1.
    def add_octree(self, levels):
        if levels < 1:
            return "Unable to make PAR2; there must be at least 1 octree level requested"

        if self.sub_type == 'PAR1':
            return "Unable to make PAR2; the PMDL was initialised as a `PAR1` (ARMATURE)"

        # Set self type
        self.sub_type = 'PAR2'
        
        # Add octree and perform subdivision
        self.octree = pmdl_par2_octree.pmdl_par2_octree(self)


        return None


    # Finally, generate complete collection buffer
    def generate_collection_buffer(self, draw_generator, endianness, psize):
        
        endian_char = None
        if endianness == 'LITTLE':
            endian_char = '<'
        elif endianness == 'BIG':
            endian_char = '>'
        
        # Bytearray for collection bytes
        collection_bytes = bytearray()
        
        # Arrays to hold bytes objects for generated collection buffers
        collection_header_buffers = []
        collection_vertex_buffers = []
        collection_element_buffers = []
        collection_index_buffers = []
        
        # Begin generating individual collection buffers
        for i in range(len(draw_generator.collections)):
            header = bytearray()
            
            # Generate platform-specific portion of vertex buffer
            uv_count, max_bone_count, vert_bytes = draw_generator.generate_vertex_buffer(i, endian_char, psize)
            collection_vertex_buffers.append(vert_bytes)
            
            # Generate platform-specific portion of element buffer
            primitive_meshes, element_bytes = draw_generator.generate_element_buffer(i, endian_char, psize)
            collection_element_buffers.append(element_bytes)
            
            idx_buf = bytearray()
            idx_buf += struct.pack(endian_char + 'I', len(primitive_meshes))
            for mesh_primitives in primitive_meshes:
                for comp in mesh_primitives['mesh'].bound_box[0]:
                    idx_buf += struct.pack(endian_char + 'f', comp)
                for comp in mesh_primitives['mesh'].bound_box[6]:
                    idx_buf += struct.pack(endian_char + 'f', comp)
                if len(mesh_primitives['mesh'].data.materials):
                    shader_idx = self.get_shader_index(mesh_primitives['mesh'].data.materials[0].name)
                else:
                    shader_idx = -1
                idx_buf += struct.pack(endian_char + 'i', shader_idx)
            
            # Generate platform-specific portion of index buffer
            idx_buf += draw_generator.generate_index_buffer(primitive_meshes, endian_char, psize)
            
            collection_index_buffers.append(idx_buf)
            
            header += struct.pack(endian_char + 'H', uv_count)
            header += struct.pack(endian_char + 'H', max_bone_count)
            collection_header_buffers.append(header)
        
        
        # Add together header and buffer sizes to get offsets
        headers_len = len(collection_header_buffers) * 24
        headers_len_round = ROUND_UP_32(headers_len)
        headers_len_pad = headers_len_round - headers_len
        
        # Vertex buffer offsets
        cur_buf_offset = headers_len_round
        for i in range(len(collection_header_buffers)):
            collection_header_buffers[i] += struct.pack(endian_char + 'I', cur_buf_offset)
            collection_header_buffers[i] += struct.pack(endian_char + 'I', len(collection_vertex_buffers[i]))
            cur_buf_offset += len(collection_vertex_buffers[i])
        
        # Element buffer offsets
        for i in range(len(collection_header_buffers)):
            collection_header_buffers[i] += struct.pack(endian_char + 'I', cur_buf_offset)
            collection_header_buffers[i] += struct.pack(endian_char + 'I', len(collection_element_buffers[i]))
            cur_buf_offset += len(collection_element_buffers[i])
        
        # Index buffer offsets
        vert_end_round = ROUND_UP_32(cur_buf_offset)
        vert_end_pad = vert_end_round - cur_buf_offset
        cur_buf_offset = vert_end_round
        
        for i in range(len(collection_header_buffers)):
            collection_header_buffers[i] += struct.pack(endian_char + 'I', cur_buf_offset)
            cur_buf_offset += len(collection_index_buffers[i])
        
        
        # Begin generating master buffer
        for header in collection_header_buffers:
            collection_bytes += header
        for i in range(headers_len_pad):
            collection_bytes.append(0)
        
        for vert_buf in collection_vertex_buffers:
            collection_bytes += vert_buf
        for elem_buf in collection_element_buffers:
            collection_bytes += elem_buf
        for i in range(vert_end_pad):
            collection_bytes.append(0)
        
        for idx_buf in collection_index_buffers:
            collection_bytes += idx_buf
        
        
        # Add padding 0s
        buf_end_len = len(collection_bytes)
        buf_end_round = ROUND_UP_32(buf_end_len)
        buf_end_pad = buf_end_round - buf_end_len
        for i in range(buf_end_pad):
            collection_bytes.append(0)
        
        
        # Done
        return collection_bytes



    # This routine will generate a PMDL file with the requested draw generator
    # and endianness ['LITTLE', 'BIG'] at the requested path
    def generate_file(self, draw_generator, endianness, psize, file_path):


        # First, calculate various offsets into PMDL file
        header_size = 64
        rigging_info_buffer = bytes()
        octree_buffer = bytes()
        
        collection_offset = header_size + len(rigging_info_buffer) + len(octree_buffer)
        collection_buffer = self.generate_collection_buffer(draw_generator, endianness, psize)
        
        shader_refs_offset = collection_offset + len(collection_buffer)
        shader_refs_buffer = self.gen_shader_refs()
        
        bone_names_offset = shader_refs_offset + len(shader_refs_buffer)
        bone_names_buffer = self.gen_bone_table()
        
        total_size = bone_names_offset + len(bone_names_buffer)
        total_size_round = ROUND_UP_32(total_size)
        total_size_pad = total_size_round - total_size


        # Open file and write in header
        pmdl_file = open(file_path, 'wb')
        
        pmdl_header = bytearray()
        
        pmdl_header += b'PMDL'
        
        endian_char = None
        if endianness == 'LITTLE':
            pmdl_header += b'_LIT'
            endian_char = '<'
        elif endianness == 'BIG':
            pmdl_header += b'_BIG'
            endian_char = '>'

        pmdl_header += struct.pack(endian_char + 'I', psize)

        pmdl_header += self.sub_type.encode('utf-8')

        pmdl_header += draw_generator.file_identifier.encode('utf-8')

        for comp in self.bound_box_min:
            pmdl_header += struct.pack(endian_char + 'f', comp)
        for comp in self.bound_box_max:
            pmdl_header += struct.pack(endian_char + 'f', comp)
        
        pmdl_header += struct.pack(endian_char + 'I', collection_offset)
        pmdl_header += struct.pack(endian_char + 'I', len(draw_generator.collections))

        pmdl_header += struct.pack(endian_char + 'I', shader_refs_offset)

        pmdl_header += struct.pack(endian_char + 'I', bone_names_offset)

        for i in range(4):
            pmdl_header.append(0)

        pmdl_file.write(pmdl_header)


        # Now write sub-buffers
        pmdl_file.write(rigging_info_buffer)
        pmdl_file.write(octree_buffer)
        pmdl_file.write(collection_buffer)
        pmdl_file.write(shader_refs_buffer)
        pmdl_file.write(bone_names_buffer)
        for i in range(total_size_pad):
            pmdl_file.write(b'\xff')




    # Delete copied meshes from blender data
    def __del__(self):
        for obj in self.all_objs:
            bpy.data.objects.remove(obj)
        for mesh in self.all_meshes:
            bpy.data.meshes.remove(mesh)

