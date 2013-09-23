'''
PSPL Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file contains code to generate a PSPL file from
a provided Blender MESH and MATERIAL object.

Currently supported MATERIAL properties include:

* Textures
    * IMAGE TYPE ONLY!
    * Only enabled textures are written
    * Only textures influencing 'COLOR' or 'ALPHA' are written
    * ('NORMAL' influencing textures will be implemented later using LightingManager extension)
    * Mapping Modes:
        * UV (Specified UV map)
        * Generated (Vertex Position)
        * Normal (Vertex Normal; ModelView transformed at runtime)
    * Blending Modes:
        * Multiply
        * Add
        * Subtract
        * Mix (Implement later using '~' operator line; Use latter texture's alpha channel to blend colour with former texture)
    * Stencil Mode (Use a texture slot between two textures as RGBA blending control)
'''

import bpy
import os
import posixpath

from . import pspl_uvgen

# Lookup PSPL material of MESH object
def lookup_pspl_material(mesh_obj):
    cur_mesh = mesh_obj
    
    while cur_mesh is not None:
        
        if cur_mesh.pspl_material:
            base_name = posixpath.splitext(os.path.basename(bpy.data.filepath))[0]
            mesh_name = str.replace(mesh_obj.name, "/", "_")
            mesh_name = str.replace(mesh_name, "\\", "_")
            material_name = str.replace(cur_mesh.pspl_material, "/", "_")
            material_name = str.replace(material_name, "\\", "_")
            shader_name = posixpath.normpath(os.path.dirname(bpy.data.filepath) + "/" + base_name + ".psplblend/" + mesh_name + "/" + material_name + ".pspl")
            return (shader_name, bpy.data.materials[cur_mesh.pspl_material])
        
        cur_mesh = cur_mesh.parent
    
    return None

# Bit-counter for mipmap detection
def bit_counter(val):
    count = 0
    for i in range(32):
        if (val>>i)&1:
            count += 1
    return count

# Determine if IMAGE object is mipmap-able
def image_mipmap_compatible(image):
    if bit_counter(image.size[0]) == 1 and bit_counter(image.size[1]) == 1:
        return True
    return False

# Write PSPL file
class pspl_writer:

    @classmethod
    def write_pspl(cls, mesh_obj, armature):

        # Material lookup
        material = lookup_pspl_material(mesh_obj)
        if material is None:
            return None

        # Begin writing
        pspl_out = str()
        pspl_out += ""
        "/* Auto-Generated Blender PSPL\n"
        " * File:     '{0}'\n"
        " * Mesh:     '{1}'\n"
        " * Material: '{2}'\n"
        " */\n".format(os.path.basename(bpy.data.filepath), mesh_obj.name, material[1].name)

        pspl_out += "NAME(\"{0}\")\n\n".format(material[0])


        # Write vertex section
        pspl_out += "VERTEX\n======\n"

        # If rigging (armature present),
        # determine max count of vert groups for this (sub)mesh
        if armature:
            max_bones = 0
            for polygon in mesh_obj.data.polygons:
                polygon_groups = set()
                for vert_idx in polygon.vertices:
                    for group in mesh_obj.data.vertices[vert_idx].groups:
                        polygon_groups.add(group.group)

                if max_bones < len(polygon_groups):
                    max_bones = len(polygon_groups)

            pspl_out += "BONE_COUNT({0})\n".format(max_bones)



        # Add UV generators from texture slots
        added_uvs = set()
        for slot in material[1].texture_slots:

            uv_index = pspl_uvgen.pspl_uvgen.pspl_uv_index(material[1], slot.name)
            
            if uv_index < 0:
                continue

            if uv_index in added_uvs:
                continue

            added_uvs.add(uv_index)

            uv_source = None
            if slot.texture_coords == 'UV':
                uv_source = mesh_obj.data.uv_layers.find(slot.uv_layer)
            elif slot.texture_coords == 'ORCO':
                uv_source = "POSITION"
            elif slot.texture_coords == 'NORMAL':
                uv_source = "NORMAL"

            pspl_out += "UV_GEN({0} {1})\n".format(uv_index, uv_source)


        # Write Fragment section
        pspl_out += "\n\nFRAGMENT\n========\n"

        # Add stages from texture slots
        slot_idx = 0
        for slot in material[1].texture_slots:
            
            uv_index = pspl_uvgen.pspl_uvgen.pspl_uv_index(material[1], slot.name)
            
            if uv_index < 0:
                continue
            
            if slot_idx != 0:
                if slot.blend_type == 'MIX':
                    pspl_out += "~~~"
                elif slot.blend_type == 'ADD':
                    pspl_out += "+++"
                elif slot.blend_type == 'SUBTRACT':
                    pspl_out += "+--"
                elif slot.blend_type == 'MULTIPLY':
                    pspl_out += "***"
                pspl_out += "\n"

            mipmap_str = ""
            if slot.texture.pspl_mipmap and image_mipmap_compatible(slot.texture.image):
                mipmap_str = "MIPMAP"

            pspl_out += "[SAMPLE ../{0} IMAGE {1} UV {2} {3}]\n".format(os.path.basename(bpy.data.filepath), slot.texture.image.name, uv_index, mipmap_str)
                    
            uv_source = None
            if slot.texture_coords == 'UV':
                uv_source = mesh_obj.data.uv_layers.find(slot.uv_layer)
            elif slot.texture_coords == 'ORCO':
                uv_source = "POSITION"
            elif slot.texture_coords == 'NORMAL':
                uv_source = "NORMAL"
            
            pspl_out += "UV_GEN({0} {1})\n".format(uv_index, uv_source)

            slot_idx += 1

        pspl_out += "\n\n"

        return (pspl_out, material[0])


