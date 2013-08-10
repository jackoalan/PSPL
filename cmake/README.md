Using CMake With PSPL
=====================

The [CMake build system](http://www.cmake.org) is used in many
ways throughout PSPL. Since PSPL is designed as a modular system,
CMake is used to control the specific *platforms* and *extensions*
desired for a given PSPL build. Once built, a series of macros are
published in a CMake package. These macros may be used to build a
scalable project around PSPL. With this setup, CMake may be 
leveraged by PSPL *developers* and *users* alike.


Building PSPL
-------------

Both the *toolchain* and *runtime* portions of PSPL are built in a
single CMake project. **By default, the toolchain and runtime are built
to target the machine architecture being compiled on.** CMake's
default C compiler for the platform in question is used. 

***Please Note:** Only OS X 10.8 with Xcode command-line tools has been tested as a build platform

In most cases, a typical `cmake` invocation followed by a run of `make`
will correctly build PSPL. 


Forming an Art Pipeline
-----------------------




Extending PSPL
--------------

