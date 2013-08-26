
# Compile test program
if (PSPL_CROSS_WII)

find_package(PSPL-Wii REQUIRED)
set_pspl_platforms(GX)
pspl_add_executable(pspl-test test_wii.c)
pspl_target_link_libraries(pspl-test pspl-rt)

elseif (PSPL_CROSS_WIN32 OR PSPL_CROSS_WIN64)

find_package(PSPL-Win REQUIRED)
set_pspl_platforms(D3D11)
pspl_add_executable(pspl-test test_win.cpp)
pspl_target_link_libraries(pspl-test pspl-rt)

elseif (WIN32)

#find_package(PSPL-Native REQUIRED)
include(${PSPL_SOURCE_DIR}/cmake/PSPLArtPipelineMacros.cmake)
set_pspl_platforms(D3D11)
pspl_add_executable(pspl-test test_win.cpp)
pspl_target_link_libraries(pspl-test pspl-rt)

else()

find_package(PSPL-Native REQUIRED)
set_pspl_platforms(GL2)
pspl_add_executable(pspl-test test_mac.c)
find_library(GL_LIB OpenGL)
find_library(GLUT_LIB glut)
pspl_target_link_libraries(pspl-test pspl-rt ${GL_LIB} ${GLUT_LIB})

endif()


add_pspl_package(monkey monkey.pspl)

