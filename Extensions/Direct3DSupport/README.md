Direct3D Support
================


Toolchain Extension
-------------------

Extends the PSPL toolchain with a HLSL code-generator with vertex/fragment
lighting model synthesiser. Also (optionally) able to spawn an 
[`fxc.exe`](http://msdn.microsoft.com/en-us/library/windows/desktop/bb232919.aspx) 
child process on Windows or within [Wine](http://www.winehq.org) to generate HLSL
bytecode-tokens. The bytecode is then packaged within the PSPL package.
If `fxc.exe` is unavailable, the HLSL source-text is packaged instead.


Runtime Extension
-----------------

Extends the PSPL runtime to load bytecode compiled with `fxc.exe`.
If the source-text was packaged instead, the 
[D3DX Compiler API](http://msdn.microsoft.com/en-us/library/windows/desktop/bb172731.aspx)
is used instead at runtime.
Also adds a streaming texture loader to handle a common texture specification.
