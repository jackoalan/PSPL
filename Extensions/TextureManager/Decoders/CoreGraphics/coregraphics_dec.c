//
//  coregraphics_dec.c
//  PSPL
//
//  Created by Jack Andersen on 9/7/13.
//
//

#include <stdio.h>

#ifdef __APPLE__
#  include <CoreGraphics/CoreGraphics.h>
#  include <ImageIO/ImageIO.h>
#endif

#include "../../TMToolchain.h"
#include <PSPLExtension.h>

int CoreGraphics_decode(const char* file_path, const char* file_path_ext,
                        pspl_tm_image_t* image_out) {
    
#   ifdef __APPLE__
    
    /* On OS X, Core Graphics provides the necessary image decoding API */
     
    CGDataProviderRef image_file_provider = CGDataProviderCreateWithFilename(file_path);
    
    CGImageSourceRef image_source = CGImageSourceCreateWithDataProvider(image_file_provider, NULL);
    CGImageRef image = CGImageSourceCreateImageAtIndex(image_source, 0, NULL);
    
    CGImageAlphaInfo alpha_info = CGImageGetAlphaInfo(image);
    image_out->image_type = (alpha_info == kCGImageAlphaNone) ? PSPL_TM_IMAGE_RGB : PSPL_TM_IMAGE_RGBA;
    image_out->width = (unsigned int)CGImageGetWidth(image);
    image_out->height = (unsigned int)CGImageGetHeight(image);
    image_out->image_buffer = malloc(image_out->width * image_out->height * image_out->image_type);
    image_out->index_buffer = NULL;
    
    CGImageAlphaInfo new_alpha_info = (image_out->image_type == PSPL_TM_IMAGE_RGBA) ?
                                      kCGImageAlphaPremultipliedLast : kCGImageAlphaNone;
    CGContextRef quartz_context = CGBitmapContextCreate((void*)image_out->image_buffer, image_out->width,
                                                        image_out->height, 8, image_out->width * image_out->image_type,
                                                        CGColorSpaceCreateDeviceRGB(), new_alpha_info);
    CGContextDrawImage(quartz_context, CGRectMake(0, 0, image_out->width, image_out->height), image);
    
    CGContextRelease(quartz_context);
    CGImageRelease(image);
    CFRelease(image_source);
    CGDataProviderRelease(image_file_provider);

    
#   endif
    
    return 0;
    
}
