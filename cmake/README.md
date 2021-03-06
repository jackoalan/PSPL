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

### Requirements

**Please Note** - Only *OS X 10.8* with [*Xcode command-line tools*](https://developer.apple.com/downloads) 
has been tested as a build platform.

**Building PSPL requires** a *C-compilation toolchain*, [*Git*](http://git-scm.com), and [*CMake*](http://cmake.org)

The codebase includes a **Test application** featuring
[Suzanne](http://en.wikipedia.org/wiki/Blender_%28software%29#Suzanne).
In order to compile the test data, [Blender](http://blender.org) *must*
be installed on the build machine. For *OS X*, place `blender.app` in `/Applications`.
PSPL will look there by default. Other platforms may run `export BLENDER_BIN=<PATH_TO_BLENDER_EXECUTABLE>`
to override the default `$PATH` search.

Once Blender is installed, it needs the **PMDL export addon** installed and activated. 
The addon is located in the PSPL codebase at
[`Extensions/PMDLFormat/Blender/io_pmdl_export`](https://github.com/jackoalan/PSPL/tree/master/Extensions/PMDLFormat/Blender/io_pmdl_export).
Making a *symlink* within Blender's installation is a quick way to "install" the addon.

*OS X* users may run the following:
```sh
# Absolute path to cloned PSPL source
PSPL_SOURCE=<PATH_TO_PSPL_SOURCE>

# Version number of downloaded Blender ("2.68")
BLENDER_VERSION=<BLENDER_VERSION_NUMBER>

ln -s $PSPL_SOURCE/Extensions/PMDLFormat/Blender/io_pmdl_export \
/Applications/blender.app/Contents/MacOS/$BLENDER_VERSION/scripts/addons/
```

Once the symlink is in place, launch Blender, open *User Preferences* from the *File menu*,
switch to the *Addons tab*, use the *search box* to search for "PMDL", and check the box
for *Import-Export: PMDL Model Exporter*.
**Important!!** Once the box is checked, press *Save User Settings* at the bottom of the User Preferences window.
Blender may be quit after these steps.


### Building and Installing

In most cases, a default `cmake <PSPL_SOURCE>` invocation followed by a run 
of `make` will build a complete PSPL toolchain/runtime pair for the host 
build machine. 

In practice, this looks like the following:
```sh
>$ git clone https://github.com/jackoalan/PSPL.git
>$ cd PSPL
>$ ./bootstrap.sh
>$ mkdir build
>$ cd build
>$ cmake ..
>$ make
># make install
```

Once built, the *test application* may be ran by simply calling
```sh
>$ make test
```


### Cross Compiling To Other Targets

**Cross compiling** is accessible via cached CMake options. 
**They aren't without caveats.** The builder must have the necessary
cross compiler installed. Also, *separate* CMake build directories
must be maintained for the *Toolchain* and *Runtime* each 
(assuming PSPL is also used as a cross compiler).


So far, PSPL includes two pre-configured *cross-compiled platforms*:

#### Windows 7 and 8 (32/64 bit)

PSPL may be built for *Windows 7 and 8* against the *Direct3D 11 API* for runtime graphics.

A **Windows Cross Compile** involves invoking `cmake -DPSPL_CROSS_WIN64=1 <PSPL_SOURCE>` to make
a 64-bit build of *both* the PSPL Toolchain and Runtime.

This process *requires* [MinGW-w64](http://mingw-w64.sourceforge.net) installed as a *compiler toolchain*. 
For ease of setup, I recommend [going to the downloads](http://sourceforge.net/projects/mingw-w64/files/)
and obtaining a pre-built toolchain for *your OS* 
([32-bit](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/) 
or [64-bit](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/)).
After that, extract them to `/opt/mingw-w32` and/or `/opt/mingw-w64`.

#### Wii

In the interest of demonstrating PSPL's flexibility, the PowerPC-driven, embedded-platform
known as [Wii](http://en.wikipedia.org/wiki/Wii) is also an included cross-compile. The 
GameCube/Wii *GX API* provides runtime graphics.

A **Wii Cross Compile** involves invoking `cmake -DPSPL_CROSS_WII=1 <PSPL_SOURCE>`.
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
*asset-dependencies* back to CMake. For example, if a Photoshop artwork-file is referenced by a PSPL 
source-file referenced by CMake, making an edit *to the Photoshop file* will then cause the
next invocation of `make` to recompile the PSPL referencing the artwork.


### Setting Target Graphics Platform(s)
```cmake
set_pspl_platforms(plat1 plat2 ...)
```

Allows the CMake-author to specify which target platforms should be included in
packages declared using `add_pspl_package`. So far, `GL2`, `GX`, and `D3D11` are
valid platforms.


### Adding PSPL Package Targets
```cmake
add_pspl_package(package_name pspl1 pspl2 ...)
```

Declare a CMake target in the form of a PSPLP package-file. Provide a
*package name* (which may be treated as a CMake target later) and
any packaged *pspl sources* (with .pspl file extensions). The default `make`
target will include the PSPLP package generation, respecting dependency-updates.



Extending PSPL
--------------

PSPL uses an *extension-paradigm* allowing additional *PSPL-lexer-processing* code to be
added. So far, [preliminary runtime docs](http://jackoalan.github.io/PSPL) are in place.
Extensions are contained within the 
[`Extensions`](https://github.com/jackoalan/PSPL/tree/master/Extensions) 
directory in the codebase. They
may be listed as-built by running `pspl` without arguments.

