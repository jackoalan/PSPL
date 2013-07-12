'''
PMDL Export Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file defines the `pmdl_draw_general` class to generate vertex+index 
buffers and mesh arrays to draw them. `PAR1` files also include bone-weight
coefficients per-vertex for vertex-shader-driven skeletal evaluation.
'''

class pmdl_draw_general:

    # Used to lookup an existing or create a new object
    # representing a polygon-specific vertex
    def _get_loop_vertex(self, mesh, vertex, polygon):
    
        # Try to lookup existing loop vertex
        loop_vert = None
        for lv in self.loop_verts:
            if lv['mesh'] == mesh and lv['vertex'] == vertex:
                if polygon in lv['polygons']:
                    loop_vert = lv
                    break
                
                # Same vertex, but different polygon;
                # determine if we can add to the loop vert set
                uv_match = False
                for loop_idx in polygon.loop_indices:
                    for layer in mesh.uv_layers:
                        coords = layer.data[loop_idx].uv
                        if coords[0] == 
                
                break

        # Create if not found
        if not loop_vert:
            coords = []
            for layer in mesh.uv_layers:
                
            loop_vert = {'mesh':mesh, 'vertex':vertex, 'polygons':set(polygon), 'uv_coords':coords}

    def __init__(self, pmdl):
        self.object = pmdl.object
        self.loop_verts = []

        for mesh in pmdl.collections:
            
            
            for poly in mesh.polygons:
                
