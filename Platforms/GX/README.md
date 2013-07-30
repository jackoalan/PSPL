GX Support
==========

**GX** is a *fixed-function*, yet considerably advanced 3D rasterisation 
platform designed by [ArtX](http://en.wikipedia.org/wiki/ArtX). 
It was originally designed to command the 
[Nintendo GameCube's **Flipper** GPU](http://en.wikipedia.org/wiki/Nintendo_Gamecube#Technical_specifications)
and later adapted for the 
[Wii's **Hollywood** GPU](http://en.wikipedia.org/wiki/Hollywood_%28graphics_chip%29).

It works via a GameCube/Wii-specific extension to PowerPC known as the *write-gather pipe*.
The write-gather pipe works by having the CPU assign a specific memory location
designated as a *pipe address*. Anything stored at this address (via conventional PPC 
store instruction) is immediately sent to the GPU via bus transaction; with no
intervention from the application code required.

Sequential write-gather stores may be performed by the application (via the GX API)
to issue drawing commands and set the GPU into its various state modes. 
These state modes represent the *GX shader architecture*.



Toolchain Extension
-------------------

PSPL's GX platform takes advantage of the write-gather pipe by performing
an invocation of the [`libogc`](http://devkitpro.org) **GX API**, *all offline.*

It does so via a specialised PSPL **Toolchain Platform Extension**. This extension
receives an instance of a *PSPL Intermediate Representation* structure that
orchestrates GX calls. The `libogc` code has been adapted to dump *write-gather-pipe-writes*
directly into a static append-buffer that's embedded in the PSPL. 

The offline GX API is used to express *vertex lighting channel colors*
and *multi-texturing (TEV) states*. The resulting bytecode sequence is packaged 
into the PSPL package.


Runtime Extension
-----------------

The PSPL **Runtime Platform Extension** simply invokes `GX_CallDispList()` which 
commands the GX hardware to *asynchronously playback* the pre-expressed state
information. It also provides an API of its own to set various rendering parameters:

* **Model Transform** via matrix input
    * Also incorporates an optional *skeletal rigging system* for performing blended bone deformations
* **View Transform** via 3 orthogonal camera-vectors
* **Projection Transform** via standard *perspective* or *orthographic* properties

Anytime a new PSPL compiled object is bound, the display list execution occurs. 
Once done, native *GX draw commands* may be issued; shader-state applied. 
