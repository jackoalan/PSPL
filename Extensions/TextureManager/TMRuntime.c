//
//  TMRuntime.c
//  PSPL
//
//  Created by Jack Andersen on 6/3/13.
//
//

#include <stdlib.h>
#include <PSPLExtension.h>
#include "TMCommon.h"

/* Platform-specific definitions */
#if PSPL_RUNTIME_PLATFORM_GL2
#  ifdef __APPLE__
#    include <TargetConditionals.h>
#    if TARGET_OS_IPHONE
#      include <OpenGLES/ES2/gl.h>
#      include <OpenGLES/ES2/glext.h>
#    else
#      include <OpenGL/gl.h>
#    endif
#  else
#    include <GL/gl.h>
#  endif
#  define MAX_MIPS 13
typedef struct {
    int mip_count;
    GLuint tex_obj;
    uint8_t tex_ready;
} pspl_tm_gl_stream_tex_t;
#  define TEX_T pspl_tm_gl_stream_tex_t
#  define SUB_TEX_FORMAT_T GLenum
#elif PSPL_RUNTIME_PLATFORM_D3D11
#  include <d3d11.h>
#  include "TMRuntime_d3d11.h"
#  define MAX_MIPS 13
#  define TEX_T ID3D11ShaderResourceView*
#  define SUB_TEX_FORMAT_T enum DXGI_FORMAT
#elif PSPL_RUNTIME_PLATFORM_GX
#  include <ogc/gx.h>
#  include <ogc/cache.h>
#  define MAX_MIPS 11
#  define TEX_T GXTexObj
#  define SUB_TEX_FORMAT_T u8
#else
#  warning Building TextureManager Runtime with dummy types
#  define TEX_T void*
#endif


/* RGBA Format */
#if PSPL_RUNTIME_PLATFORM_GX
#  define RGBA_FORMAT "RGBAGX"
#else
#  define RGBA_FORMAT "RGBA"
#endif


/* Malloc context to keep map of PSPLC->texobj */
static pspl_malloc_context_t map_ctx;

/* Texture format */
enum TEX_FORMAT {
    TEXTURE_RGB = 1,
    TEXTURE_S3TC = 2,
    TEXTURE_PVRTC = 3
};

/* Map-entry type */
typedef struct {
    const pspl_runtime_psplc_t* owner;
    unsigned texture_count;
    TEX_T* texture_arr;
} pspl_tm_map_entry;

static pspl_mutex_t load_tex_st_lock;
static int init_hook(const pspl_extension_t* extension) {
    pspl_mutex_init(&load_tex_st_lock);
    pspl_malloc_context_init(&map_ctx);
    return 0;
}

static void shutdown_hook() {
    pspl_mutex_destroy(&load_tex_st_lock);
    pspl_malloc_context_destroy(&map_ctx);
}


/* Recursive info */
typedef struct {
    pspl_tm_map_entry* fill_struct;
    unsigned key;
    unsigned chan_count;
    enum TEX_FORMAT format;
    SUB_TEX_FORMAT_T sub_fmt;
} pspl_tm_recurse_t;

#if PSPL_RUNTIME_PLATFORM_GL2

/* Reaches out to smallest mip, binds and loads back up to largest
 * Simple means to implement DMA-enabled streamed texure loading */
