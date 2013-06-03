set(PSPL_TM_DECODER_NAME_LIST "")
set(PSPL_TM_ENCODER_NAME_LIST "")

# Macro to add decoder to texture manager
macro(pspl_tm_add_decoder format_name format_desc file_extension_list)

  list(FIND PSPL_TM_DECODER_NAME_LIST ${format_name} FMT_EXIST)
  if(FMT_EXIST GREATER -1)
    message(WARNING "Unable to add decoder '${format_name}'; already added")
  else()
    list(APPEND PSPL_TM_DECODER_NAME_LIST ${format_name})
    set(PSPL_TM_DECODER_DESC_${format_name} ${format_desc})
    set(PSPL_TM_DECODER_EXTS_${format_name} ${file_extension_list})
    add_library(${format_name}_tmdec STATIC ${ARGN})
  endif()
  
endmacro(pspl_tm_add_decoder)


# Macro to add encoder to texture manager
macro(pspl_tm_add_encoder format_name format_desc)
  
  list(FIND PSPL_TM_ENCODER_NAME_LIST ${format_name} FMT_EXIST)
  if(FMT_EXIST GREATER -1)
    message(WARNING "Unable to add encoder '${format_name}'; already added")
  else()
    list(APPEND PSPL_TM_ENCODER_NAME_LIST ${format_name})
    set(PSPL_TM_ENCODER_DESC_${format_name} ${format_desc})
    add_library(${format_name}_tmenc STATIC ${ARGN})
  endif()

endmacro(pspl_tm_add_encoder)


# Include decoders and encoders
add_subdirectory(Decoders)
add_subdirectory(Encoders)
