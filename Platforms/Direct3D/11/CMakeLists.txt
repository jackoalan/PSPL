pspl_add_platform(D3D11 "Direct3D 11 API for Microsoft platforms")
pspl_add_platform_toolchain(D3D11 d3d11_toolchain.c)
pspl_add_platform_runtime(D3D11 d3d11_runtime.c d3d11_runtime_interface.cpp)