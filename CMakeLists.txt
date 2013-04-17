cmake_minimum_required(VERSION 2.8)
project(PSPL)

# Targets to build (toolchain and/or runtime)
option(PSPL_BUILD_TOOLCHAIN "Build toolchain" ON)
option(PSPL_BUILD_RUNTIME "Build runtime" ON)

# Runtime platform options
if(${CMAKE_SYSTEM_TYPE} STREQUAL Darwin)
  option(PSPL_RUNTIME_MAC "Build the runtime for OpenGL on Mac" ON)
else()
  option(PSPL_RUNTIME_MAC "Build the runtime for OpenGL on Mac" OFF)
endif()

option(PSPL_RUNTIME_GX "Build the runtime for GX on GameCube/Wii via libogc provided through WiiStep" OFF)

option(PSPL_RUNTIME_WINDOWS_GL "Build the runtime for OpenGL on Windows" OFF)
option(PSPL_RUNTIME_WINDOWS_D3D "Build the runtime for Direct3D on Windows" OFF)


# Target subdirectories
add_subdirectory(Toolchain)
add_subdirectory(Runtime)

# Extension subdirectories
add_subdirectory(Extensions)
