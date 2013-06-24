# Copyright (c) 2013, Yan Zhou
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# The views and conclusions contained in the software and documentation are those
# of the authors and should not be interpreted as representing official policies,
# either expressed or implied, of the vSMC Project.

# Find Apple GCD support
#
# The following variable is set
#
# GCD_FOUND          - TRUE if Apple GCD is found and work correctly.
#                      But it is untested by real GCD programs
# GCD_INCLUDE_DIR    - The directory containing GCD headers
# GCD_LINK_LIBRARIES - GCD libraries that shall be linked to
#
# The following variables affect the behavior of this module
#
# GCD_INC_PATH - The path CMake shall try to find headers first
# GCD_LIB_PATH - The path CMake shall try to find libraries first

if(MAC_SDK_PATH)
  set(GCD_INC_PATH ${MAC_SDK_PATH}/usr/include)
endif()

IF (NOT DEFINED GCD_FOUND)
    IF (NOT DEFINED GCD_LINK_LIBRARIES)
        IF (APPLE)
            MESSAGE (STATUS "GCD link libraries not need (Mac OS X)")
        ELSE (APPLE)
            FIND_LIBRARY (GCD_LINK_LIBRARIES dispatch
                PATHS ${GCD_LIB_PATH} ENV LIBRARY_PATH)
            IF (GCD_LINK_LIBRARIES)
                MESSAGE (STATUS "Found GCD libraries: ${GCD_LINK_LIBRARIES}")
            ELSE (GCD_LINK_LIBRARIES)
                MESSAGE (STATUS "NOT Found GCD libraries")
            ENDIF (GCD_LINK_LIBRARIES)
        ENDIF (APPLE)
    ENDIF (NOT DEFINED GCD_LINK_LIBRARIES)

    IF (NOT DEFINED GCD_INCLUDE_DIR)
        FIND_PATH (GCD_INCLUDE_DIR dispatch/dispatch.h
            PATHS ${GCD_INC_PATH} ENV CPATH)
        IF (GCD_INCLUDE_DIR)
            MESSAGE (STATUS "Found GCD headers: ${GCD_INCLUDE_DIR}")
        ELSE (GCD_INCLUDE_DIR)
            MESSAGE (STATUS "NOT Found GCD headers")
            SET (GCD_FOUND FALSE)
        ENDIF (GCD_INCLUDE_DIR)
    ENDIF (NOT DEFINED GCD_INCLUDE_DIR)

    IF (APPLE)
        IF (GCD_INCLUDE_DIR)
            MESSAGE (STATUS "Found GCD")
            SET (GCD_FOUND TRUE CACHE BOOL "Found GCD")
        ELSE (GCD_INCLUDE_DIR)
            MESSAGE (STATUS "NOT Found GCD")
            SET (GCD_FOUND FALSE CACHE BOOL "Not Found GCD")
        ENDIF (GCD_INCLUDE_DIR)
    ELSE (APPLE)
        IF (GCD_LINK_LIBRARIES AND GCD_INCLUDE_DIR)
            MESSAGE (STATUS "Found GCD")
            SET (GCD_FOUND TRUE CACHE BOOL "Found GCD")
        ELSE (GCD_LINK_LIBRARIES AND GCD_INCLUDE_DIR)
            MESSAGE (STATUS "NOT Found GCD")
            SET (GCD_FOUND FALSE CACHE BOOL "Not Found GCD")
        ENDIF (GCD_LINK_LIBRARIES AND GCD_INCLUDE_DIR)
    ENDIF (APPLE)
ENDIF (NOT DEFINED GCD_FOUND)
