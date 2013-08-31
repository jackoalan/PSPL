PMDL 3D Model Format
====================

**PMDL** is a *file format* and *PSPL toolchain/runtime extension* that
provides a common means to define hierarchical triangle meshes with
appropriate shader and texture bindings. 

It is designed to store model data in platform-native vertex buffers.
Attributed vertices are indexed together into meshes using degenerate triangle-strips. 
The runtime portion of the PMDL extension uses this index to issue drawing commands 
using the host's native 3D graphics API. 

[Read the PMDL Specification](https://github.com/jackoalan/PSPL/blob/master/Extensions/PMDLFormat/Specification.md) 
for details. 


Skeletal Rigging Support
------------------------

The PMDL format includes a sub-type for **skeletal rigging**. 

The rigging data includes a *bone-tree*, vertex *skinning information* and
an arbitrary number of *actions* (animated clips). 


PSPL Integration
----------------

Playing on **PSPL's** modular nature, a PMDL is also *able to reference PSPL shader 
sources* associated with the model (or a hierarchical subdivision of the model). 
PMDL also *augments the PSPL language* with a set of syntactic additions. 

Any number of PMDL files may be referenced in a PSPL file. Each PMDL is identified
using a *key string* incorporated into a hash table. 


Blender Integration
-------------------

[**Blender**](http://blender.org) has been chosen as PMDL's initial 3D design and
development toolchain. As such, PMDL also includes a series of *Python-language Blender
Add-ons*. These add-ons allow complete control of mesh data saved into a PMDL
file, using Blender's on-board Python API. 
