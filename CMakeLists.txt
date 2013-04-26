cmake_minimum_required(VERSION 2.8)
project(PSPL)


# Runtime platform options
if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  set(PSPL_RUNTIME_PLATFORM MAC_GL CACHE STRING 
  "Select which platform to build runtime for [NONE, MAC_GL, EGL_GL, GLX_GL, WIN_GL, WIN_D3D, GX] (Mac OpenGL selected by default)")
else()
  set(PSPL_RUNTIME_PLATFORM NONE CACHE STRING 
  "Select which platform to build runtime for [NONE, MAC_GL, EGL_GL, GLX_GL, WIN_GL, WIN_D3D, GX] (Runtime not built by default)")
endif()


# Codebase `include` directory
include_directories(include)


# Target subdirectories
add_subdirectory(Toolchain)
if(NOT PSPL_RUNTIME_PLATFORM STREQUAL NONE)
  add_subdirectory(Runtime)
endif()

# Extension subdirectories
add_subdirectory(Extensions)
