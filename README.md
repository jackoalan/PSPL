PSPL Shader Language
====================

The **Photon Shader Preprocess Language (PSPL)** aims to simplify the task 
of designing and authoring 
[shader programs](http://en.wikipedia.org/wiki/Shader) for a wide variety of 
[*3D polygon-rasterising*](http://en.wikipedia.org/wiki/Rasterisation) 
graphics architectures; with a solid degree of control over the entire 
[graphics pipeline](http://en.wikipedia.org/wiki/Graphics_pipeline). 
It accomplishes this with an extended 
[Markdown-syntax](http://en.wikipedia.org/wiki/Markdown) *programming language* 
combined with a design-oriented suite of *file formats*, *runtime libraries* 
and offline *pre-processing tools*. 


Simplicity Meets Power
----------------------

As a document-definition format, 
[Markdown](http://daringfireball.net/projects/markdown/) works incredibly 
well in more ways than one. A document is usually authored entirely in 
plain-text, yet conveys a striking set of presentation attributes. The 
attributes are rendered through *preprocessor tools* and also
through its *plain-text perceptibility*. 

```markdown
Markdown Within Markdown (this README)
======================================

That's my primary heading up there with that line of '=' characters.


Perhaps Markdown Could Be Extended (???)
----------------------------------------

As this secondary heading ponders, the simplicity of Markdown also
indicates an easy ability to extend into other applications
(not just text formatting). 

**WARNING (bold):** the following bulleted list contains spoilers!

* Shaders
    * Graphics pipeline
        * Vertex Transformation
        * Dynamic Vertex Attributes
        * Vertex/Fragment Lighting Model(s)
        * Multi-texturing Pipeline
        * Alpha Blend Modes
        * Z-buffer Modes
    * Headings
    * Macros
    * Variables
* Offline Tools
    * Preprocessors (to other languages/configs)
    * Decompilers (from other languages/configs)
    * Packager/Unpackager (to traverse a PSPL-based art pipeline)
    * Compiler (to generate platform-native data)
    * 2D/3D Graphics Editor Integration
        * Importers
        * Exporters
        * Preview Generation
* File Formats
    * Markdown-extended source text `.pspl`
    * Intermediate PSPL package `.pspli` (directory structure)
        * May contain multiple PSPL sources
        * May contain intermediately-compressed (.PSD, .PNG, .TIF) textures
    * Compiled PSPL package `.psplc`
        * Platform-specific targeting
            * May be a "fat-package" containing multiple platforms
            * Bi-endian design
        * Textures are in platform-native format(s)
* Runtime
    * Simple file or membuf `.pspli/.psplc` loader
    * Heading handler installation/configuration
    * Macro handler installation/configuration
    * Common uniform-variable interface
    * Common vertex format definition interface
```

Given the [art pipeline](http://en.wikipedia.org/wiki/Art_pipeline) paradigm
that many digital artists follow, PSPL is designed to serve as an integral 
tool; assisting these procedural tasks.


It Goes Anywhere
----------------

When all said and done, shaders written and deployed using PSPL will function 
on many fully-programmable and fixed-function graphics architectures alike. 
Fully-programmable shader archtectures with APIs like 
[OpenGL&#91;ES&#93; 2.0](http://www.khronos.org/opengles/sdk/docs/man/) and 
[Direct3D 9](http://msdn.microsoft.com/en-us/library/windows/desktop/bb174336&#40;v=vs.85&#41;.aspx) 
function by having native shader code ([GLSL](http://en.wikipedia.org/wiki/GLSL), 
[HLSL](http://en.wikipedia.org/wiki/High-level_shader_language), etc...) 
emitted by PSPL's *preprocessor*. Certain fixed-function architectures,
like the [GX](http://libogc.devkitpro.org/gx_8h.html) API on GameCube/Wii 
function by having their binary configuration information saved in a file 
buffer, which is replayed into the API by PSPL's *runtime library*.

The **PSPL runtime** is written in portable C (except C++ for the Direct3D stuff)
and has a configurable CMake build system to seamlessly integrate with the app's 
graphics API of choice.


More Than Just A Language
-------------------------

PSPL isn't complete without its **support architecture**. 


Focus On Design
---------------

Part of PSPL's **offline toolchain** includes a packager and compiler.


Flexible Integrability
----------------------

PSPL is designed to be adaptable beyond what it brings built-in.
