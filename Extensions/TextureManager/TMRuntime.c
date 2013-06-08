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
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#  define MAX_MIPS 13
typedef struct {
    int mip_count;
    GLuint pbo_obj[MAX_MIPS];
    GLuint tex_obj;
} pspl_tm_gl_stream_tex_t;
#  define TEX_T pspl_tm_gl_stream_tex_t
#  define SUB_TEX_FORMAT_T GLenum
#elif PSPL_RUNTIME_PLATFORM_D3D9
#  include <d3d9.h>
#  define MAX_MIPS 13
#  define TEX_T IDirect3DTexture9
#  define SUB_TEX_FORMAT_T u8
#elif PSPL_RUNTIME_PLATFORM_GX
#  include <ogc/gx.h>
#  define MAX_MIPS 11
#  define TEX_T GXTexObj
#  define SUB_TEX_FORMAT_T u8
#else
#  warning Building TextureManager Runtime with dummy types
#  define TEX_T void*
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

static int init(const pspl_extension_t* extension) {
#   if PSPL_RUNTIME_PLATFORM_GL2
    const GLubyte* ext_str = glGetString(GL_EXTENSIONS);
    if (!strstr((const char*)ext_str, "GL_ARB_pixel_buffer_object")) {
        pspl_error(-1, "Incompatible OpenGL implementation",
                   "TextureManager requires the 'GL_ARB_pixel_buffer_object' extension to be available");
        return -1;
    }
#   elif PSPL_RUNTIME_PLATFORM_D3D9
    
#   endif
    pspl_malloc_context_init(&map_ctx);
    return 0;
}

static void shutdown() {
    pspl_malloc_context_destroy(&map_ctx);
}


/* Recursive info */
typedef struct {
    pspl_tm_map_entry* fill_struct;
    unsigned key;
    unsigned chan_count;
    enum TEX_FORMAT format;
    SUB_TEX_FORMAT_T sub_fmt;
    const pspl_data_provider_t* provider_hooks;
    const void* provider_handle;
} pspl_tm_recurse_t;

/* Reaches out to smallest mip, binds and loads back up to largest
 * Simple means to implement DMA-enabled streamed texure loading */
