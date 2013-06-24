if(NOT DEFINED FIND_MAC_SDK)
if(APPLE)
  execute_process(COMMAND xcodebuild -version -sdk ${OSX_SDK} Path
                  RESULT_VARIABLE exit_code
                  OUTPUT_VARIABLE sdk_string
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(exit_code EQUAL 0)
    set(MAC_SDK_PATH ${sdk_string} CACHE PATH "Mac SDK Path")
    set(FIND_MAC_SDK TRUE CACHE BOOL "Mac SDK Found")
    message(STATUS "Found OS X SDK: ${MAC_SDK_PATH}")
  else()
    set(FIND_MAC_SDK FALSE CACHE BOOL "Mac SDK Not Found")
    message(STATUS "Unable to find OS X SDK")
  endif()
endif()
endif()