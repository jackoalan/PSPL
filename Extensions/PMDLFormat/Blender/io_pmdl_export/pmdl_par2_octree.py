'''
PMDL Export Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file defines the `pmdl_par2_octree` class to construct a hierarchical
octree structure for `PAR2` PMDL files. Individual mesh objects insert
their indices into the octree as leaf nodes. The resulting reference
structure is written into the file.
'''

class pmdl_par2_octree:
    
    # Set up with collection-populated PMDL object
    def __init__(self, pmdl):
        for collection in pmdl.collections:
            
