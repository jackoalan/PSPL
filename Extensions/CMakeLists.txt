
# Set our extension list scope here
set(pspl_extension_name_list "" CACHE INTERNAL "Ordered extension name list, augmented by `pspl_add_extension`")
set(pspl_extension_dir_list "" CACHE INTERNAL "Ordered extension dir list, augmented by `pspl_add_extension`")
set(pspl_extension_dir_used "" CACHE INTERNAL "Used extension dir list")


# Include Extension authoring macros here
include(${PSPL_SOURCE_DIR}/cmake/PSPLExtensionMacros.cmake)


# Provided extensions
#add_subdirectory(PMDLFormat)
#add_subdirectory(OpenGLSupport)
add_subdirectory(GXSupport)
#add_subdirectory(Direct3DSupport)
#add_subdirectory(TextureManager)
#add_subdirectory(LightingManager)

# Assemble extension definitions together
list(LENGTH pspl_extension_name_list len)
if(len GREATER 0)
  math(EXPR len ${len}-1)
  foreach(extension RANGE ${len})
  
    # Extension member variables
    list(GET pspl_extension_name_list ${extension} EXT_NAME)
    list(GET pspl_extension_dir_list ${extension} EXT_DIR)
    
    # No redundant extension includes
    list(FIND pspl_extension_dir_used ${EXT_DIR} DIR_USED)
    if(DIR_USED LESS 0)
      set(CONF_EXT_DEF_OBJECTS "${CONF_EXT_DEF_OBJECTS}
#include <PSPL/Extensions/${EXT_DIR}/PSPLClasses.h>")
      list(APPEND pspl_extension_dir_used ${EXT_DIR})
      set(pspl_extension_dir_used ${pspl_extension_dir_used} CACHE INTERNAL "Used extension dir list")
    endif()

    # Augment C-array initialiser
    set(CONF_EXT_ARRAY_INIT_LIST
        "${CONF_EXT_ARRAY_INIT_LIST}
    ${EXT_NAME}_extension,")

  endforeach(extension)
endif()

# Generate the C config files
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/PSPLAvailableExtensions.h.in
               PSPLAvailableExtensions.h)
configure_file(${PSPL_SOURCE_DIR}/cmake/Templates/PSPLAvailableExtensions.c.in
               PSPLAvailableExtensions.c)
               