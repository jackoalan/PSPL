
# Ensure typefile for *specified runtime platform only* is included in runtime sources
list(GET pspl_platform_typefile_list ${runtime_idx} CONF_RUNTIME_PLATFORM_TYPEFILE)
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/pspl_runtime_platform_typefile.h.in
               pspl_runtime_platform_typefile.h)
               
