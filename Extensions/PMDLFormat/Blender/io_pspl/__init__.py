'''
PSPL Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file is required by Blender for Addon bootstrap
'''

# To support reload properly, try to access a package var,
# if it's there, reload everything
if "bpy" in locals():
    import imp
    imp.reload(pspl_writer)
    imp.reload(pspl_uvgen)
else:
    from . import pspl_writer
    from . import pspl_uvgen


import os
import bpy
from bpy.app.handlers import persistent

# Addon info
bl_info = {
    "name": "PSPL Blender Integration",
    "author": "Jack Andersen",
    "location": "Properties > Material",
    "description": "Enables Blender to export PSPL material definitions alongside PMDL models",
    "category": "Material"}



# PSPL Texture Sub-panel class (Provides UV Index Label)
class TEXTURE_PT_pspl(bpy.types.Panel):
    bl_idname = "TEXTURE_PT_pspl"
    bl_label = "PSPL"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "texture"
    
    @classmethod
    def poll(cls, context):
        return (bpy.context.object.active_material.active_texture is not None)
    
    
    def draw(self, context):
        layout = self.layout
        obj = context.object
        
        pspl_uv_idx = pspl_uv_index(obj.active_material, obj.active_material.active_texture.name)

        slot = obj.active_material.texture_slots[obj.active_material.active_texture_index]

        if pspl_uv_idx >= 0:
            layout.prop(obj.active_material.active_texture, "pspl_mipmap", text="PSPL MIPMAP")
            layout.prop_search(obj.active_material.active_texture, "pspl_uv_share_texture", obj.active_material, "texture_slots", text="UV Share Texture:")
            layout.label(text="UV Index: {0}".format(pspl_uv_idx))

        else:
            layout.label(text="PSPL-Incompatible texture slot")

            if obj.active_material.active_texture.type != 'IMAGE':
                layout.label(text="Texture must be of 'IMAGE' type.")
                layout.label(text="Please bake this texture to a UV-mapped image.")
            
            elif slot.blend_type not in pspl_compatible_blend_types:
                layout.label(text="Texture must use one of these blend modes:")
                layout.label(text=str(pspl_compatible_blend_types))
            
            elif slot.texture_coords not in pspl_compatible_uvgen_types:
                layout.label(text="Invalid Coordinate Mapping Type")
                layout.label(text="Please use one of [UV, Generated, Normal]")

            elif not slot.use:
                layout.label(text="Texture must be enabled (slot checkbox checked)")

            elif not (slot.use_map_color_diffuse or slot.use_map_alpha):
                layout.label(text="Texture must influence diffuse color or alpha")




# PSPL Object Sub-panel class (Provides Singular Material Selection)
class OBJECT_PT_pspl(bpy.types.Panel):
    bl_idname = "OBJECT_PT_pspl"
    bl_label = "PSPL"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "object"

    @classmethod
    def poll(cls, context):
        return (bpy.context.object is not None)

    def draw(self, context):
        layout = self.layout
        obj = context.object
        
        layout.prop_search(obj, "pspl_material", bpy.data, "materials", text="PSPL Material:")


# Save handler
@persistent
def save_handler(dummy):
    base_name = os.path.splitext(os.path.basename(bpy.data.filepath))[0]
    dir_name = os.path.normpath(os.path.dirname(bpy.data.filepath) + "/" + base_name + ".psplblend")
    
    try:
        os.mkdir(dir_name)
    except:
        pass

    # Iterate all MESH objects to write MATERIAL set
    for mesh_obj in bpy.data.objects:
        
        if mesh_obj.type != 'MESH':
            continue
        
        armature = mesh_obj
        while armature is not None:
            if armature.type != 'ARMATURE':
                armature = armature.parent
            else:
                break

        pspl = pspl_writer.write_pspl(mesh_obj, armature)


        if pspl is not None:
            
            pspl_path = os.path.normpath(pspl[1])
            
            try:
                os.mkdir(os.path.dirname(pspl_path))
            except:
                pass

            pspl_file = open(pspl_path, 'w')
            pspl_file.write(pspl[0])
            pspl_file.close()


# Registration
def register():
    bpy.utils.register_module(__name__)
    bpy.types.Object.pspl_material = bpy.props.StringProperty(name="PSPL Material", description="Singular material object used when generating mesh's PSPL")
    bpy.types.Texture.pspl_uv_share_texture = bpy.props.StringProperty(name="PSPL UV Share Texture", description="Texture object to share UV generator with at runtime")
    bpy.types.Texture.pspl_mipmap = bpy.props.BoolProperty(name="PSPL Mipmap", description="Instruct PSPL to generate MIPMAP pyramid (as long each dimension is an exponential of base 2)", default=True)
    bpy.app.handlers.save_post.append(save_handler)


def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.app.handlers.save_post.remove(save_handler)



if __name__ == "__main__":
    register()

