PSPL Built-ins Extension
========================

To increase the out-of-the-box usefulness of PSPL, a 
**Built-ins Extension** is provided with the core codebase.

Portions of the PSPL core codebase interact with this extension's codebase
directly (and vice-versa); so its presence is required to form a functional 
PSPL system.

All functionality of the built-ins extension is completely 
*cross-platform* and may be used to extend the PSPL language itself.


Built-in Preprocessor Directives
--------------------------------

### Preprocessor Messaging

For large projects and development testing, PSPL offers a built-in set of
preprocessor directives to get feedback from the preprocessor directly.

To issue informative messages to the toolchain user, a PSPL source may 
add the `MESSAGE` directive. The message directive concatenates all provided
preprocessor tokens together, converts everything to string-form, then
prints the result to `stderr`:

```
[MESSAGE "Hello World! \n\t-PSPL Preprocessor"]
```

Additionally, messages that indicate *warning* or *error* conditions may be
specially issued with the `WARN` or `ERROR` directives. The error directive
will stop the PSPL toolchain driver entirely:

```
[WARN "An error is about to occur"]
[ERROR "Oh noes!!"]
[MESSAGE "This message won't be seen"]
```


### Preprocessor Inclusion

Large projects with repeating design patterns may want to simplify
their shader codebase with repeadedly-accessed code-fragments.

Once the repeated code-fragment is authored and saved into a PSPL source
file, it may be referenced and included across multiple source files using
the `INCLUDE` directive:

```
[INCLUDE "helper_code.pspl"]
```


### Preprocessor Variable Define

PSPL's *variable architecture* includes a *preprocessor-variable namespace*.
Variables in this namespace are prepended with a '#' to this effect: 
`#PREPROCESSOR_VARIABLE_NAME`. 

Normally, preprocessor variables are set using the `-D` flag in the toolchain,
but it's also possible to (re)define (and undefine) variables in this namespace
with the `DEFINE` directive:


```
/* Define: */
[DEFINE #VARIABLE "Value String 1"]

/* Check: */
[MESSAGE "VALUE ALPHA: "#VARIABLE]
/* This prints `VALUE ALPHA: Value String 1` */


/* Redefine: */
[DEFINE #VARIABLE "Value String 2"]

/* Check: */
[MESSAGE "VALUE BETA: "#VARIABLE]
/* This prints `VALUE BETA: Value String 2` */


/* Undefine: */
[UNDEF #VARIABLE]

/* Check: */
[MESSAGE "VALUE NOTHING: "#VARIABLE]
/* This prints `VALUE NOTHING: ` */
```

Note that the '#' prefix is *always* present whenever a preprocessor
variable is being used or specified. Preprocessor variables are *never*
expanded within a *quoted string literal*.

Like other variables in PSPL, explicit declaration of a variable is *unnecessary*.
Also, variable values are *dynamically-typed*, so numeric definitions may also be made.


### Preprocessor Logic

The concept of preprocessor variables may be extended to perform
*logical comparisons* with the `IF` directive. The result of this directive
is line-by-line conditional inclusion of everything up to the next `ENDIF`
directive:

```
[IF #TRIGGER_MESSAGE == "Yes"]
[MESSAGE "I was instructed to give you this message"]
[ENDIF]
```

For variables that have been set to a numeric type, 
comparisions with C-style comparision operators 
[`<`,`>`,`<=`,`>=`,`==`,`!=`]
and logical combiners 
[`&&`,`||`,`!`]
may be performed. 

Additionally, the logical unary operator `defined()` is available to determine if
a preprocessor variable has been explicitly defined.


Built-in Commands
-----------------

### Runtime Messaging


### Primary-Heading Stack Manipulation

In certain scenarios (particularly preprocessor-inclusion) it may be useful
to *temporarily switch* the active primary heading-context in a PSPL source 
and restore it shortly thereafter. To accomplish this, PSPL provides a 
built-in command that implements the tried-and-true *context-stack* paradigm.

To save the active heading-context and switch to a different one, use the
`PSPL_PUSH_HEADING` command with the name of the primary heading. 
`PSPL_POP_HEADING` will restore the heading that was active before the previous
push-heading.

For example, to make the *FRAGMENT* context the active context, multiply in a 
colour value, and restore the previous context, do the following:

```
PSPL_PUSH_HEADING("FRAGMENT")
*********************
RGBA(1.0 0.0 0.0 1.0)
PSPL_POP_HEADING()
```

To further illustrate what's going on, say the *DEPTH* heading-context was
previously active and the above code fragment was added via *INCLUDE*.

```
DEPTH
=====
PSPL_DEPTH_WRITE(TRUE)
[INCLUDE "multiply_red.pspl"]
PSPL_DEPTH_TEST(TRUE)
```

The toolchain would expand the code out to the functional equivalent:

```
DEPTH
=====
PSPL_DEPTH_WRITE(TRUE)

FRAGMENT
========
*********************
RGBA(1.0 0.0 0.0 1.0)

DEPTH
=====
PSPL_DEPTH_TEST(TRUE)
```

Note that the second *DEPTH* heading-switch was synthesised with `PSPL_POP_HEADING`.

Also note that this is a trivial example and the author of this documentation 
*strongly advises against* this haphazard style of coding.


