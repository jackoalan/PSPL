# Cross-compiler toolchain definition for compiling *on* OSX *for* Windows
cmake_minimum_required(VERSION 2.8)

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

# MinGW toolchain path
if(PSPL_WIN32)
  set(MINGW_PATH /opt/mingw-w32)
  set(MINGW_PREFIX i686-w64-mingw32)
elseif(PSPL_WIN64)
  set(MINGW_PATH /opt/mingw-w64)
  set(MINGW_PREFIX x86_64-w64-mingw32)
endif()

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER ${MINGW_PATH}/bin/${MINGW_PREFIX}-gcc)
SET(CMAKE_CXX_COMPILER ${MINGW_PATH}/bin/${MINGW_PREFIX}-g++)
SET(CMAKE_RC_COMPILER ${MINGW_PATH}/bin/${MINGW_PREFIX}-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH ${MINGW_PATH}/${MINGW_PREFIX})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Set target architecture accordingly
if(PSPL_WIN32)
  add_definitions(-m32)
  set(CMAKE_EXE_LINKER_FLAGS -m32)
elseif(PSPL_WIN64)
  add_definitions(-m64)
  set(CMAKE_EXE_LINKER_FLAGS -m64)
endif()

add_definitions(-D __LITTLE_ENDIAN__=1)