static void recursive_mip_load(pspl_tm_recurse_t* info,
                               int mip_c, int i, unsigned width, unsigned height,
                               size_t data_off) {
    
    // Data size of this mip
    size_t dsize = 0;
    if (info->format == TEXTURE_RGB)
        dsize = width * height * info->chan_count;
    else if (info->format == TEXTURE_S3TC) {
        if (info->sub_fmt == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
            dsize = width * height / 2;
        else if (info->sub_fmt == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
            dsize = width * height;
        else if (info->sub_fmt == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
            dsize = width * height;
    }
    
    // Reach out to smallest
    if (mip_c)
        recursive_mip_load(info, mip_c-1, i+1,
                           (width == 1)?1:width/2, (height == 1)?1:height/2,
                           data_off + dsize);
    
    // Seek to location in file
    info->provider_hooks->seek(info->provider_handle, data_off);

    
#   if PSPL_RUNTIME_PLATFORM_GL2
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, info->fill_struct->texture_arr[info->key].pbo_obj[i]);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, dsize, 0, GL_STREAM_DRAW_ARB);
    GLubyte* ptr = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    info->provider_hooks->read_direct(info->provider_handle, dsize, ptr);
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    
    glBindTexture(GL_TEXTURE_2D, info->fill_struct->texture_arr[info->key].tex_obj);
    if (info->format == TEXTURE_RGB)
        glTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, width, height, info->sub_fmt, GL_UNSIGNED_BYTE, 0);
    else if (info->format == TEXTURE_S3TC)
        glCompressedTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, width, height, info->sub_fmt, (GLsizei)dsize, 0);
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    
#   elif PSPL_RUNTIME_PLATFORM_D3D9
    
    
#   endif
    
}

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
    const void* provider_handle;
    size_t length = 0;
    pspl_runtime_access_archived_file(file, &provider_hooks, &provider_handle, &length);
    size_t init_off = provider_hooks->tell(provider_handle);
    
    // Load texture metadata
    uint8_t meta_buf[256];
    provider_hooks->read_direct(provider_handle, 256, meta_buf);
    pspl_tm_texture_head_t* tex_head = (pspl_tm_texture_head_t*)&meta_buf[0];
    const char* tex_type = (char*)&meta_buf[sizeof(pspl_tm_texture_head_t)];
    
    // Determine texture type
    enum TEX_FORMAT format = 0;
    if (!strcmp(tex_type, "RGB"))
        format = TEXTURE_RGB;
    else if (!strcmp(tex_type, "S3TC"))
        format = TEXTURE_S3TC;
    else if (!strcmp(tex_type, "PVRTC"))
        format = TEXTURE_PVRTC;
    else
        pspl_error(-1, "Unsupported texture format",
                   "this build of TextureManager doesn't support '%s' textures", tex_type);
    
    // Validate mip count
    if (tex_head->num_mips >= MAX_MIPS)
        pspl_error(-1, "Unsupported mip-mapped texture",
                   "maximum of %d mips supported; this texture has %d",
                   MAX_MIPS, tex_head->num_mips);
    
    // Prepare streamed load
    pspl_tm_recurse_t recurse_info = {
        .fill_struct = fill_struct,
        .key = key,
        .chan_count = tex_head->chan_count,
        .format = format,
        .provider_hooks = provider_hooks,
        .provider_handle = provider_handle
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
    
#   endif
    
    // Initialise platform objects
#   if PSPL_RUNTIME_PLATFORM_GL2
    glGenTextures(1, &fill_struct->texture_arr[key].tex_obj);
    fill_struct->texture_arr[key].mip_count = tex_head->num_mips;
    glGenBuffersARB(tex_head->num_mips, fill_struct->texture_arr[key].pbo_obj);
    
#   elif PSPL_RUNTIME_PLATFORM_D3D9
    
#   endif
    
    // Perform load
#   if PSPL_RUNTIME_PLATFORM_GL2 || PSPL_RUNTIME_PLATFORM_GL2_D3D9
    recursive_mip_load(&recurse_info,
                       tex_head->num_mips-1, 0, tex_head->size.native.width,
                       tex_head->size.native.height, init_off + tex_head->data_off);
    
#   elif PSPL_RUNTIME_PLATFORM_GX
    void* tex_data = memalign(32, length - tex_head->data_off);
    provider_hooks->seek(provider_handle, init_off + tex_head->data_off);
    provider_hooks->read_direct(provider_handle, length - tex_head->data_off, tex_data);
    GX_InitTexObj(&fill_struct->texture_arr[key], tex_data,
                  tex_head->size.native.width, tex_head->size.native.height,
                  recurse_info.sub_fmt, GX_REPEAT, GX_REPEAT, (tex_head->num_mips > 1)?GX_TRUE:GX_FALSE);
    GX_InitTexObjUserData(&fill_struct->texture_arr[key], tex_data);
    
#   endif
    
    return 0;
}
static void load_object(const pspl_runtime_psplc_t* object) {
    int tex_c = pspl_runtime_count_integer_embedded_data_objects(object);
    pspl_tm_map_entry* ent = pspl_malloc_malloc(&map_ctx, sizeof(pspl_tm_map_entry));
    ent->owner = object;
    ent->texture_count = tex_c;
    ent->texture_arr = calloc(tex_c, sizeof(TEX_T));
    pspl_runtime_enumerate_integer_embedded_data_objects(object,
                                                         (pspl_integer_enumerate_hook)load_enumerate,
                                                         (void*)&ent);
}

static void unload_object(const pspl_runtime_psplc_t* object) {
    int i,j;
    for (i=0 ; i<map_ctx.object_num ; ++i) {
        pspl_tm_map_entry* ent = map_ctx.object_arr[i];
        if (ent->owner == object) {
            // Destroy platform objects
            for (j=0 ; j<ent->texture_count ; ++j) {
#               if PSPL_RUNTIME_PLATFORM_GL2
                glDeleteTextures(1, &ent->texture_arr[j].tex_obj);
                glDeleteBuffersARB(ent->texture_arr[j].mip_count, ent->texture_arr[j].pbo_obj);
                
#               elif PSPL_RUNTIME_PLATFORM_GX
                void* tex_data = GX_GetTexObjUserData(&ent->texture_arr[j]);
                free(tex_data);
                
#               endif
            }
            free(ent->texture_arr);
            pspl_malloc_free(&map_ctx, ent);
            break;
        }
    }
}

static void bind_object(const pspl_runtime_psplc_t* object) {
    int i,j;
    for (i=0 ; i<map_ctx.object_num ; ++i) {
        pspl_tm_map_entry* ent = map_ctx.object_arr[i];
        if (ent->owner == object) {
            // Bind platform objects
            for (j=0 ; j<ent->texture_count ; ++j) {
#               if PSPL_RUNTIME_PLATFORM_GL2
                glActiveTexture(GL_TEXTURE0+j);
                glBindTexture(GL_TEXTURE_2D, ent->texture_arr[j].tex_obj);
                if (ent->texture_arr[j].mip_count > 1)
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                
#               elif PSPL_RUNTIME_PLATFORM_GX
                GX_LoadTexObj(&ent->texture_arr[j], GX_TEXMAP0+j);
                
#               endif
            }
            break;
        }
    }
}

pspl_runtime_extension_t TextureManager_runext = {
    .init_hook = init,
    .shutdown_hook = shutdown,
    .load_object_hook = load_object,
    .unload_object_hook = unload_object,
    .bind_object_hook = bind_object
};
