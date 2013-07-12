'''
PMDL Export Blender Addon
By Jack Andersen <jackoalan@gmail.com>

This file is required by Blender for Addon bootstrap
'''

# Addon info
bl_info = {
    "name": "PMDL Model Exporter",
    "author": "Jack Andersen",
    "location": "File > Export",
    "description": "Exporter for PMDL 3D Model Format",
    "category": "Import-Export"}


# To support reload properly, try to access a package var,
# if it's there, reload everything
if "bpy" in locals():
    import imp
    imp.reload(pmdl)
    imp.reload(pmdl_draw_general)
    imp.reload(pmdl_draw_gx)
    imp.reload(pmdl_draw_collision)
else:
    from . import pmdl, pmdl_draw_general, pmdl_draw_gx, pmdl_draw_collision

import bpy
from bpy.props import *
from bpy_extras.io_utils import ExportHelper


# Export PMDL
class EXPORT_OT_pmdl(bpy.types.Operator, ExportHelper):
    bl_idname = "io_export_scene.pmdl"
    bl_description = "Export current selected mesh object and child meshes to PMDL Model (.pmdl)"
    bl_label = "Export PMDL Model"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    
    # From ExportHelper. Filter filenames.
    filename_ext = ".pmdl"
    filter_glob = StringProperty(default="*.pmdl", options={'HIDDEN'})
    
    # File path property
    filepath = bpy.props.StringProperty(name="File Path",
                                        description="File path used for exporting the PMDL file",
                                        maxlen= 1024, default= "")
    
    # Octree porperties
    export_par2 = bpy.props.BoolProperty(name="PAR2 Octree",
                                         description="Generate `PAR2` PMDL with an Octree for frustum-culling",
                                         default=False)
                                         
    export_par2_levels = bpy.props.IntProperty(name="PAR2 Octree level count",
                                               description="When generating `PAR2`, this marks how many subdivisions should occur in the octree",
                                               default=5,
                                               min=1,
                                               max=10,
                                               subtype='UNSIGNED')

    # Draw format property
    def _export_draw_fmt_update(self, context):
        if self.export_draw_fmt == 'GX':
            self.export_endianness = 'BIG'
    export_draw_fmt = bpy.props.EnumProperty(name="Draw Format",
                                             description="Selects the target draw format to export",
                                             default='GENERAL',
                                             items=[('GENERAL', "General Draw Format", "Format used by OpenGL and Direct3D API"),
                                                    ('GX', "GX Draw Format", "Format used by GameCube/Wii GX API"),
                                                    ('COLLISION', "Collision Draw Format", "Format used by collision detection systems")],
                                             update=_export_draw_fmt_update)

    # Endianness Property
    export_endianness = bpy.props.EnumProperty(name="Byte Order",
                                               description="Selects the byte-ordering to export",
                                               default='LITTLE',
                                               items=[('LITTLE', "Little Endian", "Byte ordering used by many Intel and ARM based platforms"),
                                                      ('BIG', "Big Endian", "Byte ordering used by many PowerPC based platforms")],
                                               update=_export_draw_fmt_update)


    # Blender operator stuff
    @classmethod
    def poll(cls, context):
        return context.object is not None and (context.object.type == 'MESH' or context.object.type == 'ARMATURE')

    def execute(self, context):
        print("Exporting PMDL: ", self.properties.filepath)
        pmdl_obj = pmdl.pmdl(context.object)
        
        if self.properties.export_par2:
            if context.object.type == 'ARMATURE':
                self.report({'ERROR_INVALID_INPUT'}, "A PAR2 file may not be generated from an ARMATURE object")
                return {'CANCELLED'}
            octree_err = pmdl_obj.add_octree(self.properties.export_par2_levels)
            if octree_err:
                self.report({'ERROR_INVALID_INPUT'}, octree_err)
                return {'CANCELLED'}
        
        generator = None
        if self.properties.export_draw_fmt == 'GENERAL':
            generator = pmdl_draw_general.pmdl_draw_general(pmdl_obj)
        elif self.properties.export_draw_fmt == 'GX':
            generator = pmdl_draw_gx.pmdl_draw_gx(pmdl_obj)
        elif self.properties.export_draw_fmt == 'COLLISION':
            generator = pmdl_draw_collision.pmdl_draw_collision(pmdl_obj)

        if generator:
            pmdl_obj.generate_file(generator, self.properties.export_endianness, self.properties.filepath)
        else:
            self.report('ERROR_INVALID_INPUT', "Invalid draw type")

        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}



# Registration

def menu_func_export_pmdl(self, context):
    self.layout.operator(EXPORT_OT_pmdl.bl_idname, text="Photon Model (.pmdl)...")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func_export_pmdl)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func_export_pmdl)


if __name__ == "__main__":
    register()

