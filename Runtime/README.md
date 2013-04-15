PSPL Runtime
============

The **Runtime** portion of PSPL is responsible for *loading* data
from a `.psplc` flat-file or `.psplcd` compiled-directory, *initialising*
the data in the host's graphics API and *providing a binding mechanism*
for the application's rendering code to use.


Runtime Design
--------------

PSPL's runtime is designed with *portability* and *extensibility* in mind. 
Therefore, it is written in pure C with a C-struct-inheritance mechanism 
to allow objects and extensions to be expressed.


### Object Oriented

An instance of the PSPL runtime uses a parent-pointer-linked chain of 
common C structures to establish a dependency-driven series of objects. 
The chain forms an 
[acyclic graph](http://en.wikipedia.org/wiki/Directed_acyclic_graph) 
building the code-stack necessary to put PSPL and its extension together
to run the PSPL source files written by the shader author.

The acyclic graph is rooted at the *PSPL context* and reaches out to 
the individual PSPL sources. Graphically, a complete graph may look like
the following (think of this *outline* as a *graph* instead):

* **PSPLContext_t** – Always present, even across host platforms
    * **Graphics API Context** – Platform-specific data structure maintaining root state, added by extension
        * **Texture Manager** – Detects which graphics API is present and loads a common texture specification accordingly
	* **Lighting Manager** – Synthesises the lighting portion of a shader configuration (vertex or fragment) based on a common lighting specification
	    * **Loaded PSPL Source** – An object representation of the PSPL source written by the shader author.


### Dependency Driven

A PSPL extension is required to uniquely identify itself with an 
*extension identifier string*. Extensions may declare other extensions
(or logical combinations of extensions) needing to be loaded in order
to function properly.


