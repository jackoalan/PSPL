pspl(1) -- toolchain driver to generate PSPL package files
==========================================================

SYNOPSIS
--------

`pspl` [`-o` _out-path_] [`-E`|`-c`] [`-G` _reflist-out-path_] [`-S` _staging-root-path_] [`-D` _def-name_[=_def-value_]]... [`-T` _target-platform_]... [`-e` _LITTLE_,_BIG_,_BI_] _source1_ [_source2_ [_sourceN_]]...

OPTIONS
-------

* `-o` _out-path_:
  Specify the file path to write `pspl` output. By default, output is written 
  to `a.out.psplp`

DESCRIPTION
-----------

The **pspl toolchain driver** is the data-generation portion of the `PSPL` software package. 
The driver is similarly designed as a typical **C-language compiler toolchain**.

As with C, there are driver modes reflecting **compilation** and **linking**. 
PSPL's toolchain has a literal **compiler stage**, but the linking process is 
known as the **packaging stage**. 


