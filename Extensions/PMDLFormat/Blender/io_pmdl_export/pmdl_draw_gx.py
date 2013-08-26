'''
PMDL Export Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file defines the `pmdl_draw_gx` class to generate vertex-attribute
buffers and display lists to draw them. `PAR1` files also include bone-weight
coefficients per-vertex for CPU-driven skeletal evaluation.
'''

import struct
import bpy

from . import pmdl_loop_vert

# Round up to nearest 32 multiple
def ROUND_UP_32(num):
    if num%32:
        return ((num>>5)<<5)+32
    else:
        return num

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
                existing_uv_coords = uv_layer.data[existing_loop.loop.index].uv
                check_uv_coords = uv_layer.data[loop.index].uv
                if (existing_uv_coords[0] != check_uv_coords[0] or
                    existing_uv_coords[1] != check_uv_coords[1]):
                    matches = False
                    break
            if matches:
                existing_loop_set.append(lv)
                return

    # If we get here, no match found; add new set to `lv_array`
    lv_array.append([lv])


# Get loop set from collection generated with above method;
# containing a specified loop
def _get_loop_set(lv_array, mesh, loop):
    
    # Create loop_vert object for comparitive testing
    lv = pmdl_loop_vert.loop_vert(mesh, loop)

    for existing_loop_set in lv_array:
        if lv in existing_loop_set:
            return existing_loop_set
    return None


# Method to find triangle opposite another triangle over two loop-vert sets
def _find_polygon_opposite_lvs(mesh, original_triangle, lv_a, lv_b):
    a_idx = lv_a[0].loop.vertex_index
    b_idx = lv_b[0].loop.vertex_index
    
    for triangle in mesh.polygons:
        
        if triangle == original_triangle:
            continue
        
        if (a_idx in triangle.vertices and b_idx in triangle.vertices):
            return triangle

    return None




