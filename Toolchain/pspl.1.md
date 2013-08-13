pspl(1) -- toolchain driver to generate PSPL package files
==========================================================


SYNOPSIS
--------

`pspl` [`-o` _out-path_] [`-E`|`-c`] [`-G` _reflist-out-path_] [`-S` _staging-root-path_] [`-D` _def-name_[=_def-value_]]... [`-T` _target-platform_]... [`-e` `LITTLE`,`BIG`,`BI`] _source1_ [_source2_ [_sourceN_]]...


DESCRIPTION
-----------

The **PSPL toolchain driver** is the data-generation portion of the **PSPL software package**.
It's vaguely designed as a typical **C-language compiler toolchain**.

As with **C**, there are driver modes reflecting **compilation** and **linking**. 
PSPL's toolchain has a literal **compiler stage**, but the linking process is 
known as the **packaging stage**.

The **PSPL package** file (`*.psplp`) is the ultimate output of the `pspl` toolchain.
It contains data, converted for the target platform(s) assembled into a [fat ]package
indexing scheme.

As for input, there are two options. One or many **PSPL source** files (`*.pspl`) 
may be offered to form a complete package. Alternatively, pre-compiled **PSPL object** 
files (`*.psplc`) may substitute source files in forming the package. A PSPL object
is formed from only one PSPL source and may be output with the `-c` flag.

**To determine the capabilities** of a `pspl` toolchain build, simply run
`pspl` without arguments.


OPTIONS
-------

* `-o` _out-path_:
  Specify the file path to write `pspl` **output**. By default, output is written 
  to `a.out.pspl[p|c]`
  
* `-E`:
  Only run the **PSPL preprocessor** on _source1_ and output to _out-path_ a 
  directive-expanded **PSPL source file** (*.pspl)
  *May not be combined with `-c`*.
  
* `-c`:
  Only run the **PSPL preprocessor and compiler** on _source1_ and output to _out-path_ a
  directive-expanded and compiled **PSPL object file** (*.psplc). 
  *May not be combined with `-E`*.
  
* `-G` _reflist-out-path_:
  Instruct toolchain to additionally assemble a **CMake reference list file** (*.cmake).
  This file provides data to CMake; conveying dependencies on *packaged files* 
  passed through the compiler.
  
* `-S` _staging-root-path_:
  Specify a **staging directory path** (*working directory* by default) to put
  a single subdirectory named `PSPLFiles`. This subdirectory contains archived-file
  objects, converted for the target platform[s] during compilation; awaiting packaging.
  With `PSPLFiles`, conversion *doesn't redundantly occur* on repeated PSPL source 
  compilations.
  
* `-D` _def-name_[=_def-value_]...:
  Add a **keyed definition string** for PSPL toolchain extensions to utilise.
  
* `-T` _target-platform_...:
  Add a **target platform** for the PSPL package or object being output.
  Currently, `GL2`,`GX`,`D3D11` are the options.
  
* `-e` `LITTLE`,`BIG`,`BI`:
  Set **default endianness** for the PSPL package or object being output.
  By default, the endianness used is either *overridden by the target platform*
  or the *native-endianness of the PSPL toolchain execution*.


AUTHOR
------

Jack Andersen <jackoalan@gmail.com>


COPYRIGHT
---------

Copyright (c) 2013 Jack Andersen

View licences of **third-party code** used in `pspl` by running `pspl -l`

