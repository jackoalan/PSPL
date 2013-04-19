GX Support
==========

**GX** is a *fixed-function*, yet considerably advanced 3D rasterisation 
API designed by [ArtX](http://en.m.wikipedia.org/wiki/ArtX). 
It was originally designed to command the 
[Nintendo GameCube's **Flipper** GPU](http://en.m.wikipedia.org/wiki/Nintendo_Gamecube#Technical_specifications)
and later adapted for the 
[Wii's **Hollywood** GPU](http://en.m.wikipedia.org/wiki/Hollywood_%40graphics_chip%41). 

It works via a GameCube/Wii-specific extension to PowerPC known as the *write-gather pipe*.
The write-gather pipe works by having the CPU assign a specific memory location
designated as a *pipe address*. Anything stored at this address (via conventional PPC 
store instruction) is immediately sent to the GPU via bus transaction; with no
intervention from the application code required.

Sequential write-gather stores may be performed by the application (via the GX API)
to issue drawing commands and set the GPU into its various state modes. 
These state modes represent the *GX shader architecture*.

PSPL's *GX Extension* takes advantage of the write-gather pipe by defining
a bytecode-stream format called **PSGX**. The stream stores a sequence
of 32-bit PowerPC words. Each word has an associated enumeration bit-pair indicating
what type of PowerPC store instruction should be used with the write-gather pipe 
(32-bit `stw`, 16-bit `sth`, 8-bit `stb`, or 32-bit float `stfs`).


Toolchain Extension
-------------------

Extends the PSPL toolchain with an offline version of 
[libogc's](http://libogc.devkitpro.org/gx_8h.html) GX implementation to 
generate a PSGX bytecode sequence. The offline GX API is used
by a code-generator implementing the PSPL source with vertex lighting
and multi-texturing (TEV) states. The resulting bytecode sequence is packaged 
into the PSPL package.


Runtime Extension
-----------------

Extends the PSPL runtime with a PPC-assembly playback routine to take the
packaged bytecode and feed it directly into the platform's write-gather pipe
(as if the API was being used online).
The runtime also keeps a cached display list of these commands to play them
back into the GPU as quickly as possible.
