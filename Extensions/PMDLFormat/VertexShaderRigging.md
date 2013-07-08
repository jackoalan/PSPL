PMDL Vertex Shader Rigging
==========================

In order to flexibly support linear blending of skeletal bones,
a dynamic vertex-shader generation scheme is required.

It's helpful to determine what data will *always* be available
In this case, *per-vertex* position *and* normal information plus
(up to) 8 static, *per-vertex*, 2-component UV coords.
This information is *always* available via varying-data interface to
fragment shader (which is generated offline via PSPL or Blender).



Once 
