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

class pmdl:
    
    # Recursive routine to add all MESH objects as individual PMDL collections
    def _recursive_add_mesh(self, object):
        
        # Start with root object
        if object.type == 'MESH':
            if len(object.polygons):
                # Only add if there is at least *one* polygon in mesh
                self.collections.add(object)

        # Recursively incorporate child meshes
        for child in object.children:
            if child.type == 'MESH':
                self._recursive_add_mesh(child)

    
    # PMDL Constructor, given a blender object for initialisation
    def __init__(self, object):
        
        self.object = object
        
        self.sub_type = 'PAR0'
        if object.type == 'ARMATURE':
            # Create a PAR1 file if given an ARMATURE
            self.sub_type = 'PAR1'
            self.rigging = pmdl_par1_rigging.pmdl_par1_rigging(self)
        
        
        # Break down blender object into collections
        self.collections = set()
        self._recursive_add_mesh(object)


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
    def generate_file(self, draw_generator, endianness, file_path):
        pass

