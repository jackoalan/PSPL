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

class pmdl:
    
    # Recursive routine to add all MESH objects as individual PMDL collections
    def _recursive_add_mesh(self, draw_gen, object):
        
        # Start with root object
        if object.type == 'MESH':
            if len(object.data.polygons):
                # Only add if there is at least *one* polygon in mesh
                
                # Copy mesh
                copy_name = object.name + "_tri"
                copy_mesh = bpy.data.meshes.new(copy_name)
                copy_obj = bpy.data.objects.new(copy_name, copy_mesh)
                copy_obj.data = object.data.copy()
                copy_obj.scale = object.scale
                copy_obj.location = object.location
                bpy.context.scene.objects.link(copy_obj)
                
                # Triangulate mesh
                copy_obj.select = True
                bpy.ops.object.mode_set(mode='EDIT')
                bpy.ops.mesh.select_all(action='SELECT')
                bpy.ops.mesh.quads_convert_to_tris()
                bpy.ops.object.mode_set(mode='OBJECT')
    
                # Add mesh to draw generator
                draw_gen.add_mesh(copy_obj)

                # Delete copied mesh
                bpy.context.scene.objects.unlink(copy_obj)
                bpy.data.objects.remove(copy_obj)
                bpy.data.meshes.remove(copy_mesh)
                

        # Recursively incorporate child meshes
        for child in object.children:
            if child.type == 'MESH':
                self._recursive_add_mesh(draw_gen, child)

    
    # PMDL Constructor, given a blender object for initialisation
    def __init__(self, object, draw_gen):
        
        self.object = object
        
        self.sub_type = 'PAR0'
        if object.type == 'ARMATURE':
            # Create a PAR1 file if given an ARMATURE
            self.sub_type = 'PAR1'
            self.rigging = pmdl_par1_rigging.pmdl_par1_rigging(self)
        
        
        # Break down blender object into collections
        self.collections = set()
        self._recursive_add_mesh(draw_gen, object)


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


    # This routine will generate a PMDL file with the requested draw generator
    # and endianness ['LITTLE', 'BIG'] at the requested path
    def generate_file(self, draw_generator, endianness, psize, file_path):

        # Open file and write in header
        pmdl_file = open(file_path, 'wb')
        cur_off = 0
        
        pmdl_file.write('PMDL'.encode('utf-8'))
        cur_off += 4
        
        endian_char = None
        if endianness == 'LITTLE':
            pmdl_file.write('_LIT'.encode('utf-8'))
            endian_char = '<'
        elif endianness == 'BIG':
            pmdl_file.write('_BIG'.encode('utf-8'))
            endian_char = '>'
        cur_off += 4

        pmdl_file.write(struct.pack(endian_char + 'I', psize))
        cur_off += 4

        pmdl_file.write(self.sub_type.encode('utf-8'))
        cur_off += 4

        pmdl_file.write(draw_generator.file_identifier().encode('utf-8'))
        cur_off += 4

        for comp in draw_generator.bound_box_min:
            pmdl_file.write(struct.pack(endian_char + 'f', comp))
            cur_off += 4
        for comp in draw_generator.bound_box_max:
            pmdl_file.write(struct.pack(endian_char + 'f', comp))
            cur_off += 4

        




