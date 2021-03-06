################################
# TextureManager Format Coders #
################################

set(PSPL_TM_DECODER_NAME_LIST "" CACHE INTERNAL "" FORCE)
set(PSPL_TM_ENCODER_NAME_LIST "" CACHE INTERNAL "" FORCE)

set(PSPL_TM_CODER_SRCS "" CACHE INTERNAL "" FORCE)

# Macro to add decoder to texture manager
macro(pspl_tm_add_decoder format_name format_desc file_extension_list)

  list(FIND PSPL_TM_DECODER_NAME_LIST ${format_name} FMT_EXIST)
  if(FMT_EXIST GREATER -1)
    message(WARNING "Unable to add decoder '${format_name}'; already added")
  else()
    list(APPEND PSPL_TM_DECODER_NAME_LIST ${format_name})
    set(PSPL_TM_DECODER_NAME_LIST ${PSPL_TM_DECODER_NAME_LIST} CACHE INTERNAL "" FORCE)
    set(PSPL_TM_DECODER_DESC_${format_name} ${format_desc} CACHE INTERNAL "" FORCE)
    set(PSPL_TM_DECODER_EXTS_${format_name} ${file_extension_list} CACHE INTERNAL "" FORCE)
    set(src_list "")
    foreach(src ${ARGN})
      get_filename_component(srca ${src} ABSOLUTE)
      list(APPEND src_list ${srca})
    endforeach(src)
    set(PSPL_TM_CODER_SRCS "${PSPL_TM_CODER_SRCS};${src_list}" CACHE INTERNAL "" FORCE)
  endif()
  
endmacro(pspl_tm_add_decoder)


# Macro to add encoder to texture manager
macro(pspl_tm_add_encoder format_name format_desc)
  
  list(FIND PSPL_TM_ENCODER_NAME_LIST ${format_name} FMT_EXIST)
  if(FMT_EXIST GREATER -1)
    message(WARNING "Unable to add encoder '${format_name}'; already added")
  else()
    list(APPEND PSPL_TM_ENCODER_NAME_LIST ${format_name})
    set(PSPL_TM_ENCODER_NAME_LIST ${PSPL_TM_ENCODER_NAME_LIST} CACHE INTERNAL "" FORCE)
    set(PSPL_TM_ENCODER_DESC_${format_name} ${format_desc} CACHE INTERNAL "" FORCE)
    set(src_list "")
    foreach(src ${ARGN})
      get_filename_component(srca ${src} ABSOLUTE)
      list(APPEND src_list ${srca})
    endforeach(src)
    set(PSPL_TM_CODER_SRCS "${PSPL_TM_CODER_SRCS};${src_list}" CACHE INTERNAL "" FORCE)
  endif()

endmacro(pspl_tm_add_encoder)


# Include decoders and encoders
add_subdirectory(Decoders)
add_subdirectory(Encoders)


# Generate Toolchain Config Files
set(CONF_DECODER_INCLUDES "")
set(CONF_DECODER_INIT_LIST "")
set(CONF_ENCODER_INCLUDES "")
set(CONF_ENCODER_INIT_LIST "")

foreach(DEC_NAME ${PSPL_TM_DECODER_NAME_LIST})
  set(DEC_DESC ${PSPL_TM_DECODER_DESC_${DEC_NAME}})
  set(DEC_EXTS "")
  foreach(ext ${PSPL_TM_DECODER_EXTS_${DEC_NAME}})
    set(DEC_EXTS "${DEC_EXTS}\n    \"${ext}\",")
  endforeach(ext)
  configure_file(cmake/pspl_tm_dec_def.h.in ${DEC_NAME}_dec_def.h)
  set(CONF_DECODER_INCLUDES "${CONF_DECODER_INCLUDES}\n#include \"${DEC_NAME}_dec_def.h\"")
  set(CONF_DECODER_INIT_LIST "${CONF_DECODER_INIT_LIST}\n    &${DEC_NAME}_tmdec,")
endforeach(DEC_NAME)

foreach(ENC_NAME ${PSPL_TM_ENCODER_NAME_LIST})
  set(ENC_DESC ${PSPL_TM_ENCODER_DESC_${ENC_NAME}})
  configure_file(cmake/pspl_tm_enc_def.h.in ${ENC_NAME}_enc_def.h)
  set(CONF_ENCODER_INCLUDES "${CONF_ENCODER_INCLUDES}\n#include \"${ENC_NAME}_enc_def.h\"")
  set(CONF_ENCODER_INIT_LIST "${CONF_ENCODER_INIT_LIST}\n    &${ENC_NAME}_tmenc,")
endforeach(ENC_NAME)

configure_file(cmake/pspl_tm_config.c.in pspl_tm_config.c)


#######################
# TextureManager Core #
#######################

include_directories(.)
pspl_add_extension(TextureManager "Platform-independent texture conversion and integration")
pspl_add_extension_toolchain(TextureManager TMToolchain.c ${PSPL_TM_CODER_SRCS} pspl_tm_config.c)
if(PSPL_RUNTIME_PLATFORM MATCHES D3D11)
  pspl_add_extension_runtime(TextureManager TMRuntime.c TMRuntime_d3d11.cpp)
  pspl_target_link_libraries(TextureManager_runext ${DirectX11_LIBRARY} ${DirectX11_D3DCOMPILER_LIBRARY})
else()
  pspl_add_extension_runtime(TextureManager TMRuntime.c)
endif()


  