class pmdl_draw_gx:
    
    def __init__(self):
        
        # 4-byte ID string used in generated PMDL file
        self.file_identifier = '__GX'
        
        # Array that holds collections. A collection is a 16-bit index
        # worth of vertices, elements referencing them, and a
        # primitive array to draw them
        self.collections = []
    
        # Array of meshes that have been added with `add_mesh`
        # used to generate GX-specific POSITION and NORMAL buffers
        self.meshes = []
    
    
    # Method to get collection-scope vertex indices across all meshes
    def _get_collection_vertex_index(self, mesh, idx):
        result = 0
        for exist_mesh in self.meshes:
            if exist_mesh == mesh:
                return result + idx
            result += len(mesh.vertices)
    
        return None
    
        

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
    def add_mesh(self, pmdl, obj, rigger):
        mesh = obj.data
        print("Optimising mesh:", obj.name)
        opt_gpu_vert_count = 0
    
        # First, generate compressed loop-vertex array-array-set collection
        loop_vert_array = []
        for vert in mesh.vertices:
            loop_verts = []
            for loop in mesh.loops:
                if loop.vertex_index == vert.index:
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
            
            # Rolling references of last two emitted loop-vert sets (b is older)
            last_loop_vert_a = None
            last_loop_vert_b = None
                
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
                        last_loop_vert_b = last_loop_vert_a
                        last_loop_vert_a = loop_vert
                        opt_gpu_vert_count += 1
                        #print('appended initial loop', loop_vert[0].loop.index)

                    if is_new_collection:
                        break
            
                
                else: # Not the first triangle in strip; look up all three loop-verts,
                      # ensure it matches last-2 rolling reference, emit remaining loop-vert
                    
                    # Iterate loop verts
                    odd_loop_vert_out = None
                    loop_vert_match_count = 0
                    for poly_loop_idx in temp_poly.loop_indices:
                        poly_loop = mesh.loops[poly_loop_idx]
                        loop_vert = _get_loop_set(loop_vert_array[poly_loop.vertex_index], mesh, poly_loop)
                    
                        if (loop_vert == last_loop_vert_a or loop_vert == last_loop_vert_b):
                            loop_vert_match_count += 1
                            continue
                
                        odd_loop_vert_out = loop_vert
                    
                    
                    # Ensure there are two existing matches to continue tri-strip
                    if loop_vert_match_count != 2 or not odd_loop_vert_out:
                        break
                            
                    
                    # Add to tri-strip
                    if odd_loop_vert_out not in best_collection['vertices']:
                        best_collection, is_new_collection = self._check_collection_overflow(mesh, best_collection)
                        if is_new_collection:
                            break
                        best_collection['vertices'].append(odd_loop_vert_out)
                    tri_strip.append(best_collection['vertices'].index(odd_loop_vert_out))
                    last_loop_vert_b = last_loop_vert_a
                    last_loop_vert_a = odd_loop_vert_out
                    opt_gpu_vert_count += 1



                # This polygon is good
                visited_polys.add(temp_poly)
                
                
                # Find a polygon directly connected to this one to continue strip
                temp_poly = _find_polygon_opposite_lvs(mesh, temp_poly, last_loop_vert_a, last_loop_vert_b)
                if temp_poly in visited_polys:
                    temp_poly = None



            # Add tri-strip to element array
            best_collection['tri_strips'].append({'mesh':obj, 'strip':tri_strip})
            
        print("GPU will receive", opt_gpu_vert_count, "unified tri-strip vertices out of", len(mesh.loops), "original vertices")
        print("Mesh contains", len(mesh.polygons), "triangles\n")
        self.meshes.append(mesh)
        
    

    # Generate binary vertex buffer of collection index
    def generate_vertex_buffer(self, index, endian_char, psize):
        collection = self.collections[index]
        if not collection:
            return None
        
        # Generate vert buffer struct
        vstruct = struct.Struct(endian_char + 'f')
        
        # Build Vertex byte array
        vert_count = 0
        vert_bytes = bytearray()
        for mesh in self.meshes:

            # Position
            for vert in mesh.vertices:
                vert_count += 1
                for comp in vert.co:
                    vert_bytes += vstruct.pack(comp)
            
                
            
        # Build Normal byte array
        normal_bytes = bytearray()
        for mesh in self.meshes:
            
            # Normal
            for vert in mesh.vertices:
                for comp in vert.normal:
                    vert_bytes += vstruct.pack(comp)
        
        
        
        # Build UV byte array
        uv_bytes = []
        for uv_idx in range(collection['uv_count']):
            uv_sub_bytes = bytearray()
            for loop_vert in collection['vertices']:
                bloop = loop_vert[0]
                mesh = bloop.mesh
                
                # UVs
                for comp in mesh.uv_layers[uv_idx].data[loop_vert.loop.index].uv:
                    uv_sub_bytes += vstruct.pack(comp)

            uv_bytes.append(uv_sub_bytes)


        # Arrange into master buffer-struct
        master_bytes = bytearray()
        master_bytes += struct.pack(endian_char + 'I', vert_count)
        master_bytes += struct.pack(endian_char + 'I', len(collection['vertices']))
        for i in range(24):
            master_bytes.append(0)
        
        master_bytes += vert_bytes
        master_bytes += normal_bytes
        
        for uv_sub_bytes in uv_bytes:
            master_bytes += uv_sub_bytes

        return collection['uv_count'], collection['max_bone_count'], master_bytes
        
        
    # Generate binary element buffer of collection index
    def generate_element_buffer(self, index, endian_char, psize):
        collection = self.collections[index]
        if not collection:
            return None
        
        # Generate element buffer struct
        estruct = struct.Struct(endian_char + 'H')
        
        # Build mesh-primitive hierarchy
        last_mesh = collection['tri_strips'][0]['mesh']
        mesh_primitives = {'mesh':last_mesh, 'primitives':[]}
        collection_primitives = [mesh_primitives]

        
        # Last element index entry and strip length for forming degenerate strip
        last_elem = None
        strip_len = 0
        gx_bytes = bytearray() # Builds raw element-index


        # Build single degenerate tri-strip
        for strip in collection['tri_strips']:
            #print('new strip', collection['tri_strips'].index(strip))
            
            if last_mesh != strip['mesh']:
                #print('splitting primitive')
                # New mesh; force new strip
                new_gx = bytearray()
                new_gx += struct.pack('B', 0x98) # Begins a tri-strip using VAT 0
                new_gx += struct.pack('>H', strip_len) # Vertex count
                new_gx += gx_bytes
                gx_bytes = bytearray()
                
                mesh_primitives['primitives'].append({'buffer':new_gx})
                last_elem = None
                last_mesh = strip['mesh']
                mesh_primitives = {'mesh':last_mesh, 'primitives':[]}
                collection_primitives.append(mesh_primitives)
                strip_len = 0
            
            elif last_elem:
                #print('extending primitive')
                # Existing mesh being extended as degenerate strip
                loop_vert = collection['vertices'][last_elem]
                collection_index_vert = self._get_collection_vertex_index(loop_vert[0].mesh, loop_vert[0].loop.vertex_index)
                gx_bytes += struct.pack('>H', collection_index_vert)
                gx_bytes += struct.pack('>H', collection_index_vert)
                for uv_idx in range(collection['uv_count']):
                    gx_bytes += struct.pack('>H', last_elem)
                
                loop_vert = collection['vertices'][strip['strip'][0]]
                collection_index_vert = self._get_collection_vertex_index(loop_vert[0].mesh, loop_vert[0].loop.vertex_index)
                gx_bytes += struct.pack('>H', collection_index_vert)
                gx_bytes += struct.pack('>H', collection_index_vert)
                for uv_idx in range(collection['uv_count']):
                    gx_bytes += struct.pack('>H', strip['strip'][0])
                
                strip_len += 2
            
                # If current element count is odd, add additional degenerate strip to make it even
                # This ensures that the sub-strip has proper winding-order for backface culling
                if (strip_len & 1):
                    strip_len += 1
                    loop_vert = collection['vertices'][strip['strip'][0]]
                    collection_index_vert = self._get_collection_vertex_index(loop_vert[0].mesh, loop_vert[0].loop.vertex_index)
                    gx_bytes += struct.pack('>H', collection_index_vert)
                    gx_bytes += struct.pack('>H', collection_index_vert)
                    for uv_idx in range(collection['uv_count']):
                        gx_bytes += struct.pack('>H', strip['strip'][0])
                
            
            # Primitive tri-strip byte array
            for idx in strip['strip']:
                #print(idx)
                loop_vert = collection['vertices'][idx]
                collection_index_vert = self._get_collection_vertex_index(loop_vert[0].mesh, loop_vert[0].loop.vertex_index)
                gx_bytes += struct.pack('>H', collection_index_vert)
                gx_bytes += struct.pack('>H', collection_index_vert)
                for uv_idx in range(collection['uv_count']):
                    gx_bytes += struct.pack('>H', idx)
                last_elem = idx
                strip_len += 1
    
        # Final mesh entry
        new_gx = bytearray()
        new_gx += struct.pack('B', 0x98) # Begins a tri-strip using VAT 0
        new_gx += struct.pack('>H', strip_len) # Vertex count
        new_gx += gx_bytes
        
        mesh_primitives['primitives'].append({'buffer':new_gx})
            
        return collection_primitives, bytearray()


    # Generate binary draw-index buffer of collection index
    def generate_index_buffer(self, collection_primitives, endian_char, psize, rigger):

        # Bytearray to fill
        index_bytes = bytearray()
        
        # Per mesh-display list concatenations (and paddings)
        gx_dls = []
        for mesh in collection_primitives:
            mesh_dl = bytearray()
            for prim in mesh['primitives']:
                mesh_dl += prim['buffer']
            gx_pad = ROUND_UP_32(len(mesh_dl)) - len(mesh_dl)
            for i in range(gx_pad):
                mesh_dl.append(0)
            gx_dls.append(mesh_dl)

        # Table end padded offset
        offset_val = ROUND_UP_32(8*len(collection_primitives))

        # Display-list offset/length table
        for mesh_idx in range(len(collection_primitives)):
            index_bytes += struct.pack('>I', offset_val)
            index_bytes += struct.pack('>I', len(gx_dls[mesh_idx]))
            offset_val += len(gx_dls[mesh_idx])
        
        table_pad = ROUND_UP_32(8*len(collection_primitives)) - (8*len(collection_primitives))
        for i in range(table_pad):
            index_bytes.append(0)

        # And data
        for mesh_idx in range(len(collection_primitives)):
            index_bytes += gx_dls[mesh_idx]


        return index_bytes

