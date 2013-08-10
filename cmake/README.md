Using CMake With PSPL
=====================

The [CMake build system](http://www.cmake.org) is used in many
ways throughout PSPL. Since PSPL is designed as a modular system,
CMake is used to control the specific *platforms* and *extensions*
desired for a given PSPL build. Once built, a series of macros are
published in a CMake package. These macros may be used to build a
scalable project around PSPL. With this *bi-directional approach*, 
CMake may be leveraged by PSPL *developers* and *users* alike.


Building PSPL
-------------

Both the *toolchain* and *runtime* portions of PSPL are built in a
single CMake project. **By default, the toolchain and runtime are built
to target the machine architecture being compiled on.** CMake's
default C compiler for the platform in question is used. 

***Please Note**: Only *OS X 10.8* with *Xcode command-line tools* has been tested as a build platform

In most cases, a default `cmake <PSPL_SOURCE>` invocation followed by a run 
of `make` will build a complete PSPL toolchain/runtime pair for the host 
build machine. 


### Cross Compiling To Other Targets

So far, PSPL includes two pre-configured *cross-compiled platforms*:

#### Windows 7 and 8 (32/64 bit)

PSPL may be built for *Windows 7 and 8* against the *Direct3D 11 API* for runtime graphics.

A **Windows Cross Compile** involves invoking `cmake -DPSPL_WIN64=1 <PSPL_SOURCE>` to make
a 64-bit build of *both* the PSPL Toolchain and Runtime.

This process *requires* [MinGW-w64](http://mingw-w64.sourceforge.net) installed as a *compiler toolchain*. 
For ease of setup, I recommend [going to the downloads](http://sourceforge.net/projects/mingw-w64/files/)
and obtaining a pre-built toolchain for *your OS* 
([32-bit](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/) 
or [64-bit](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/)).
After that, extract them to `/opt/mingw-w32` and/or `/opt/mingw-w64`.

#### Wii

In the interest of demonstrating PSPL's flexibility, the PowerPC-driven, embedded-platform
known as [Wii](http://en.wikipedia.org/wiki/Wii) is also an included cross-compile.

A **Wii Cross Compile** involves invoking `cmake -DPSPL_WII=1 <PSPL_SOURCE>`.
Note that *only the runtime* is built when targeting Wii.

This process *requires* cloning and building my other project, [WiiStep](https://github.com/jackoalan/WiiStep).
Once built, WiiStep's build system will export details of its components via *CMake package*.
PSPL's build system finds this package in `~/.cmake/packages` and automatically uses it
as a cross-compiler and embedded-runtime linker.



Forming an Art Pipeline
-----------------------

PSPL integrates with CMake in a bi-directional manner. This means that any user of CMake
may call `find_package(PSPL REQUIRED)` in a CMake project and start using a series of 
macros that ease PSPL package construction effort. In turn, PSPL will provide data about
*asset-dependencies* back to CMake. This way, if a Photoshop artwork-file is referenced by a PSPL 
source-file referenced by CMake, making an edit *to the Photoshop file* will then cause the
next invocation of `make` to recompile the PSPL referencing the artwork.

The two macros are as follows:

### set_pspl_platforms(plat1 plat2 ...)

Allows the CMake-author to specify which target platforms should be included in
packages declared using `add_pspl_package`.


### add_pspl_package(package_name pspl1 pspl2 ...)

Declare a CMake target in the form of a PSPLP package-file. Provide a
*package name* (which may be treated as a CMake target later) and
any packaged *pspl sources* (with .pspl file extensions). The default `make`
target will include the PSPLP package generation, respecting dependency-updates.



Extending PSPL
--------------

PSPL uses an *extension-paradigm* allowing additional *PSPL-lexer-processing* code to be
added. So far, [preliminary runtime docs](http://jackoalan.github.io/PSPL) are in place.
Extensions are contained within the `Extensions` directory in the codebase. They
may be listed as-built by running `pspl` without arguments.

