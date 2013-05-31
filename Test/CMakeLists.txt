include(${PSPL_SOURCE_DIR}/cmake/PSPLArtPipelineMacros.cmake)

# Compile test program
add_executable(pspl-test test_mac.c)
find_library(GL_LIB gl)
find_library(GLUT_LIB glut)
target_link_libraries(pspl-test pspl-rt ${GL_LIB} ${GLUT_LIB})


# Run Art Pipeline
