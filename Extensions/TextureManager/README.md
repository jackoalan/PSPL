PSPL Texture Manager
====================

Extends the syntax of PSPL adding an easy way to sample packaged
and externally-managed textures within a PSPL shader. 


Toolchain Extension
------------------

For textures packaged by PSPL, the toolchain has the ability to 
automatically convert an intermediate-format texture
(.PNG, .PSD, .TGA, etc...) into the appropriate format(s) for the
platform(s) being targeted. The toolchain also auto-generates mipmap
pyramids for qualifying texture images.


Runtime Extension
-----------------

The runtime is extended with the ability to load packaged textures 
in a rapid, *streaming* manner (in order of low-LOD to high-LOD). 
