GX Support
==========


Toolchain Extension
-------------------

Extends the PSPL toolchain with an offline version of 
[libogc's](http://libogc.devkitpro.org/gx_8h.html) GX implementation to 
generate a specialised bytecode sequence. The offline GX API is used
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
