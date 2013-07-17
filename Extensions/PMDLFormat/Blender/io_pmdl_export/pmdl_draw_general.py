'''
PMDL Export Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file defines the `pmdl_draw_general` class to generate vertex+index 
buffers and mesh arrays to draw them. `PAR1` files also include bone-weight
coefficients per-vertex for vertex-shader-driven skeletal evaluation.
'''

import struct
import bpy

from . import pmdl_loop_vert

# This routine conditionally inserts a loop into a multi-tiered
# array/set collection; simultaneously relating verts to loops and
# eliminating redundant loops (containing identical UV coordinates)
def _augment_loop_vert_array(lv_array, mesh, loop):
    
    # Create loop_vert object for comparitive testing
    lv = pmdl_loop_vert.loop_vert(mesh, loop)
    
    # First perform quick check to see if loop is already in a set
    for existing_loop_set in lv_array:
        if lv in existing_loop_set:
            return

    # Now perform extended check to see if any UV coordinate values already match
    for existing_loop_set in lv_array:
        for existing_loop in existing_loop_set:
            matches = True
            for uv_layer in mesh.uv_layers:
                existing_uv_coords = uv_layer.data[existing_loop.index].uv
                check_uv_coords = uv_layer.data[loop.index].uv
                if (existing_uv_coords[0] != check_uv_coords[0] or
                    existing_uv_coords[1] != check_uv_coords[1]):
                    matches = False
                    break
            if matches:
                existing_loop_set.add(lv)
                return

    # If we get here, no match found; add new set to `lv_array`
    lv_array.append(set([lv]))


# Get loop set from collection generated with above method;
# containing a specified loop
def _get_loop_set(lv_array, mesh, loop):
    
    # Create loop_vert object for comparitive testing
    lv = pmdl_loop_vert.loop_vert(mesh, loop)

    for existing_loop_set in lv_array:
        if lv in existing_loop_set:
            return existing_loop_set
    return None


# Method to find polygon on opposite side of edge
def _find_polygon_opposite_edge(mesh, polygon, edge):
    for poly in mesh.polygons:
        
        # Skip provided polygon
        if poly == polygon:
            continue

        # Determine if polygon touches edge
        for loop_idx in poly.loop_indices:
            if edge.index == mesh.loops[loop_idx].edge_index:
                return poly
                
    return None




