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