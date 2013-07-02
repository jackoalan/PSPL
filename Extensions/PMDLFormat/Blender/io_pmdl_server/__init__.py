'''
    This is the main Blender PMDL addon file for PSPL.
    It contains code to host a server (accessible locally or remotely; visible to PSPL)
    and write out PMDL data as specified by PSPL.
'''

# Addon info
bl_info = {
    "name": "PMDL Server",
    "author": "Jack Andersen",
    "location": "(Hosts a server)",
    "description": "Server to Generate and Provide PMDL data to PSPL client",
    "category": "Import-Export"}


# To support reload properly, try to access a package var,
# if it's there, reload everything
if "bpy" in locals():
    import imp
    if 'pmdl_server' in locals():
        imp.reload(pmdl_server)
    if 'pmdl_writer' in locals():
        imp.reload(pmdl_writer)

import bpy
import socketserver
from . import pmdl_server


# PMDL Preferences
class PMDLAddonPreferences(bpy.types.AddonPreferences):
    bl_idname = __name__
    
    remote_access = bpy.props.BoolProperty(name="Enable Remote PSPL Access",
                                           description="Hosts a remotely-accessible TCP server to provide PSPL access",
                                           default=False)
    
    remote_port = bpy.props.IntProperty(name="Remote Access Port",
                                        description="Port number for the remotely-accessible TCP server",
                                        default=33320,
                                        min=0,
                                        max=65535,
                                        subtype='UNSIGNED')
                                        
    # Server instances
    @classmethod
    def start_local_server(self):
        print("starting")
        self.local_server = socketserver.TCPServer(("localhost", 33320), pmdl_server)
        self.local_server.serve_forever()
    
    @classmethod
    def stop_local_server(self):
        print("stopping")
        self.local_server.server_close()

                          
    def draw(self, context):
        layout = self.layout
        layout.prop(self, "remote_access")
        layout.prop(self, "remote_port")





def register():
    bpy.utils.register_class(PMDLAddonPreferences)
    PMDLAddonPreferences.start_local_server()
    

def unregister():
    bpy.utils.unregister_class(PMDLAddonPreferences)
    PMDLAddonPreferences.stop_local_server()


if __name__ == "__main__":
    register()

