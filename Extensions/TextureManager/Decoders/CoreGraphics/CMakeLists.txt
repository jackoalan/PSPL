if(APPLE)
  pspl_tm_add_decoder(CoreGraphics "Native OS X Image Decoding API" "PNG;JPG;JPEG;TGA;TIF;TIFF" coregraphics_dec.c)
endif()