static void recursive_mip_load(pspl_tm_recurse_t* info,
                               int mip_c, int i, unsigned width, unsigned height,
                               void* data_off) {
    
    // Data size of this mip
    size_t dsize = 0;
    if (info->format == TEXTURE_RGB)
        dsize = width * height * info->chan_count;
    else if (info->format == TEXTURE_S3TC) {
#       if GL_EXT_texture_compression_s3tc
            unsigned c_width = (width < 4)?4:width;
            unsigned c_height = (height < 4)?4:height;
            if (info->sub_fmt == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
                dsize = c_width * c_height / 2;
            else if (info->sub_fmt == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
                dsize = c_width * c_height;
            else if (info->sub_fmt == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
                dsize = c_width * c_height;
#       endif
    } else if (info->format == TEXTURE_PVRTC) {
#       if GL_IMG_texture_compression_pvrtc
            unsigned c_width = (width < 4)?4:width;
            unsigned c_height = (height < 4)?4:height;
            if (info->sub_fmt == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG)
                dsize = c_width * c_height / 2;
            else if (info->sub_fmt == GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
                dsize = c_width * c_height / 4;
#       endif
    }
    
    
    // Load into OpenGL
    // (`data_off` is either pointer or offset depending if PBOs are used)
    if (info->format == TEXTURE_RGB)
        glTexImage2D(GL_TEXTURE_2D, i, info->sub_fmt, width, height, 0, info->sub_fmt, GL_UNSIGNED_BYTE, data_off);
    else if (info->format == TEXTURE_S3TC)
        glCompressedTexImage2D(GL_TEXTURE_2D, i, info->sub_fmt, width, height, 0, (GLsizei)dsize, data_off);
    
    
    // Reach out to smallest
    if (mip_c)
        recursive_mip_load(info, mip_c-1, i+1,
                           (width == 1)?1:width/2, (height == 1)?1:height/2,
                           data_off + dsize);
    
}

#endif

static int load_enumerate(pspl_data_object_t* obj, uint32_t key, pspl_tm_map_entry* fill_struct) {
    
    // Ready texture file
    const pspl_hash* tex_file_hash = (pspl_hash*)obj->object_data;
    const pspl_runtime_arc_file_t* file = pspl_runtime_get_archived_file_from_hash(fill_struct->owner->parent, tex_file_hash, 0);
    if (!file) {
        char hash[PSPL_HASH_STRING_LEN];
        pspl_hash_fmt(hash, tex_file_hash);
        pspl_error(-1, "Unable to access texture",
                   "TextureManager unable to access texture with data hash '%s'", hash);
    }
    const pspl_data_provider_t* provider_hooks;
    pspl_dup_data_provider_handle_t provider_handle;
    size_t length = 0;
    pspl_runtime_access_archived_file(file, &provider_hooks, &provider_handle, &length);
    size_t init_off = provider_hooks->tell(provider_handle);
    
    // Load texture metadata
    uint8_t meta_buf[256];
    provider_hooks->read_direct(provider_handle, 256, meta_buf);
    pspl_tm_texture_head_t* tex_head = (pspl_tm_texture_head_t*)&meta_buf[0];
    const char* tex_type = (char*)(meta_buf + sizeof(pspl_tm_texture_head_t));
    
    // Determine texture type
    enum TEX_FORMAT format = 0;
    if (!strcmp(tex_type, RGBA_FORMAT))
        format = TEXTURE_RGB;
    else if (!strcmp(tex_type, "S3TC")) {
        format = TEXTURE_S3TC;
#       if PSPL_RUNTIME_PLATFORM_GL2 && !GL_EXT_texture_compression_s3tc
            pspl_warn("Unsupported texture format",
                      "S3TC textures not supported by this OpenGL platform");
            return 0;
#       endif
    } else if (!strcmp(tex_type, "PVRTC")) {
        format = TEXTURE_PVRTC;
#       if PSPL_RUNTIME_PLATFORM_GL2 && !GL_IMG_texture_compression_pvrtc
            pspl_warn("Unsupported texture format",
                      "PVRTC textures not supported by this OpenGL platform");
            return 0;
#       endif
    } else
        pspl_error(-1, "Unsupported texture format",
                   "this build of TextureManager doesn't support '%s' textures", tex_type);
    
    // Validate mip count
    if (tex_head->num_mips > MAX_MIPS)
        pspl_error(-1, "Unsupported mip-mapped texture",
                   "maximum of %d mips supported; this texture has %d",
                   MAX_MIPS, tex_head->num_mips);
    
    // Prepare streamed load
    pspl_tm_recurse_t recurse_info = {
        .fill_struct = fill_struct,
        .key = key,
        .chan_count = tex_head->chan_count,
        .format = format
    };
    
    // Resolve sub format
#   if PSPL_RUNTIME_PLATFORM_GL2
        if (format == TEXTURE_RGB) {
            switch (tex_head->chan_count) {
                case 1:
                    recurse_info.sub_fmt = GL_LUMINANCE;
                    break;
                case 2:
                    recurse_info.sub_fmt = GL_LUMINANCE_ALPHA;
                    break;
                case 3:
                    recurse_info.sub_fmt = GL_RGB;
                    break;
                case 4:
                    recurse_info.sub_fmt = GL_RGBA;
                    break;
                default:
                    pspl_error(-1, "Unsupported texture format",
                               "unable to init RGB texture with %u channel count",
                               tex_head->chan_count);
                    break;
            }
        } else if (format == TEXTURE_S3TC) {
#           if GL_EXT_texture_compression_s3tc
                switch (tex_head->chan_count) {
                    case 1:
                        recurse_info.sub_fmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                        break;
                    case 3:
                        recurse_info.sub_fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                        break;
                    case 5:
                        recurse_info.sub_fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                        break;
                    default:
                        pspl_error(-1, "Unsupported texture format",
                                   "unable to init S3TC texture with mode %u",
                                   tex_head->chan_count);
                        break;
                }
#           endif
        } else if (format == TEXTURE_PVRTC) {
#           if GL_IMG_texture_compression_pvrtc
                switch (tex_head->chan_count) {
                    case 4:
                        recurse_info.sub_fmt = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
                        break;
                    case 2:
                        recurse_info.sub_fmt = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
                        break;
                    default:
                        pspl_error(-1, "Unsupported texture format",
                                   "unable to init PVRTC texture with mode %u",
                                   tex_head->chan_count);
                        break;
                }
#           endif
        }
        
#   elif PSPL_RUNTIME_PLATFORM_GX
        if (tex_head->size.native.width > 1024 || tex_head->size.native.height > 1024)
            pspl_error(-1, "Unsupported texture dimensions",
                       "GX doesn't support textures larger than 1024x1024");
        if (format == TEXTURE_RGB) {
            switch (tex_head->chan_count) {
                case 1:
                    recurse_info.sub_fmt = GX_TF_I8;
                    break;
                case 2:
                    recurse_info.sub_fmt = GX_TF_IA8;
                    break;
                case 3:
                    pspl_error(-1, "Unsupported texture format",
                               "GX doesn't support 3-component RGB8 textures");
                    break;
                case 4:
                    recurse_info.sub_fmt = GX_TF_RGBA8;
                    break;
                default:
                    recurse_info.sub_fmt = 0;
                    break;
            }
        } else if (format == TEXTURE_S3TC && tex_head->chan_count == 1) {
            recurse_info.sub_fmt = GX_TF_CMPR;
        } else
            pspl_error(-1, "Unsupported texture format",
                       "GX doesn't support '%s-%d' textures", tex_type, tex_head->chan_count);
    
#   elif PSPL_RUNTIME_PLATFORM_D3D11
    if (tex_head->size.native.width > 4096 || tex_head->size.native.height > 4096)
        pspl_error(-1, "Unsupported texture dimensions",
                   "D3D11 doesn't support textures larger than 4096x4096");
    if (format == TEXTURE_RGB) {
        switch (tex_head->chan_count) {
            case 1:
                recurse_info.sub_fmt = DXGI_FORMAT_R8_UINT;
                break;
            case 2:
                recurse_info.sub_fmt = DXGI_FORMAT_R8G8_UINT;
                break;
            case 3:
                pspl_error(-1, "Unsupported texture format",
                           "D3D11 doesn't support 3-component RGB8 textures");
                break;
            case 4:
                recurse_info.sub_fmt = DXGI_FORMAT_R8G8B8A8_UINT;
                break;
            default:
                recurse_info.sub_fmt = 0;
                break;
        }
    } else if (format == TEXTURE_S3TC && tex_head->chan_count == 1) {
        switch (tex_head->chan_count) {
            case 1:
                recurse_info.sub_fmt = DXGI_FORMAT_BC1_TYPELESS;
                break;
            case 3:
                recurse_info.sub_fmt = DXGI_FORMAT_BC2_TYPELESS;
                break;
            case 5:
                recurse_info.sub_fmt = DXGI_FORMAT_BC3_TYPELESS;
                break;
            default:
                pspl_error(-1, "Unsupported texture format",
                           "unable to init S3TC texture with mode %u",
                           tex_head->chan_count);
                break;
        }
    } else
        pspl_error(-1, "Unsupported texture format",
                   "GX doesn't support '%s-%d' textures", tex_type, tex_head->chan_count);
    
#   endif
    
    
    // Perform load
    size_t image_size = length - tex_head->data_off;
    provider_hooks->seek(provider_handle, init_off + tex_head->data_off);
    
#   if PSPL_RUNTIME_PLATFORM_GL2
        union {
            GLuint pbo;
            void* tex;
        } load_data;
        glGenTextures(1, &fill_struct->texture_arr[key].tex_obj);
        fill_struct->texture_arr[key].mip_count = tex_head->num_mips;
#       if GL_ARB_pixel_buffer_object
            glGenBuffersARB(1, &load_data.pbo);
            glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, load_data.pbo);
            glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, image_size, 0, GL_STREAM_DRAW_ARB);
            GLubyte* ptr = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
            provider_hooks->read_direct(provider_handle, image_size, ptr);
            glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
            glBindTexture(GL_TEXTURE_2D, fill_struct->texture_arr[key].tex_obj);
            recursive_mip_load(&recurse_info,
                               tex_head->num_mips-1, 0, tex_head->size.native.width,
                               tex_head->size.native.height, 0);
            fill_struct->texture_arr[key].tex_ready = 1;
            glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
            glDeleteBuffersARB(1, &load_data.pbo);
#       else
            load_data.tex = malloc(image_size);
            provider_hooks->read_direct(provider_handle, image_size, load_data.tex);
            glBindTexture(GL_TEXTURE_2D, fill_struct->texture_arr[key].tex_obj);
            recursive_mip_load(&recurse_info,
                               tex_head->num_mips-1, 0, tex_head->size.native.width,
                               tex_head->size.native.height, load_data.tex);
            fill_struct->texture_arr[key].tex_ready = 1;
            free(load_data.tex);
#       endif
    
#   elif PSPL_RUNTIME_PLATFORM_GX
        void* tex_data = pspl_allocate_media_block(image_size);
        provider_hooks->read_direct(provider_handle, image_size, tex_data);
        DCStoreRange(tex_data, image_size);
        GX_InitTexObj(&fill_struct->texture_arr[key], tex_data,
                      tex_head->size.native.width, tex_head->size.native.height,
                      recurse_info.sub_fmt, GX_REPEAT, GX_REPEAT, (tex_head->num_mips > 1)?GX_TRUE:GX_FALSE);
        GX_InitTexObjUserData(&fill_struct->texture_arr[key], tex_data);
        //GX_LoadTexObj(&fill_struct->texture_arr[key], GX_TEXMAP0);
        //GX_InvalidateTexAll();
    
#   elif PSPL_RUNTIME_PLATFORM_D3D11
        BYTE* tex_buf = malloc(image_size);
        provider_hooks->read_direct(provider_handle, image_size, tex_buf);
        D3D11_TEXTURE2D_DESC desc = {
            .Width = tex_head->size.native.width,
            .Height = tex_head->size.native.height,
            .MipLevels = tex_head->num_mips,
            .ArraySize = 1,
            .Format = recurse_info.sub_fmt,
            .SampleDesc.Count = 1,
            .SampleDesc.Quality = 0,
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
            .CPUAccessFlags = 0,
            .MiscFlags = 0
        };
        fill_struct->texture_arr[key] = pspl_d3d11_create_texture(&desc, tex_buf);
        free(tex_buf);
    
#   endif
    
    pspl_runtime_unaccess_archived_file(provider_hooks, &provider_handle);
    
    return 0;
}

/* Multi-threading context switch */
#if PSPL_RUNTIME_PLATFORM_GL2
    extern void gl_set_load_context();
#endif

static struct load_tex_st {
    pspl_runtime_psplc_t* object;
    pspl_tm_map_entry* ent;
} load_tex_st;
static void load_texture_thread(void* null) {
    struct load_tex_st load_tex_st_local = {
        .object = load_tex_st.object,
        .ent = load_tex_st.ent
    };
#   if PSPL_RUNTIME_PLATFORM_GL2
        gl_set_load_context();
#   endif
    pspl_runtime_enumerate_integer_embedded_data_objects(load_tex_st_local.object,
                                                         (pspl_integer_enumerate_hook)load_enumerate,
                                                         load_tex_st_local.ent);
    pspl_mutex_unlock(&load_tex_st_lock);
}

static void load_object(pspl_runtime_psplc_t* object) {
    int tex_c = pspl_runtime_count_integer_embedded_data_objects(object);
    pspl_tm_map_entry* ent = pspl_malloc(&map_ctx, sizeof(pspl_tm_map_entry));
    ent->owner = object;
    ent->texture_count = tex_c;
    ent->texture_arr = calloc(tex_c, sizeof(TEX_T));
    memset(ent->texture_arr, 0, sizeof(TEX_T) * tex_c);
    pspl_mutex_lock(&load_tex_st_lock);
    load_tex_st.object = object;
    load_tex_st.ent = ent;
#   if PSPL_RUNTIME_PLATFORM_GX
        load_texture_thread(NULL);
#   else
        pspl_thread_fork(load_texture_thread, NULL);
#   endif
}

static void unload_object(pspl_runtime_psplc_t* object) {
    int i,j;
    for (i=0 ; i<map_ctx.object_num ; ++i) {
        pspl_tm_map_entry* ent = map_ctx.object_arr[i];
        if (ent->owner == object) {
            // Destroy platform objects
            for (j=0 ; j<ent->texture_count ; ++j) {
#               if PSPL_RUNTIME_PLATFORM_GL2
                    glDeleteTextures(1, &ent->texture_arr[j].tex_obj);
                
#               elif PSPL_RUNTIME_PLATFORM_GX
                    void* tex_data = GX_GetTexObjUserData(&ent->texture_arr[j]);
                    pspl_free_media_block(tex_data);
                
#               elif PSPL_RUNTIME_PLATFORM_D3D11
                    pspl_d3d11_destroy_texture(ent->texture_arr[j]);
                
#               endif
            }
            free(ent->texture_arr);
            pspl_malloc_free(&map_ctx, ent);
            break;
        }
    }
}

static void bind_object(pspl_runtime_psplc_t* object) {
    int i,j;
    for (i=0 ; i<map_ctx.object_num ; ++i) {
        pspl_tm_map_entry* ent = map_ctx.object_arr[i];
        if (ent->owner == object) {
            // Bind platform objects
#           if PSPL_RUNTIME_PLATFORM_D3D11
                pspl_d3d11_bind_texture_array(ent->texture_arr, ent->texture_count);
#           else
                for (j=0 ; j<ent->texture_count ; ++j) {
#                   if PSPL_RUNTIME_PLATFORM_GL2
                        glActiveTexture(GL_TEXTURE0+j);
                        glBindTexture(GL_TEXTURE_2D, ent->texture_arr[j].tex_ready?ent->texture_arr[j].tex_obj:0);
                        if (ent->texture_arr[j].mip_count > 1)
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                    
#                   elif PSPL_RUNTIME_PLATFORM_GX
                        GX_LoadTexObj(&ent->texture_arr[j], GX_TEXMAP0+j);

#                   endif
                }
#           endif
            
#           if PSPL_RUNTIME_PLATFORM_GX
                GX_InvalidateTexAll();
#           endif
            
            break;
        }
    }
}

pspl_runtime_extension_t TextureManager_runext = {
    .init_hook = init_hook,
    .shutdown_hook = shutdown_hook,
    .load_object_hook = load_object,
    .unload_object_hook = unload_object,
    .bind_object_hook = bind_object
};
