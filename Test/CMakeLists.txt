set(PSPL_BINARY_LOC ${PSPL_BINARY_DIR}/Toolchain/pspl)
include(${PSPL_SOURCE_DIR}/cmake/PSPLArtPipelineMacros.cmake)

# Compile test program
if (PSPL_WII)

pspl_add_executable(pspl-test test_wii.c)
pspl_target_link_libraries(pspl-test pspl-rt)

elseif (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)

pspl_add_executable(pspl-test test_mac.c)
find_library(GL_LIB OpenGL)
find_library(GLUT_LIB glut)
pspl_target_link_libraries(pspl-test pspl-rt ${GL_LIB} ${GLUT_LIB})

endif()


# Run Art Pipeline
if (PSPL_WII)
  set_pspl_platforms(GX)
else ()
  set_pspl_platforms(GL2)
endif ()

add_pspl_package(monkey monkey.pspl)