class pmdl_draw_general:
    
    def __init__(self):
        
        # Array that holds collections. A collection is a 16-bit index
        # worth of vertices, elements referencing them, and a
        # primitive array to draw them
        self.collections = []
    
    
        # Bound box of all meshes
        self.bound_box_accounted = False
        self.bound_box_min = [0,0,0]
        self.bound_box_max = [0,0,0]
    
    
    def file_identifier(self):
        return '_GEN'
    
    
    # If vertex index space is exceeded for a single additional vertex,
    # a new collection is created and returned by this routine
    def _check_collection_overflow(self, mesh, collection):
        if not collection or len(collection['vertices']) >= 65535:
            new_collection = {'uv_count':len(mesh.uv_layers), 'max_bone_count':0, 'vertices':[], 'tri_strips':[]}
            self.collections.append(new_collection)
            return new_collection, True
        else:
            return collection, False


    # Augments draw generator with a single blender MESH data object
    def add_mesh(self, obj):
        mesh = obj.data
        
        # Account for bound box
        bbox = obj.bound_box
        if not self.bound_box_accounted:
            self.bound_box_accounted = True
            for i in range(3):
                self.bound_box_min[i] = bbox[0][i]
                self.bound_box_max[i] = bbox[6][i]
        else:
            for i in range(3):
                if self.bound_box_min[i] > bbox[0][i]:
                    self.bound_box_min[i] = bbox[0][i]
                if self.bound_box_max[i] < bbox[6][i]:
                    self.bound_box_max[i] = bbox[6][i]
    
        # First, generate compressed loop-vertex array-array-set collection
        loop_vert_array = []
        for vert in mesh.vertices:
            loop_verts = []
            for loop in mesh.loops:
                _augment_loop_vert_array(loop_verts, mesh, loop)
            loop_vert_array.append(loop_verts)
    

        # Find best collection to add mesh data into
        best_collection = None
        for collection in self.collections:
            if (collection['uv_count'] == len(mesh.uv_layers) and
                collection['max_bone_count'] == 0 and
                len(collection['vertices']) < 65000):
                best_collection = collection
                break
        if not best_collection:
            # Create a new one if no good one found
            best_collection, is_new_collection = self._check_collection_overflow(mesh, None)
        
        
        # Now begin generating draw primitives
        visited_polys = set()
        for poly in mesh.polygons:
            # Skip if already visited
            if poly in visited_polys:
                continue
                
            # Begin a tri-strip primitive (array of vert indices)
            tri_strip = []
            
            # Temporary references to trace out strips of triangles
            temp_poly = poly
            temp_edge = None
                
            # In the event of vertex-buffer overflow, this will be made true;
            # resulting in the immediate splitting of a tri-strip
            is_new_collection = False
            
            # As long as there is a connected polygon to visit
            while temp_poly:
                if 0 == len(tri_strip): # First triangle in strip
                
                    # Add three loop-vert vertices to tri-strip
                    for poly_loop_idx in temp_poly.loop_indices:
                        poly_loop = mesh.loops[poly_loop_idx]
                        loop_vert = _get_loop_set(loop_vert_array[poly_loop.vertex_index], mesh, poly_loop)
                        if loop_vert not in best_collection['vertices']:
                            best_collection, is_new_collection = self._check_collection_overflow(mesh, best_collection)
                            if is_new_collection:
                                break
                            best_collection['vertices'].append(loop_vert)
                        tri_strip.append(best_collection['vertices'].index(loop_vert))

                    if is_new_collection:
                        break
                
                
                else: # Not the first triangle in strip
                    
                    # Find odd loop-vert out, add to tri-strip
                    loop_vert = None
                    for vert_idx in temp_poly.vertices:
                        if vert_idx in temp_edge.vertices:
                            continue
                        for loop_idx in temp_poly.loop_indices:
                            loop = mesh.loops[loop_idx]
                            if loop.vertex_index == vert_idx:
                                loop_vert = _get_loop_set(loop_vert_array[vert_idx], mesh, loop)
                                break
                        break
            
                    if loop_vert not in best_collection['vertices']:
                        best_collection, is_new_collection = self._check_collection_overflow(mesh, best_collection)
                        if is_new_collection:
                            break
                        best_collection['vertices'].append(loop_vert)
                    tri_strip.append(best_collection['vertices'].index(loop_vert))
                
                
                # This polygon is good
                visited_polys.add(temp_poly)
                temp_edge = None
                find_poly = temp_poly
                temp_poly = None
                
                # Find a polygon directly connected to this one to continue strip
                for poly_loop_idx in find_poly.loop_indices:
                    poly_loop = mesh.loops[poly_loop_idx]
                    poly_edge = mesh.edges[poly_loop.edge_index]
                
                    opposite_poly = _find_polygon_opposite_edge(mesh, find_poly, poly_edge)
                    if opposite_poly and opposite_poly not in visited_polys:
                        temp_edge = poly_edge
                        temp_poly = opposite_poly
                        break
                
                
            # Add tri-strip to element array
            best_collection['tri_strips'].append({'mesh':mesh, 'strip':tri_strip})
            
        


    # Augments draw generator with a single blender MESH data object and
    # associated par1_rigging object
    def add_rigged_mesh(self, mesh, rigger):
        pass
    

    # Generate binary vertex buffer of collection index
    def generate_vertex_buffer(self, index, endianness):
        collection = self.collections[index]
        if not collection:
            return None
        
        # Generate vert buffer struct
        endian_char = None
        if endianness == 'LITTLE':
            endian_char = '<'
        elif endianness == 'BIG':
            endian_char = '>'
        vstruct = struct.Struct(endian_char + 'f')
        
        # Build byte array
        vert_bytes = bytearray()
        for loop_vert in collection['vertices']:
            bloop = loop_vert[0]
            mesh = bloop.id_data
            bvert = mesh.vertices[bloop.vertex_index]
            
            # Position
            for comp in bvert.co:
                vert_bytes.append(vstruct.pack(comp))
                
            # Normal
            for comp in bvert.normal:
                vert_bytes.append(vstruct.pack(comp))
            
            # UVs
            for uv_idx in range(collection['uv_count']):
                for comp in mesh.uv_layers[uv_idx].data.uv:
                    vert_bytes.append(vstruct.pack(comp))
                
        return collection['uv_count'], collection['max_bone_count'], vert_bytes
        
        
    # Generate binary element buffer of collection index
    def generate_element_buffer(self, index, endianness):
        collection = self.collections[index]
        if not collection:
            return None
        
        # Generate element buffer struct
        endian_char = None
        if endianness == 'LITTLE':
            endian_char = '<'
        elif endianness == 'BIG':
            endian_char = '>'
        estruct = struct.Struct(endian_char + 'H')
        
        # Build mesh-primitive hierarchy
        last_mesh = collection['tri_strips'][0]['mesh']
        mesh_primitives = {'mesh':last_mesh, 'primitives':[]}
        collection_primitives = [mesh_primitives]
        
        # Collection element byte-array
        cur_offset = 0
        element_bytes = bytearray()
                
        for strip in collection['tri_strips']:
            if last_mesh != strip['mesh']:
                last_mesh = strip['mesh']
                mesh_primitives = {'mesh':last_mesh, 'primitives':[]}
                collection_primitives.append(mesh_primitives)
            
            # Primitive tri-strip byte array
            for idx in strip['strip']:
                element_bytes.append(estruct.pack(idx))
                
            mesh_primitives['primitives'].append({'offset':cur_offset, 'length':len(strip['strip'])})
            cur_offset += len(strip['strip'])
            
        return collection_primitives, element_bytes

