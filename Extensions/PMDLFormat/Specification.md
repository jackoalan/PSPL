PMDL Format Specification
=========================

The **PMDL 3D Model Format** is a *runtime-optimised* file-format that's
designed to be used in various scenarios. As such, there are three different
PMDL *sub-type* formats, each sharing many indexing characteristics, but each 
bringing their own as well.

* [Non-partitioned model (`PAR0`)](#non-partitioned-models)
    * For small, static models (ideal for stationary objects)
* [Rigged non-partitioned model (`PAR1`)](#rigged-models)
    * For small, skeletally-rigged models (ideal for animated characters and objects)
* [Partitioned model (`PAR2`)](#partitioned-models)
    * For large, static models (ideal for environments that enclose the camera-view)

PMDL files are designed to be packaged into PSPL package-files and contain
internal references to PSPL objects for shader-specifications.

As far as spacial-representation is concerned, a right-handed, XYZ coordinate system
is used throughout.


General Layout
--------------

All PMDL sub-types have the following general data layout:

* Header
    * Magic (`PMDL`)
    * File-wide endianness (`_LIT`, `_BIG`)
    * Pointer-size (in bytes; 32-bit word)
        * PMDL features explicit zero-regions to provide the PMDL runtime
          with pre-allocated space to store internal pointer-references in context.
          This value marks the length of these regions (generally 4 or 8 bytes).
          This value is validated early on at load time. For instance,
          a 64-bit build of the PMDL runtime *rejects* PMDLs that hold pointers 
          *not* 8-bytes in size.
    * Sub-type ([`PAR0`](#non-partitioned-models), [`PAR1`](#rigged-models), [`PAR2`](#partitioned-models))
    * Draw-buffer format ([`_GEN`](#general-draw-format), [`__GX`](#gx-draw-format), [`_COL`](#collision-draw-format))
    * Master [AABB](http://en.wikipedia.org/wiki/Bounding_volume) (2x points; 6x 32-bit floats; (XYZ mins, XYZ maxes))
    * Draw-buffer-collection array offset (32-bit word)
    * Count of draw-buffer collections (32-bit word)
    * Shader-object reference absolute offset (32-bit word)
    * Bone string-table absolute offset (32-bit word)
    * 32-byte alignment padding (4 bytes)
* [Rigged Skeleton Info Section](#rigged-skeleton-info-section) (`PAR1` only)
* [Rigged Skinning Info Section](#rigged-skinning-info-section) (`PAR1` only)
* [Rigged Animation Section](#rigged-animation-section) (`PAR1` only)
* [Partitioned Octree Section](#octree-section) (`PAR2` only)
* [Draw-buffer-collection array](#draw-buffer-collections)
    * Header data (one for each collection)
        * UV-attribute count (16-bit word)
        * Bone count (16-bit word; used by `PAR1` only)
            * Note that this only indicates the *maximum* number of bones referenced
              by vertices in this buffer, unused bone-weight-attribute values are zero
        * Vertex-buffer offset (32-bit word)
        * Vertex-buffer length (32-bit word)
        * Element-buffer offset (32-bit word)
        * Element-buffer length (32-bit word)
        * Drawing-index offset (32-bit word)
    * 32-byte alignment padding
    * Vertex-buffer data (one for each collection)
        * The format of this blob is indicated by the draw-buffer format
    * Element-buffer data (one for each collection)
        * The format of this blob is indicated by the draw-buffer format
    * 32-byte alignment padding
    * Drawing-index buffer data (one for each collection)
        * Drawing-index mesh count (32-bit word)
        * Drawing mesh
            * Mesh AABB (contained within Master AABB; same format)
            * Mesh shader reference (32-bit word; index into SHA1 table)
                * In `PAR2` models, the octree leaves may multiply-reference
                  meshes that extend across leaf-nodes. Due to the possibility of
                  over-draw in this indexing-scheme, the MSB of this offset value
                  is set to 1 when this mesh is drawn so the PMDL runtime won't
                  draw it again if already set. At the end of the frame, all 
                  of these draw bits are reset to 0.
        * Mesh blob
            * The format of this blob is indicated by the draw-buffer format
    * 32-byte alignment padding
* Shader-object references
    * Reference count (32-bit word)
    * Reference array (array of 20-byte SHA1 hashes)
        * These hashes correspond to PSPL objects defining shader-configurations
* Bone String table (`PAR1` only)
    * Bone array
        * Null-terminated string for bone name
* 32-byte-rounded `0xff` padding


### Draw Buffer Collections ###

The root-level hierarchy of draw-buffer data is the **Draw-Buffer-Collection array**.
Each collection instance within the array consists of a *vertex-attribute buffer*
and a *draw-index buffer*. The draw-index buffer references vertices within
its paired vertex-attribute buffer using a 16-bit index value. 

For complex models with a large number of vertices exceeding 16-bit indexing space,
latter portions of the model may overflow into a new collection; granting a brand-new
indexing space.

Another reason to have separate collection is for models that have a widely-varied,
yet topologically-separable vertex format (UV-attribute or bone counts).


Non-Partitioned Models
----------------------

Small models that generally fit within the 
[viewing frustum](http://en.wikipedia.org/wiki/Viewing_frustum) 
are known as **non-partitioned** or **`PAR0`** models. These models have a single 
[bounding box](http://en.wikipedia.org/wiki/Bounding_volume)
that may be oriented with the model's *master transform* at runtime (OBB).

When the PMDL runtime calls for a non-partitioned model to be drawn,
a frustum test is performed. If its OBB is determined to (at least partially) 
lie within the view-frustum, the *entire* model is drawn with master transform 
and view-projection transform applied.

The only partitioning that takes place is within the *drawing-index*. 
Model vertices are indexed into element-arrays, whose format is laid out
specially for the target graphics API. These element-arrays are split into 
sub-sections. Each sub-section has a header indicating if/how the platform 
should bind a different shader-configuration (including texture selection).
This allows single models with many materials to be drawn accordingly.


Rigged Models
-------------

Non-Partitioned models may also incorporate **skeletal-rigging** data.
These are known as **`PAR1`** models. Like regular non-paritioned models,
rigged models have *exactly one* OBB and a *master transform* for runtime 
drawing.

The main file header and drawing sub-sections include extra data indicating 
how the PMDL runtime will transform data for skeletally-rigged models.


### Rigged Skeleton Info Section ###

* Section length (32-bit word)
* Bone structure count (32-bit word)
* Bone structure offset array
    * Relative offsets into array marking individual entries (32-bit word)
* Bone structure array (each of variable length)
    * Bone name absolute offset (32-bit word)
        * References into string table at end of file
    * Armature-relative bone-head coordinates (3x float + 1 padding float)
    * Parent bone index (or -1 if no parent) (signed 32-bit word)
    * Child count (32-bit word)
    * Child index array
        * Bone indices of each child bone (32-bit word)


### Rigged Skinning Info Section ###

* Section length (32-bit word)
* Skin structure count (32-bit word)
* Skin structure offset array (32-bit words)
    * Relative offsets into array marking individual entries
* Skin structure array (each of variable length)
    * Bone count (32-bit word; up to 8)
    * Bone array
        * Bone index (matches index in *skeleton info section*)

            
### Rigged Animation Section ###

`PAR1` files are also able to embed pre-recorded **animation actions**.
The animated data includes *bone rotation* (expressed as quaternion), 
*bone scale*, and/or *bone position*. Each keyframe is a 2D 
[cubic bézier](http://en.wikipedia.org/wiki/Bézier_curve) handle,
permitting smoothly curved animation playback. Actions consisting of *one* keyframe
are used to express static poses.

* Action structure count (32-bit word)
* String table offset (32-bit word)
* Action structure offset array (32-bit words)
    * Relative offsets into array marking individual entries
* Action structure array (each of variable length)
    * Bone Count
    * Action duration (float)
    * Bone array (read in linearly)
        * Bone index (lower 29 bits; matches index in *skeleton info section*) and property bitfield (upper 3 bits)
        * (Optional) Scale Array
            * X Keyframe Count
            * X Keyframe Array
                * Left control point coordinates
                * Main control point coordinates
                * Right control point coordinates
            * Y Keyframe Count
            * Y Keyframe Array
            * Z Keyframe Count
            * Z Keyframe Array
        * (Optional) Rotation Array (4-component quaternion values)
            * W Keyframe Count
            * W Keyframe Array
                * Left control point coordinates
                * Main control point coordinates
                * Right control point coordinates
            * X Keyframe Count
            * X Keyframe Array
            * Y Keyframe Count
            * Y Keyframe Array
            * Z Keyframe Count
            * Z Keyframe Array
        * (Optional) Position Array
            * X Keyframe Count
            * X Keyframe Array
                * Left control point coordinates
                * Main control point coordinates
                * Right control point coordinates
            * Y Keyframe Count
            * Y Keyframe Array
            * Z Keyframe Count
            * Z Keyframe Array
* Action strings offset array (32-bit words)
    * Relative offsets into string table marking individual entries
* Action string table
    * Null-terminated strings naming actions



Partitioned Models
------------------

Finally, for efficient, **hierarchically-frustum-culled** *static-environment* rendering, 
there is the **`PAR2`** format. This format allows a large, volumnous model to be 
sub-divided using an [octree](http://en.wikipedia.org/wiki/Octree). The PMDL runtime employs recursive 
[view-frustum culling](http://en.wikipedia.org/wiki/Hidden_surface_determination#Viewing_frustum_culling)
against octree nodes when the model's draw routine is called. The octree subdivides the master 
AABB in a uniform manner across three dimensions.

### Octree Section ###

* Octree structure
    * Octree node array (ordered most-significant hierarchy to least-significant)
        * 8x2-bit sub-node-type-indicator
            * 0b00 - NULL node
            * 0b01 - sub-node
            * 0b10 - leaf
        * 16-bit padding
        * For each non-NULL node, a 32-bit Octree-relative offset of sub-node or tree-leaf
* 4-byte-rounded padding
* Tree leaves (variable count)
    * Mesh count (32-bit word)
    * Mesh array
        * Collection index (32-bit word)
        * Mesh index (32-bit word)
        

General Draw Format
-------------------

When a PMDL file is made to target OpenGL or Direct3D, the **general draw-format** is used.
The General format is designed to operate *natively* with 
[OpenGL VBO](http://www.opengl.org/wiki/Vertex_Buffer_Object#Vertex_Buffer_Object) objects and
[Direct3D buffer](http://msdn.microsoft.com/en-us/library/ff476351.aspx) objects. 


### General Vertex Buffer Format ###

* Buffer Length (32-bit word)
* Vertex Buffer
    * Attribute-interleaved, single-precision floats
    * 1x 3-component position
    * 1x 3-component normal
    * 0-8x 2-component UV
    * (`PAR1` only) 1-2x 4-component bone-weight coefficients (8 bone-per-primitive maximum)
    
    
### General Drawing Index Buffer Format ###

* 3x Pointer-space structure to hold the following members
    * OpenGL *VAO* or Direct3D *InputLayout*
    * OpenGL *VBO* or Direct3D *Buffer* for Vertex attributes
    * OpenGL *VBO* or Direct3D *Buffer* for Element indices
* Mesh array (each variable length)
    * Primitive count (32-bit word)
    * Primitive array
        * (`PAR1` only) index of skin structure to be bound (32-bit word)
            * PMDL generators should take care to arrange same-skin primitives 
              contiguously within the mesh. If the previously-bound skin is requested
              again, redundant GPU-loading overhead may be avoided.
        * Primitive-type enumeration (32-bit word)
            * 0 - points
            * 1 - non-batched triangles
            * 2 - triangle-fans
            * 3 - triangle-strips
            * 4 - non-batched lines
            * 5 - line-strips
        * Primitive-element start index (32-bit word)
        * Primitive-element count (32-bit word)


GX Draw Format
--------------

The PMDL format includes a drawing-index and vertex-buffer format specifically 
designed for use with [GameCube/Wii **GX** API](http://libogc.devkitpro.org/gx_8h.html).

The GX API uses OpenGL 1.0 style display-lists (rather than element-buffers)
to store drawing commands for rapid, on-demand drawing. The binary-format of these
display-lists is consistent for the GX platform and
[documented accordingly](http://hitmen.c02.at/files/yagcd/yagcd/chap5.html#sec5.11.5).
This means it's possible to generate a drawing display-list offline and embed its
buffer within the PMDL directly, making for a very lightweight initialisation routine 
at runtime.

Additionally, GX does draw-indexing on a *per-vertex-attribute* basis rather than
a *per-entire-vertex* basis (like OpenGL or Direct3D). This means that the vertex-buffer
expresses a *structure-of-arrays* rather than an *array-of-structures*.
Redundant instances of vertex attributes may be eliminated and multiply-referenced within
the drawing-index. For complex models, this may lead to a smaller overall vertex-buffer
size and more efficient use of the GPU's vertex cache.

Unfortunately, GX doesn't have a very flexible method to apply blended skeletal transforms
on the GPU. Therefore, the `PAR1` format employs a per-vertex *bone-weight-coefficient*
array to match the regular *position* and *normal* arrays. All verts in the arrays
are computed (on the CPU) against the current bone transforms referenced via the 
PMDL skin entries.


### GX Vertex Buffer Format ###

* (`PAR1` only) Relative offset to bone-weighting buffers after main buffers
* *Position/normal* count (32-bit word)
* *UV* count (32-bit word)
* 32-byte-aligned padding
* Array Buffers
    * Each array is prepended and appended with 32-byte padding
    * Each value is expressed as 2 or 3 component, single-precision floats
* (`PAR1` only) Pointer to dynamically-allocated *position-transform* stage (pointer space)
* (`PAR1` only) Pointer to dynamically-allocated *normal-transform* stage (pointer space)
* (`PAR1` only) *Bone-weight coefficient* array
    * Skin-entry index (32-bit word)
    * Weight-values (32-bit floats; matches count of bones in enclosing collection)
    * Has same count and order of *position* and *normal* array entries
    * Before the PMDL is rendered, the transform stages are populated with
      blended, skeletally-transformed versions for the entire model. The
      master modelview transform is still applied by the GPU at draw-time.
    
    
### GX Drawing Index Buffer Format ###

* Mesh array (each variable length)
    * Primitive count (32-bit word)
    * Primitive array
        * Primitive-element display list offset (32-bit word; relative within display list array below)
        * Primitive-element display list length (32-bit word)
* Element-buffer dispay list array
    * A series of 32-byte-aligned and 0x0-padded display lists


Collision Draw Format
---------------------

PMDL may also be used to express *trimesh-geometry* for **collision-detection** systems.
In this format, 32-bit indexing is used in the drawing index (rather than 16)
and *only one* draw buffer collection is specified. 

When paired with the `PAR2` format, hierarchial collision geometry may be expressed
for efficient collider routines.
