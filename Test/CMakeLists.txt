
# Compile test program
if (PSPL_WII)

find_package(PSPL-Wii REQUIRED)
set_pspl_platforms(GX)

pspl_add_executable(pspl-test test_wii.c)
pspl_target_link_libraries(pspl-test pspl-rt)

elseif (PSPL_WIN32 OR PSPL_WIN64)

find_package(PSPL-Win REQUIRED)


else ()

find_package(PSPL-Native REQUIRED)
set_pspl_platforms(GL2)

pspl_add_executable(pspl-test test_mac.c)
find_library(GL_LIB OpenGL)
find_library(GLUT_LIB glut)
pspl_target_link_libraries(pspl-test pspl-rt ${GL_LIB} ${GLUT_LIB})

endif()


add_pspl_package(monkey monkey.pspl)

