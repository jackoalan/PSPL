PMDL 3D Model Format
====================

**PMDL** is a *file format* and *PSPL toolchain/runtime extension* that
provides a common means to define hierarchical triangle meshes with
appropriate shader and texture bindings. 

It is designed to store model data in platform-native, *attribute-interleaved*
or *attribute-discrete* structured vertex array buffers. Standard floating-point
values may be expressed in a *bi-endian* manner. Attributed vertices are tied
together into meshes using *hierarchical topology indexing*. The runtime portion of the PSPL
extension uses this index to issue drawing commands using the host's native 3D
graphics API. 


PSPL Integration
----------------

Playing on **PSPL's** modular nature, a PMDL is also *able to reference PSPL shader 
sources* associated with the model (or a hierarchical subdivision of the model). 
PMDL also *augments the PSPL language* with a set of syntactic additions. 

One PSPL shader may be referenced by many PMDL models, yet the precise behaviour 
Of the shader may be tweaked by the specific PMDL file. This is accomplished
by including valued macro-tags within the PMDL file's *metadata*. 


Blender Integration
-------------------

[**Blender**](http://blender.org) has been chosen as PMDL's initial design and
development toolchain. As such, PMDL also includes a series of *Python-language Blender
Add-ons*. These add-ons allow complete control of mesh data saved into a PMDL
file, using Blender's conventions. 
