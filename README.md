PSPL Shader Language
====================

The **Photon Shader Preprocess Language (PSPL)** aims to simplify the task 
of designing and authoring 
[shader programs](http://en.wikipedia.org/wiki/Shader) for a wide variety of 
[*3D polygon-rasterising*](http://en.wikipedia.org/wiki/Rasterisation) 
graphics architectures; with a solid degree of control over the entire 
[graphics pipeline](http://en.wikipedia.org/wiki/Graphics_pipeline). 
Additionally, PSPL defines a complete set of conventions to build an 
[art pipeline](http://en.wikipedia.org/wiki/Art_pipeline); bridging the gap
between 2D/3D graphics editors and the target application's runtime.

It accomplishes this with an extended 
[Markdown-syntax](http://en.wikipedia.org/wiki/Markdown) *macro language* 
combined with a design-oriented suite of *file formats*, *runtime libraries* 
and an *offline toolchain*. 

The Markdown *variable* and *command* extensions take design cues from 
[CMake's listcode syntax](http://www.cmake.org/cmake/help/cmake_tutorial.html).


Simplicity Meets Power
----------------------

As a document-definition format, 
[Markdown](http://daringfireball.net/projects/markdown/) works remarkably 
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


Art Pipeline Toolchain
----------------------

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
    * Preprocessor directives
    * Commands
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
    * Simple file or membuf `.psplc` loader
    * Heading-context hook installation/configuration
    * Command hook installation/configuration
    * Common uniform-variable interface
    * Common vertex format definition interface
```


Design-Focused Pipeline
-----------------------

Given the [art pipeline](http://en.wikipedia.org/wiki/Art_pipeline) paradigm
that many digital artists follow, PSPL is designed to serve as an integral 
tool; assisting these procedural tasks. The focus on an *art pipeline* paradigm
is realised in five stages:

* **Intermediate** – Filesystem-based means of gathering `.pspl` sources and intermediate assets (models, textures, etc...) into a `.pspli` root directory
* **Preprocessing** – Token-based substitution and external inclusion (think C preprocessor) for `.pspl` sources
* **Compiling** – Conversion to platform native data formats (including shader code generation and asset conversion)
* **Packaging** – Gathering of compiled shader objects and native flat-file assets into a `.psplc` package file for runtime
* **Runtime Playback** – Loading of `.psplc` and processing into the platform API in question, all in one efficient stage 

Everything in PSPL revolves around the *Markdown-extended* **PSPL** language 
(sources have `.pspl` file extension). 
The language provides *multi-level context headings*, *preprocessor directives*, 
*runtime commands*, *line-based read-ins* and an optional *indentation-sensitive syntax* 
(for some contexts).

Of course, commenting is also supported with C-style `// ...` or `/* ... */` syntax.

Here's a sample PSPL shader illustrating these features:

```pspl
/* Preprocessor directive invocations use a multi-token statement syntax 
 * encapsulated in square brackets. They are resolved at *compile-time*. 
 * This includes the contents of 'common.pspl' at the root of the same package
 * as this .pspl file. */
[INCLUDE PACKAGED common.pspl]

/* Command invocations are made with C-style function-call syntax.
 * They are resolved at *run-time* and may be dynamically (re)defined by the 
 * application integrating the PSPL runtime. 
 *
 * This particular command is a runtime built-in and will trigger the 
 * runtime to print a string to `stderr` at load-time */
PSPL_LOAD_MESSAGE("Hello World!\n")



/* So far, we've been operating in the *GLOBAL* context. In order to route 
 * commands and code-lines to the correct extensions of the PSPL runtime, 
 * a multi-level hierarchical context-system is used. The context levels
 * are invoked with the standard Markdown heading syntax of the appropriate level
 * (either line-prepending '#'s or post-line-break '='s or '-'s). 
 *
 * Here we switch to the VERTEX shader component. */
VERTEX
======

/* Each attribute of the VERTEX shader may be defined with secondary headings.
 * Here's the POSITION attribute */
POSITION  
--------
/* PSPL's compiler interface provides a line-by-line read-in convention.
 * For the POSITION context, each line defines a link in a matrix-multiplication
 * chain. Links are delimited by lines of '*' indicating a matrix-multiplication
 * stage. This allows the shader author to easily define a custom vertex
 * transformation chain for the shader. */
@VERT_POSITION
**************
$APP_MODELVIEW_MTX
******************
$APP_PROJ_MTX


/* Now let's take a closer look at how PSPL *variables* work. As the POSITION
 * context demonstrated, named tokens are provided to clearly identify each 
 * matrix-role in the chain. A key thing to note is the *punctuation-prefix*
 * for each varible name. 
 *
 * The '@' prefix indicates that the variable value 
 * is sourced from an 'attribute' within the vertex buffer of the 3D model being 
 * rendered. 
 *
 * The '$' prefix indicates the variable value is sourced from a 
 * 'uniform' provision made via the PSPL runtime. 
 * 
 * There is also the '%' prefix indicating that the
 * variable value is dynamic within the PSPL source (perhaps across contexts
 * and/or interpolated between shader components). 
 *
 * The NORMAL context uses the same sort of conventions. */
NORMAL
------
@VERT_NORMAL
************
$APP_MODELVIEW_INVXPOSE_MTX

/* So far, all headings presented have been simple identifiers indicating
 * the context being operated in. There is an additional syntax feature: *heading arguments*.
 * With heading arguments, it becomes possible to denote a secondary level of 
 * hierarchy within context switches. 
 * 
 * Heading arguments may also be used to stipulate
 * the nature of that context's inclusion in the overall shader. 
 * In this case, a new dynamic variable is defined which generates UV-coordinates
 * at the vertex stage of the shader, which the GPU will interpolate over to the
 * fragment stage. */
TEXCOORD (%STATIC_LIGHT_MAP_UV)
-------------------------------
@VERT_UV1

/* Here's another texcoord definition, this time including a 
 * runtime-provided, animated matrix-uniform multiplication. */
TEXCOORD (%SURFACE_MAP_UV)
--------------------------
@VERT_UV2
*********
$APP_SURFACE_ANIMATION_MTX



/* Woo! Vertex Pipeline Complete. Now, if we are to follow the conventional
 * order of the graphics pipeline, our Z-buffer (depth) configuration comes next
 * (the beginning of the fragment pipeline).
 *
 * Please note that standard PSPL is not at all picky about context orderings 
 * within the file; the fragment definition may even occur before the vertex 
 * definition. */
DEPTH
=====
/* The following are runtime commands (another PSPL syntactic feature).
 * These will invoke the proper platform-specific APIs to set the
 * general design-feature described. APIs like this *depth* one take 
 * a boolean argument (TRUE/FALSE, YES/NO, ON/OFF, <*>/0) to enable/disable 
 * the GPU feature. 
 * 
 * In cases like this, the shader author may also provide 
 * the 'PLATFORM' value, which uses the platform-specific default setting
 * and is PSPL's default behaviour for these commands in the event of 
 * the author's non-specification. */
PSPL_DEPTH_TEST(TRUE)
PSPL_DEPTH_WRITE(TRUE)



/* Now the depth context has decided whether or not to run the code generating
 * an RGBA colour fragment for presenting somewhere within the drawing framebuffer. */
FRAGMENT
========

/* Our COLOUR (or COLOR) sub-context is a line-by-line, top-to-bottom 
 * Photoshop-layer-style blend sequence. 
 *
 * This example employs both a 
 * Multiply blending stage (for a static shadow and lighting map 
 * rendered offline) and a Linear Dodge (or Add) blending stage (for
 * an emission [self illumination] map on the surface of the model).
 *
 * The Multiply stage is denoted with a line of '*' and the Linear Dodge '+' */
COLOUR
------
[SAMPLE PACKAGED ModelLightingMap.png %STATIC_LIGHT_MAP_UV]
***********************************************************
[SAMPLE PACKAGED ModelSurfaceDiffuseMap.png %SURFACE_MAP_UV]
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[SAMPLE PACKAGED ModelSurfaceEmissionMap.png %SURFACE_MAP_UV]



/* This surface is entirely opaque, so ensure the graphics architecture treats
 * it as such. The BLEND context controls the fragment's inclusion into the draw buffer.*/
BLEND
=====
PSPL_ALPHA_BLEND(FALSE)
```

Now, here is it without comments:

```pspl
[INCLUDE PACKAGED common.pspl]

PSPL_LOAD_MESSAGE("Hello World!\n")



VERTEX
======

POSITION  
--------
@VERT_POSITION
**************
$APP_MODELVIEW_MTX
******************
$APP_PROJ_MTX

NORMAL
------
@VERT_NORMAL
************
$APP_MODELVIEW_INVXPOSE_MTX

TEXCOORD (%STATIC_LIGHT_MAP_UV)
-------------------------------
@VERT_UV1

TEXCOORD (%SURFACE_MAP_UV)
--------------------------
@VERT_UV2
*********
$APP_SURFACE_ANIMATION_MTX



DEPTH
=====
PSPL_DEPTH_TEST(TRUE)
PSPL_DEPTH_WRITE(TRUE)



FRAGMENT
========

COLOUR
------
[SAMPLE PACKAGED ModelLightingMap.png %STATIC_LIGHT_MAP_UV]
***********************************************************
[SAMPLE PACKAGED ModelSurfaceDiffuseMap.png %SURFACE_MAP_UV]
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
[SAMPLE PACKAGED ModelSurfaceEmissionMap.png %SURFACE_MAP_UV]



BLEND
=====
PSPL_ALPHA_BLEND(FALSE)
```

Part of PSPL's **offline toolchain** includes a *compiler* and a *packager*. 

The **compiler**
performs any preprocessor invocations. It then generates file-buffers containing 
platform-specific, generated shader-sources (also compiled if able). Finally,
it generates a PSPL-specific binary configuration file `.psplb` instructing the runtime
how to set itself up against the platform graphics API (using author-specified commands).
The compiler will add a set of platform-specific directories within the *intermediate directory*
containing the binary `.psplb` files. The compiler will then perform asset
conversion and place the converted assets alongside the binaries. 

Once the `.pspli` intermediate directory is established, it may be ran through
the **packager**. The packager links all `.psplb` files into a monolithic object-buffer 
for the runtime and archives platform-specific assets together. The output is a single
`.psplc` compiled-flat-file, containing the entire shader package.


That And The Kitchen Sink
-------------------------

Since the runtime has direct, non-volatile access to texture data, it may perform
its own *streamed texture loading* as the app requests shader loads via the runtime.
Also, the *packager* may be integrated at this stage, bypassing offline packaging


It Goes Anywhere
----------------

When all said and done, shader packages written and deployed using PSPL will function 
on many fully-programmable and fixed-function graphics architectures alike. 
Fully-programmable shader archtectures with APIs like 
[OpenGL&#91;ES&#93; 2.0](http://www.khronos.org/opengles/sdk/docs/man/) and 
[Direct3D 9](http://msdn.microsoft.com/en-us/library/windows/desktop/bb174336.aspx) 
function by having native shader code ([GLSL](http://en.wikipedia.org/wiki/GLSL), 
[HLSL](http://en.wikipedia.org/wiki/High-level_shader_language), etc...) 
emitted by PSPL's *preprocessor*. Certain fixed-function architectures,
like the [GX API](http://libogc.devkitpro.org/gx_8h.html) on GameCube/Wii 
function by having their binary configuration information saved in a file 
buffer, which is replayed into the API by PSPL's *runtime library*.

The **PSPL runtime** is written in portable C (except C++ for the Direct3D stuff)
and has a configurable CMake build system to seamlessly integrate with the app's 
graphics API of choice.


Flexible Integrability
----------------------

PSPL is designed to be adaptable beyond what it brings built-in.
Headings (and sub-headings) are able to trigger routines at the *preprocessing*, 
*compiling*, and *runtime playback* stages. Commands also exist to programmatically
invoke processing routines during the same stages. It's also possible to provide 
syntactic access to more esoteric platform-specific features not 
