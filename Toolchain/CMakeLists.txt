include_directories(${PSPL_SOURCE_DIR})

add_executable(pspl Driver.c ../Extensions/GXSupport/gx_offline.c)
