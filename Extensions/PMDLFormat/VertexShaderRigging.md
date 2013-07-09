PMDL Vertex Shader Rigging
==========================

In order to flexibly support linear blending of skeletal bones,
a dynamic vertex-shader generation scheme is required.

It's helpful to determine what data will *always* be available
In this case, *per-vertex* position *and* normal information plus
(up to) 8 static, *per-vertex*, 2-component UV coords.
This information is *always* available via varying-data interface to
fragment shader (which is generated offline via PSPL or Blender).

Even though the shader *always* accepts 8 UV coordinates per-vertex,
the vertex buffer itself may contain less, so the shader automatically
receives zero for the remaining UV attributes.

In addition to the standard vertex attributes indexed 0-9, attributes
10 and 11 are 4-component bone-weight values. 2x 4-component values
allows for up to eight blending-coefficients. 


The Vertex Shader Itself
------------------------

Given the known consistent structure above, the only variance between
PMDL Vertex programs needs to be bone count. Within the PMDL runtime,
rendering batches are bound to individual *skin-assist* structures.
These structures define the number of bones for the rendering batch
(and consequently, the number of blending coefficients contained within
the vertex buffer itself). The skin-assist structure also selects which 
bones (out of the entire set) need to be loaded into the GPU for that 
batch.

All in all, 9 vertex shaders need to be compiled and available for the
GPU (ranging from 0 bones to 8 bones). Obviously, the linear transformations
involved for bones are the most expensive operation in the vertex program.
Limiting the count of these operations helps tremendously.
