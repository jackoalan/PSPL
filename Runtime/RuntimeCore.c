//
//  RuntimeCore.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#define PSPL_INTERNAL

/* Should an error condition print backtrace? */
#ifndef _WIN32
#define PSPL_ERROR_PRINT_BACKTRACE 1
#endif

#ifndef _WIN32
#include <sys/ioctl.h>
#endif
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <PSPLExtension.h>
#include <PSPLInternal.h>
#include <PSPL/PSPLHash.h>
#if PSPL_ERROR_PRINT_BACKTRACE
#include <execinfo.h>
#endif


/* Escape character sequences to control xterm */
#define NOBKD     "\E[47;49m"
#define RED       "\E[47;31m"NOBKD
#define GREEN     "\E[47;32m"NOBKD
#define YELLOW    "\E[47;33m"NOBKD
#define BLUE      "\E[47;34m"NOBKD
#define MAGENTA   "\E[47;35m"NOBKD
#define CYAN      "\E[47;36m"NOBKD
#define NORMAL    "\033[0m"
#define BOLD      "\033[1m"
#define UNDERLINE "\033[4m"
#define SGR0      "\E[m\017"

/* Count of bytes to initially load from beginning of PSPLPs */
#define PSPL_RUNTIME_INITIAL_LOAD_SIZE 256


/* Heap Stuff */
#ifdef HW_RVL
extern void _pspl_wii_mem_init();
#define _pspl_mem_init _pspl_wii_mem_init
#define _pspl_mem_shutdown()
#else
pspl_malloc_context_t media_heap;
pspl_malloc_context_t indexing_heap;
void _pspl_mem_init() {
    pspl_malloc_context_init(&media_heap);
    pspl_malloc_context_init(&indexing_heap);
}
void _pspl_mem_shutdown() {
    pspl_malloc_context_destroy(&media_heap);
    pspl_malloc_context_destroy(&indexing_heap);
}
#endif


/* Active runtime extensions */
extern const pspl_extension_t* pspl_available_extensions[];

/* Active runtime platform */
extern const pspl_platform_t* pspl_available_platforms[];
#define pspl_runtime_platform pspl_available_platforms[0]

/* Are we using xterm? */
uint8_t xterm_colour = 0;

/* Forward decls */
static void _pspl_runtime_release_psplc(pspl_runtime_psplc_t* psplc, int total);
static void _pspl_runtime_release_archived_file(const pspl_runtime_arc_file_t* file, int total);

/* Thread-specific API state setters */
extern void pspl_api_set_load_state(intptr_t state);
extern void pspl_api_set_load_subject_index(intptr_t index);

#pragma mark Terminal Utils

/* Way to get terminal width (for line wrapping) */
static int term_cols() {
#   if _WIN32 || HW_RVL
    return 80;
#   else
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    return w.ws_col;
#   endif
}

/* Word len (similar to `strlen` but ignores escape sequences) */
static size_t word_len(const char* word) {
    size_t result = 0;;
    uint8_t in_esc = 0;
    --word;
    while ((*(++word))) {
        if (*word == '\E' || *word == '\033')
            in_esc = 1;
        else if (in_esc && (*word == 'm' || *word == '\017'))
            in_esc = 0;
        else if (*word == '\0')
            break;
        else if (!in_esc)
            ++result;
    }
    return result;
}

/* Transform string into wrapped string */
#define truncate_string(str, max_len, to_buf) strncpy(to_buf, str, (max_len)-3); strncpy(to_buf+(max_len)-3, "...", 4);
static char* wrap_string(const char* str_in, int indent) {
    
    // Term col count
    int num_cols = term_cols();
    if (num_cols <= 0)
        num_cols = 80;
    if (num_cols > 6)
        num_cols -= 6;
    
    // Space for strings
    char cpy_str[1024];
    char* result_str = malloc(1024);
    result_str[0] = '\0';
    
    // Copy string
    strlcpy(cpy_str, str_in, 1024);
    
    // Initialise space-tokeniser
    char* save_ptr;
    char* word_str = strtok_r(cpy_str, " ", &save_ptr);
    
    // Enumerate words (and keep track of accumulated characters)
    int cur_line_col = 0;
    int cur_line = 0;
    
    // All-indent
    if (indent == 2) {
        cur_line_col = 6;
        strlcat(result_str, "      ", 1024);
    }
    
    for (;;) {
        if (!word_str) // Done
            break;
        
        // Available space remaining on line
        int avail_line_space = num_cols-cur_line_col;
        
        // Actual word length (0 length is actually a space)
        size_t word_length = word_len(word_str);
        if (!word_length)
            word_length = 1;
        
        // Spill onto next line if needed (with optional indent for readability)
        if (word_length >= avail_line_space) {
            if (indent) {
                cur_line_col = 6;
                strlcat(result_str, "\n      ", 1024);
            } else {
                cur_line_col = 0;
                strlcat(result_str, "\n", 1024);
            }
            avail_line_space = num_cols-cur_line_col;
            //if (!cur_line)
            //    num_cols += 2;
            ++cur_line;
        }
        
        strlcat(result_str, word_str, 1024);
        strlcat(result_str, " ", 1024);
        cur_line_col += word_length+1;
        
        word_str = strtok_r(save_ptr, " ", &save_ptr);
    }
    
    char* last_space = strrchr(result_str, ' ');
    if (last_space)
        *last_space = '\0';
    return result_str;
    
}

#pragma mark Error handling

/* This will print a backtrace for `pspl_error` call */
#if PSPL_ERROR_PRINT_BACKTRACE
#define BACKTRACE_DEPTH 20
static void print_backtrace() {
    void *array[BACKTRACE_DEPTH];
    size_t size;
    char **strings;
    size_t i;
    
#   if HW_RVL
    size = backtrace(array, BACKTRACE_DEPTH);
    
    for (i = 0; i < size; i++)
        fprintf(stderr, "%p\n", array[i]);
    
#   else
    size = backtrace(array, BACKTRACE_DEPTH);
    strings = backtrace_symbols(array, (int)size);
    
    for (i = 0; i < size; i++)
        fprintf(stderr, "%s\n", strings[i]);
    
    free(strings);
    
#   endif
    
}
#endif

/* Runtime-wide error handler
 * uses a global context state to print context-sensitive error info */
void pspl_error(int exit_code, const char* brief, const char* msg, ...) {
    if (xterm_colour)
        fprintf(stderr, BOLD RED"*** PSPL ERROR ***\n"SGR0);
    else
        fprintf(stderr, "*** PSPL ERROR ***\n");
    
    brief = wrap_string(brief, 1);
    if (xterm_colour)
        fprintf(stderr, BOLD"%s:\n"SGR0, brief);
    else
        fprintf(stderr, "%s:\n", brief);
    char* msg_str = NULL;
    if (msg) {
        va_list va;
        va_start(va, msg);
        char msg_str_buf[1024];
        msg_str = msg_str_buf;
        vsprintf(msg_str, msg, va);
        va_end(va);
        char* new_msg = wrap_string(msg_str, 2);
        msg_str = new_msg;
        fprintf(stderr, "%s\n", msg_str);
        free(new_msg);
    }
    
#   if PSPL_ERROR_PRINT_BACKTRACE
    if (xterm_colour)
        fprintf(stderr, BOLD BLUE"\nBacktrace:\n"SGR0);
    else
        fprintf(stderr, "\nBacktrace:\n");
    
    print_backtrace();
    fprintf(stderr, "\n");
#   endif // PSPL_ERROR_PRINT_BACKTRACE
    
    exit(exit_code);
}

/* Runtime-wide warning handler
 * uses a global context state to print context-sensitive error info */
void pspl_warn(const char* brief, const char* msg, ...) {
    if (xterm_colour)
        fprintf(stderr, BOLD MAGENTA"*** PSPL WARNING ***\n"SGR0);
    else
        fprintf(stderr, "*** PSPL WARNING ***\n");
    
    brief = wrap_string(brief, 1);
    if (xterm_colour)
        fprintf(stderr, BOLD"%s:\n"SGR0, brief);
    else
        fprintf(stderr, "%s:\n", brief);
    char* msg_str = NULL;
    if (msg) {
        va_list va;
        va_start(va, msg);
        char msg_str_buf[1024];
        msg_str = msg_str_buf;
        vsprintf(msg_str, msg, va);
        va_end(va);
        char* new_msg = wrap_string(msg_str, 2);
        msg_str = new_msg;
        fprintf(stderr, "%s\n", msg_str);
        free(new_msg);
    }
}


#pragma mark Internal Types

struct stdio_handle {
    pspl_malloc_context_t mem_ctx;
    FILE* file;
    char original_path[MAXPATHLEN];
};

struct mem_handle {
    void* data;
    void* cur;
    size_t len;
};

/* Internal hashed object record type */
typedef struct {
    pspl_hash hash;
    pspl_object_hash_record_t record;
} _pspl_object_hash_record_t;

/* Internal extension/platform index object */
typedef struct {
    union {
        uint32_t extension_index;
        uint32_t platform_index;
    };
    unsigned int h_arr_c;
    const _pspl_object_hash_record_t* h_arr;
    unsigned int i_arr_c;
    const pspl_object_int_record_t* i_arr;
} _pspl_object_index_t;

/* Internal PSPLC representation type */
typedef struct {
    
    // Public structure
    pspl_runtime_psplc_t public;
    
    // Reference count
    unsigned int ref_count;
    
    // Object arrays
    unsigned int ext_arr_c;
    const _pspl_object_index_t* ext_arr;
    unsigned int plat_arr_c;
    const _pspl_object_index_t* plat_arr;
    
    // File offset and length of data blobs
    uint32_t blobs_off;
    uint32_t blobs_len;
    
    // Memory buffer for loading data blobs into
    void* blobs_buf;
    
    // Per-extension user-pointers (the `count` definition is set via CMake)
    void* extension_pointers[PSPL_RUNTIME_EXTENSION_COUNT];
    
} _pspl_runtime_psplc_t;

/* Internal Archived File representation type */
typedef struct {
    
    // Public structure
    pspl_runtime_arc_file_t public;
    
    // Reference count
    unsigned int ref_count;
    
    // File offset
    uint32_t file_off;
    
} _pspl_runtime_arc_file_t;

/* Package representation type */
struct _pspl_loaded_package {
    
    // PSPLP data provider
    union {
        struct stdio_handle stdio;
        struct mem_handle mem;
        const void* provider;
    } provider;
    uint8_t provider_local;
    
    // Index of current runtime platform encoded within PSPLP
    uint8_t runtime_platform_index;
    
    // Data provider hooks
    const pspl_data_provider_t* provider_hooks;
    
    // Extension array (NULL-term)
    const pspl_extension_t** ext_array;
    
    
    
    // Main PSPLC object table
    unsigned int psplc_count;
    _pspl_runtime_psplc_t* psplc_array;
    
    // Main Archived file table
    unsigned int file_count;
    const _pspl_runtime_arc_file_t* file_array;
    
};


#pragma mark Extension/Platform Load API

/* These *must* be called within the `load` or `bind` hook of platform or extension */
enum api_load_state {
    PSPL_LOADING_NONE = 0,
    PSPL_LOADING_EXT  = 1,
    PSPL_LOADING_PLAT = 2
};

/* Get embedded data object for extension by key (to hash) */
int pspl_runtime_get_embedded_data_object_from_key(const pspl_runtime_psplc_t* object,
                                                   const char* key,
                                                   pspl_hash* hash_out,
                                                   pspl_data_object_t* data_object_out) {
    if (!object || !key)
        return -1;
    
    // Hash key
    pspl_hash_ctx_t hash_ctx;
    pspl_hash_init(&hash_ctx);
    pspl_hash_write(&hash_ctx, key, strlen(key));
    pspl_hash* result;
    pspl_hash_result(&hash_ctx, result);
    if (hash_out)
        pspl_hash_cpy(hash_out, result);
    
    return pspl_runtime_get_embedded_data_object_from_hash(object, result, data_object_out);
    
}

/* Count embedded object in PSPLC keyed by hash */
int pspl_runtime_count_hash_embedded_data_objects(const pspl_runtime_psplc_t* object) {
    if (!object)
        return -1;
    
    const _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)object;
    
    intptr_t api_load_state = pspl_api_load_state();
    intptr_t api_load_subject_index = pspl_api_load_subject_index();
    
    const _pspl_object_index_t* idx = NULL;
    if (api_load_state == PSPL_LOADING_EXT)
        idx = &obj->ext_arr[api_load_subject_index];
    else if (api_load_state == PSPL_LOADING_PLAT)
        idx = &obj->plat_arr[api_load_subject_index];
    else
        return -1;
    
    return idx->h_arr_c;
}

/* Get embedded object for extension by hash */
int pspl_runtime_get_embedded_data_object_from_hash(const pspl_runtime_psplc_t* object,
                                                    const pspl_hash* hash,
                                                    pspl_data_object_t* data_object_out) {
    if (!object || !hash || !data_object_out)
        return -1;
    
    int i,j;
    const _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)object;
    
    intptr_t api_load_state = pspl_api_load_state();
    intptr_t api_load_subject_index = pspl_api_load_subject_index();
    
    const _pspl_object_index_t* idx = NULL;
    if (api_load_state == PSPL_LOADING_EXT)
        idx = &obj->ext_arr[api_load_subject_index];
    else if (api_load_state == PSPL_LOADING_PLAT)
        idx = &obj->plat_arr[api_load_subject_index];
    else
        return -1;
    
    
    // Perform lookup
    for (i=0 ; i<idx->h_arr_c ; ++i) {
        const _pspl_object_hash_record_t* rec = &idx->h_arr[i];
        for (j=0 ; j<(sizeof(pspl_hash)/4) ; ++j) {
            if (hash->w[j] != rec->hash.w[j]) {
                rec = NULL;
                break;
            }
        }
        if (rec) {
            
            // Populate data
            data_object_out->object_data = obj->blobs_buf + rec->record.object_off;
            data_object_out->object_len = rec->record.object_len;
            
            // Done
            return 0;
            
        }
    }
    
    return -1;
    
}

/* Enumerate hashed embedded objects */
void pspl_runtime_enumerate_hash_embedded_data_objects(const pspl_runtime_psplc_t* object,
                                                       pspl_hash_enumerate_hook hook,
                                                       void* user_ptr) {
    if (!object || !hook)
        return;
    
    int i;
    const _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)object;
    
    intptr_t api_load_state = pspl_api_load_state();
    intptr_t api_load_subject_index = pspl_api_load_subject_index();
    
    const _pspl_object_index_t* idx = NULL;
    if (api_load_state == PSPL_LOADING_EXT)
        idx = &obj->ext_arr[api_load_subject_index];
    else if (api_load_state == PSPL_LOADING_PLAT)
        idx = &obj->plat_arr[api_load_subject_index];
    else
        return;
    
    
    // Perform lookup
    for (i=0 ; i<idx->h_arr_c ; ++i) {
        const _pspl_object_hash_record_t* rec = &idx->h_arr[i];
        
        // Populate data
        pspl_data_object_t data_object_out;
        data_object_out.object_data = obj->blobs_buf + rec->record.object_off;
        data_object_out.object_len = rec->record.object_len;
            
        if(hook(&data_object_out, &rec->hash, user_ptr) < 0)
            return;

    }
}

/* Count embedded objects in PSPLC keyed by integer */
int pspl_runtime_count_integer_embedded_data_objects(const pspl_runtime_psplc_t* object) {
    if (!object)
        return -1;
    
    const _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)object;
    
    intptr_t api_load_state = pspl_api_load_state();
    intptr_t api_load_subject_index = pspl_api_load_subject_index();
    
    const _pspl_object_index_t* idx = NULL;
    if (api_load_state == PSPL_LOADING_EXT)
        idx = &obj->ext_arr[api_load_subject_index];
    else if (api_load_state == PSPL_LOADING_PLAT)
        idx = &obj->plat_arr[api_load_subject_index];
    else
        return -1;
    
    return idx->i_arr_c;
}

/* Get embedded object for extension by index */
int pspl_runtime_get_embedded_data_object_from_integer(const pspl_runtime_psplc_t* object,
                                                       uint32_t index,
                                                       pspl_data_object_t* data_object_out) {
    if (!object || !data_object_out)
        return -1;
    int i;
    const _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)object;
    
    intptr_t api_load_state = pspl_api_load_state();
    intptr_t api_load_subject_index = pspl_api_load_subject_index();
    
    const _pspl_object_index_t* idx = NULL;
    if (api_load_state == PSPL_LOADING_EXT)
        idx = &obj->ext_arr[api_load_subject_index];
    else if (api_load_state == PSPL_LOADING_PLAT)
        idx = &obj->plat_arr[api_load_subject_index];
    else
        return -1;
    
    
    // Perform lookup
    for (i=0 ; i<idx->i_arr_c ; ++i) {
        const pspl_object_int_record_t* rec = &idx->i_arr[i];
        if (index == rec->object_index) {
            
            // Populate data
            data_object_out->object_data = obj->blobs_buf + rec->object_off;
            data_object_out->object_len = rec->object_len;
            
            // Done
            return 0;
            
        }
    }
    
    return -1;
    
}

/* Enumerate integer-indexed embedded objects */
void pspl_runtime_enumerate_integer_embedded_data_objects(const pspl_runtime_psplc_t* object,
                                                          pspl_integer_enumerate_hook hook,
                                                          void* user_ptr) {
    if (!object || !hook)
        return;
    int i;
    const _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)object;
    
    intptr_t api_load_state = pspl_api_load_state();
    intptr_t api_load_subject_index = pspl_api_load_subject_index();
    
    const _pspl_object_index_t* idx = NULL;
    if (api_load_state == PSPL_LOADING_EXT)
        idx = &obj->ext_arr[api_load_subject_index];
    else if (api_load_state == PSPL_LOADING_PLAT)
        idx = &obj->plat_arr[api_load_subject_index];
    else
        return;
    
    
    // Perform lookup
    for (i=0 ; i<idx->i_arr_c ; ++i) {
        const pspl_object_int_record_t* rec = &idx->i_arr[i];
            
        // Populate data
        pspl_data_object_t data_object_out;
        data_object_out.object_data = obj->blobs_buf + rec->object_off;
        data_object_out.object_len = rec->object_len;
        
        if(hook(&data_object_out, rec->object_index, user_ptr) < 0)
            return;
        
    }
}

/* Set extension-specific user-data pointer for individual PSPLC object */
int pspl_runtime_set_extension_user_data_pointer(const pspl_extension_t* extension, const pspl_runtime_psplc_t* object, void* ptr) {
    if (!extension || !object)
        return -1;
    _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)object;
    obj->extension_pointers[extension->runtime_extension_index] = ptr;
    return 0;
}

/* Get extension-specific user-data pointer for individual PSPLC object */
void* pspl_runtime_get_extension_user_data_pointer(const pspl_extension_t* extension, const pspl_runtime_psplc_t* object) {
    if (!extension || !object)
        return NULL;
    const _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)object;
    return obj->extension_pointers[extension->runtime_extension_index];
}

#pragma mark Runtime API

/* Malloc context for loaded packages */
static pspl_malloc_context_t package_mem_ctx;

/**
 * Init PSPL Runtime
 *
 * Allocates necessary state for the runtime platform compiled into `pspl-rt`.
 * PSPLP package files may be loaded afterwards.
 *
 * @param platform_out Output pointer that's set with a reference to the
 *        compiled-in platform metadata structure
 * @return 0 if successful, negative otherwise
 */
int pspl_runtime_init(const pspl_platform_t** platform_out) {
    
    // Setup memory heaps
    _pspl_mem_init();
    
    // Check for xterm
    const char* term_type = getenv("TERM");
    if (term_type && strlen(term_type) >= 5 && !strncmp("xterm", term_type, 5))
        xterm_colour = 1;
    
    // Init package mem context
    pspl_malloc_context_init(&package_mem_ctx);
    
    // This platform
    const pspl_platform_t* plat = pspl_runtime_platform;
    int err;

    // Init platform
    if (plat->runtime_platform && plat->runtime_platform->init_hook)
        if ((err = plat->runtime_platform->init_hook(plat)))
            return err;
    
    // Init extensions
    const pspl_extension_t** ext_arr = pspl_available_extensions;
    const pspl_extension_t* ext;
    --ext_arr;
    while ((ext = *(++ext_arr))) {
        if (ext->runtime_extension && ext->runtime_extension->init_hook)
            if ((err = ext->runtime_extension->init_hook(ext)))
                return err;
    }
    
    if (platform_out)
        *platform_out = plat;
    
    return 0;
    
}

/**
 * Shutdown PSPL Runtime
 *
 * Deallocates any objects and archived files owned by the PSPL runtime,
 * closes any open packages, unloads all objects
 */
void pspl_runtime_shutdown() {
    
    // Unload all packages
    int i;
    for (i=0 ; i<package_mem_ctx.object_num ; ++i) {
        pspl_runtime_package_t* package = package_mem_ctx.object_arr[i];
        if (package)
            pspl_runtime_unload_package(package);
    }
    
    // Destroy package mem context
    pspl_malloc_context_destroy(&package_mem_ctx);
    
    // Final extensions
    const pspl_extension_t** ext_arr = pspl_available_extensions;
    const pspl_extension_t* ext;
    --ext_arr;
    while ((ext = *(++ext_arr))) {
        if (ext->runtime_extension && ext->runtime_extension->shutdown_hook)
            ext->runtime_extension->shutdown_hook();
    }
    
    const pspl_platform_t* plat = pspl_runtime_platform;
    
    // Final platform
    if (plat->runtime_platform && plat->runtime_platform->shutdown_hook)
        plat->runtime_platform->shutdown_hook();
    
    // Destroy heaps
    _pspl_mem_shutdown();
    
}


#pragma mark Packages

/* Lookup target extension by name */
static const pspl_extension_t* lookup_ext(const char* ext_name, unsigned int* idx_out) {
    const pspl_extension_t* ext = NULL;
    unsigned int i = 0;
    while ((ext = pspl_available_extensions[i++])) {
        if (!strcmp(ext->extension_name, ext_name)) {
            if (idx_out)
                *idx_out = i-1;
            break;
        }
    }
    return ext;
}


static const pspl_data_provider_t stdio;
static const pspl_data_provider_t membuf;


/* Private routine to load PSPLP data */
#define PACKAGE_PROVIDER(package) (package->provider_local)?&package->provider.provider:package->provider.provider
#define IS_PSPLP_BI pspl_head->endian_flags == PSPL_BI_ENDIAN
static int load_psplp(pspl_runtime_package_t* package) {
    int j,k,l;
    const void* package_provider = PACKAGE_PROVIDER(package);
    size_t package_len = package->provider_hooks->len(package_provider);
    
    
#   pragma mark Header Read
    
    // Read header chunk of data
    struct {
        pspl_header_t header;
        pspl_off_header_bi_t off_header;
        pspl_psplp_header_bi_t psplp_header;
    } h_data;
    void* header_data = &h_data;
    if (package->provider_hooks == &membuf) {
        package->provider_hooks->read(package_provider,
                                      sizeof(h_data),
                                      &header_data);
    } else {
        package->provider_hooks->read_direct(package_provider,
                                             sizeof(h_data),
                                             header_data);
    }
    
    // Bind header
    const pspl_header_t* pspl_head = header_data;
    const void* pspl_cur = header_data + sizeof(pspl_header_t);
    
    // Validate members
    if (memcmp(pspl_head->magic, "PSPL", 4)) {
        pspl_warn("Invalid magic on provided package", "magic should be 'PSPL'");
        return -1;
    }
    if (pspl_head->package_flag != 1) {
        pspl_warn("Provided PSPLC instead of package", "only packages are accepted by runtime");
        return -1;
    }
    if (pspl_head->version != PSPL_VERSION) {
        pspl_warn("PSPL version mismatch", "Version %u provided; expected version %u",
                  pspl_head->version, PSPL_VERSION);
        return -1;
    }
#   if defined(__LITTLE_ENDIAN__)
    if (pspl_head->endian_flags == PSPL_BIG_ENDIAN) {
        pspl_warn("Incompatible PSPL byte-order", "unable to use big-endian PSPL");
        return -1;
    }
#   elif defined(__BIG_ENDIAN__)
    if (pspl_head->endian_flags == PSPL_LITTLE_ENDIAN) {
        pspl_warn("Incompatible PSPL byte-order", "unable to use little-endian PSPL");
        return -1;
    }
#   endif
    
    // Offset header next
    const pspl_off_header_t* off_header = pspl_cur;
    if (IS_PSPLP_BI) {
        const pspl_off_header_bi_t* bi_header = pspl_cur;
        off_header = &bi_header->native;
        pspl_cur += sizeof(pspl_off_header_t);
    }
    pspl_cur += sizeof(pspl_off_header_t);

    // PSPLP header next
    const pspl_psplp_header_t* psplp_header = pspl_cur;
    if (IS_PSPLP_BI) {
        const pspl_psplp_header_bi_t* bi_header = pspl_cur;
        psplp_header = &bi_header->native;
        pspl_cur += sizeof(pspl_psplp_header_t);
    }
    pspl_cur += sizeof(pspl_psplp_header_t);
    
    
#   pragma mark String Tables Read
    
    // Read in string tables at end
    void* strings_data = NULL;
    size_t strings_len = package_len - off_header->extension_name_table_off;
    if (strings_len && strings_len < package_len) {
        package->provider_hooks->seek(package_provider, off_header->extension_name_table_off);
        if (package->provider_hooks == &membuf)
            package->provider_hooks->read(package_provider, strings_len, &strings_data);
        else {
            strings_data = pspl_allocate_indexing_block(strings_len);
            package->provider_hooks->read_direct(package_provider, strings_len, strings_data);
        }
    }
    
    if (strings_data) {
        
        // Populate extension set
        unsigned int psplc_extension_count = off_header->extension_name_table_c;
        const char* psplc_extension_name = (char*)strings_data;
        package->ext_array =
        pspl_allocate_indexing_block((psplc_extension_count+1) * sizeof(pspl_extension_t*));
        for (j=0 ; j<psplc_extension_count ; ++j) {
            
            // Lookup
            const pspl_extension_t* ext = lookup_ext(psplc_extension_name, NULL);
            if (!ext) {
                pspl_warn("PSPLP-required extension not available",
                          "requested `%s` which is not available in this build of `pspl-rt`",
                          psplc_extension_name);
                return -1;
            }
            
            // Add to set
            package->ext_array[j] = ext;
            
            // Advance
            psplc_extension_name += strlen(psplc_extension_name) + 1;
            
        }
        package->ext_array[psplc_extension_count] = NULL;
        
        
        // Populate platform index
        unsigned int psplc_platform_count = off_header->platform_name_table_c;
        const char* psplc_platform_name = psplc_extension_name;
        package->runtime_platform_index = 0xff;
        for (j=0 ; j<psplc_platform_count ; ++j) {
            
            // Verify and set if found
            if (!strcmp(psplc_platform_name, pspl_runtime_platform->platform_name)) {
                package->runtime_platform_index = j;
                break;
            }
            
            // Advance
            psplc_platform_name += strlen(psplc_platform_name) + 1;
            
        }
        
        // Advise user if this platform has been discriminated
        if (psplc_platform_count && package->runtime_platform_index == 0xff)
            pspl_warn("Platform not present in PSPLP",
                      "this build of `pspl-rt` was made for packages targeting '%s' platform; "
                      "graphics functionality will not be utilised",
                      pspl_runtime_platform->platform_name);
        
        // Free strings data
        pspl_free_indexing_block(strings_data);
        
    }
    
    
#   pragma mark Index Read
    
    // Read in index tables for each embedded PSPLC
    size_t i1_off = pspl_cur - header_data;
    size_t i1_len = (((IS_PSPLP_BI)?
                      sizeof(pspl_psplp_psplc_index_bi_t):
                      sizeof(pspl_psplp_psplc_index_t))+
                     sizeof(pspl_hash))*psplp_header->psplc_count;
    void* i1_table = NULL;
    
    if (i1_len && i1_len < package_len) {
        package->provider_hooks->seek(package_provider, i1_off);
        if (package->provider_hooks == &membuf)
            package->provider_hooks->read(package_provider, i1_len, &i1_table);
        else {
            i1_table = pspl_allocate_indexing_block(i1_len);
            package->provider_hooks->read_direct(package_provider, i1_len, i1_table);
        }
    }
    
    package->psplc_count = 0;
    package->psplc_array = NULL;
    if (i1_table) {
        const void* i1_table_cur = i1_table;
        
        // Allocate main table
        _pspl_runtime_psplc_t* dest_table = pspl_allocate_indexing_block(psplp_header->psplc_count *
                                                                         sizeof(_pspl_runtime_psplc_t));
        
        // Read in each PSPLC record
        for (j=0 ; j<psplp_header->psplc_count ; ++j) {
            const pspl_hash* hash = i1_table_cur;
            i1_table_cur += sizeof(pspl_hash);
            const pspl_psplp_psplc_index_t* ent = i1_table_cur;
            if (IS_PSPLP_BI) {
                const pspl_psplp_psplc_index_bi_t* bi_ent = i1_table_cur;
                ent = &bi_ent->native;
                i1_table_cur += sizeof(pspl_psplp_psplc_index_t);
            }
            i1_table_cur += sizeof(pspl_psplp_psplc_index_t);
            
            // Data blobs offset
            uint32_t blobs_off = ent->psplc_base + ent->psplc_tables_len;
            
            // Read offset tables
            void* off_tables = NULL;
            package->provider_hooks->seek(package_provider, ent->psplc_base);
            if (package->provider_hooks == &membuf)
                package->provider_hooks->read(package_provider, ent->psplc_tables_len, &off_tables);
            else {
                off_tables = pspl_allocate_indexing_block(ent->psplc_tables_len);
                package->provider_hooks->read_direct(package_provider, ent->psplc_tables_len, off_tables);
            }
            const void* off_tables_cur = off_tables;
            
            // PSPLC head
            const pspl_psplc_header_t* psplc_head = off_tables_cur;
            if (IS_PSPLP_BI) {
                const pspl_psplc_header_bi_t* bi_ent = off_tables_cur;
                psplc_head = &bi_ent->native;
                off_tables_cur += sizeof(pspl_psplc_header_t);
            }
            off_tables_cur += sizeof(pspl_psplc_header_t);
                        
            // Extension heads
            _pspl_object_index_t* ext_index_arr = pspl_allocate_indexing_block(psplc_head->extension_count *
                                                                               sizeof(_pspl_object_index_t));
            for (k=0 ; k<psplc_head->extension_count ; ++k) {
                const pspl_object_array_extension_t* ext = off_tables_cur;
                if (IS_PSPLP_BI) {
                    const pspl_object_array_extension_bi_t* bi_ent = off_tables_cur;
                    ext = &bi_ent->native;
                    off_tables_cur += sizeof(pspl_object_array_extension_t);
                }
                off_tables_cur += sizeof(pspl_object_array_extension_t);
                
                // Allocate arrays
                _pspl_object_hash_record_t* hash_dest_table = pspl_allocate_indexing_block(ext->ext_hash_indexed_object_count *
                                                                                           sizeof(_pspl_object_hash_record_t));
                pspl_object_int_record_t* int_dest_table = pspl_allocate_indexing_block(ext->ext_int_indexed_object_count *
                                                                                        sizeof(pspl_object_int_record_t));
                
                // Hashed indexing read-in
                unsigned int h_arr_c = 0;
                const void* ext_off = off_tables + ext->ext_hash_indexed_object_array_off;
                for (l=0 ; l<ext->ext_hash_indexed_object_count ; ++l) {
                    const pspl_hash* hash = ext_off;
                    ext_off += sizeof(pspl_hash);
                    const pspl_object_hash_record_t* rec = ext_off;
                    if (IS_PSPLP_BI) {
                        const pspl_object_hash_record_bi_t* bi_rec = ext_off;
                        rec = &bi_rec->native;
                        ext_off += sizeof(pspl_object_hash_record_t);
                    }
                    ext_off += sizeof(pspl_object_hash_record_t);
                    
                    // Determine if platform is (not) supported
                    if (!((rec->platform_availability_bits >> package->runtime_platform_index) & 1))
                        continue;
                    
                    // Populate runtime data (simple copy)
                    _pspl_object_hash_record_t* dest = &hash_dest_table[h_arr_c++];
                    pspl_hash_cpy(&dest->hash, hash);
                    dest->record = *rec;
                    dest->record.object_off -= blobs_off;
                }
                
                // Integer indexing read-in
                unsigned int i_arr_c = 0;
                ext_off = off_tables + ext->ext_int_indexed_object_array_off;
                for (l=0 ; l<ext->ext_int_indexed_object_count ; ++l) {
                    const pspl_object_int_record_t* rec = ext_off;
                    if (IS_PSPLP_BI) {
                        const pspl_object_int_record_bi_t* bi_rec = ext_off;
                        rec = &bi_rec->native;
                        ext_off += sizeof(pspl_object_int_record_t);
                    }
                    ext_off += sizeof(pspl_object_int_record_t);
                    
                    // Determine if platform is (not) supported
                    if (!((rec->platform_availability_bits >> package->runtime_platform_index) & 1))
                        continue;
                    
                    // Populate runtime data (simple copy)
                    pspl_object_int_record_t* dest = &int_dest_table[i_arr_c++];
                    *dest = *rec;
                    dest->object_off -= blobs_off;
                }
                
                // Access index array and apply
                _pspl_object_index_t* index = &ext_index_arr[k];
                index->extension_index = ext->extension_index;
                index->h_arr_c = h_arr_c;
                index->h_arr = hash_dest_table;
                index->i_arr_c = i_arr_c;
                index->i_arr = int_dest_table;
                
            }
            
            // Platform heads
            _pspl_object_index_t* plat_index_arr = pspl_allocate_indexing_block(sizeof(_pspl_object_index_t));
            unsigned loaded_plat_count = 0;
            for (k=0 ; k<psplc_head->platform_count ; ++k) {
                const pspl_object_array_extension_t* ext = off_tables_cur;
                if (IS_PSPLP_BI) {
                    const pspl_object_array_extension_bi_t* bi_ent = off_tables_cur;
                    ext = &bi_ent->native;
                    off_tables_cur += sizeof(pspl_object_array_extension_t);
                }
                off_tables_cur += sizeof(pspl_object_array_extension_t);
                
                // Determine if this is (not) the platform we need
                if (ext->platform_index != package->runtime_platform_index)
                    continue;
                
                // Allocate arrays
                _pspl_object_hash_record_t* hash_dest_table = pspl_allocate_indexing_block(ext->ext_hash_indexed_object_count *
                                                                                           sizeof(_pspl_object_hash_record_t));
                pspl_object_int_record_t* int_dest_table = pspl_allocate_indexing_block(ext->ext_int_indexed_object_count *
                                                                                        sizeof(pspl_object_int_record_t));
                
                // Hashed indexing read-in
                unsigned int h_arr_c = 0;
                const void* ext_off = off_tables + ext->ext_hash_indexed_object_array_off;
                for (l=0 ; l<ext->ext_hash_indexed_object_count ; ++l) {
                    const pspl_hash* hash = ext_off;
                    ext_off += sizeof(pspl_hash);
                    const pspl_object_hash_record_t* rec = ext_off;
                    if (IS_PSPLP_BI) {
                        const pspl_object_hash_record_bi_t* bi_rec = ext_off;
                        rec = &bi_rec->native;
                        ext_off += sizeof(pspl_object_hash_record_t);
                    }
                    ext_off += sizeof(pspl_object_hash_record_t);
                    
                    // Populate runtime data (simple copy)
                    _pspl_object_hash_record_t* dest = &hash_dest_table[h_arr_c++];
                    pspl_hash_cpy(&dest->hash, hash);
                    dest->record = *rec;
                    dest->record.object_off -= blobs_off;
                }
                
                // Integer indexing read-in
                unsigned int i_arr_c = 0;
                ext_off = off_tables + ext->ext_int_indexed_object_array_off;
                for (l=0 ; l<ext->ext_int_indexed_object_count ; ++l) {
                    const pspl_object_int_record_t* rec = ext_off;
                    if (IS_PSPLP_BI) {
                        const pspl_object_int_record_bi_t* bi_rec = ext_off;
                        rec = &bi_rec->native;
                        ext_off += sizeof(pspl_object_int_record_t);
                    }
                    ext_off += sizeof(pspl_object_int_record_t);
                    
                    // Populate runtime data (simple copy)
                    pspl_object_int_record_t* dest = &int_dest_table[i_arr_c++];
                    *dest = *rec;
                    dest->object_off -= blobs_off;
                }
                
                // Access index array and apply
                _pspl_object_index_t* index = &plat_index_arr[loaded_plat_count++];
                index->platform_index = ext->platform_index;
                index->h_arr_c = h_arr_c;
                index->h_arr = hash_dest_table;
                index->i_arr_c = i_arr_c;
                index->i_arr = int_dest_table;
                
                // Only one platform loaded
                break;

            }
            
            // Populate runtime object
            _pspl_runtime_psplc_t* dest_psplc = &dest_table[j];
            pspl_hash_cpy((pspl_hash*)&dest_psplc->public.hash, hash);
            dest_psplc->public.parent = package;
            dest_psplc->ref_count = 0;
            dest_psplc->ext_arr_c = psplc_head->extension_count;
            dest_psplc->ext_arr = ext_index_arr;
            dest_psplc->plat_arr_c = loaded_plat_count;
            dest_psplc->plat_arr = plat_index_arr;
            dest_psplc->blobs_off = blobs_off;
            dest_psplc->blobs_len = ent->psplc_blobs_len;
            dest_psplc->blobs_buf = NULL;
            
            // Free offset tables if loaded using stdio
            if (package->provider_hooks != &membuf)
                pspl_free_indexing_block(off_tables);
            
        }
        
        // Free i1 table if loaded using stdio
        if (package->provider_hooks != &membuf)
            pspl_free_indexing_block(i1_table);
        
        // Apply to package
        package->psplc_count = psplp_header->psplc_count;
        package->psplc_array = dest_table;
        
    } else {
        pspl_warn("No valid PSPLC index table present in PSPLP", "PSPLP may be corrupt");
        return -1;
    }
    
    
#   pragma mark Archived File Table Read
    
    // Read in archived file table
    size_t ft_off = off_header->file_table_off;
    size_t ft_len = (((IS_PSPLP_BI)?
                      sizeof(pspl_file_stub_bi_t):
                      sizeof(pspl_file_stub_t))+
                     sizeof(pspl_hash))*off_header->file_table_c;
    void* ft_table = NULL;
    
    if (ft_len && ft_len < package_len) {
        package->provider_hooks->seek(package_provider, ft_off);
        if (package->provider_hooks == &membuf)
            package->provider_hooks->read(package_provider, ft_len, &ft_table);
        else {
            ft_table = pspl_allocate_indexing_block(ft_len);
            package->provider_hooks->read_direct(package_provider, ft_len, ft_table);
        }
    }
    
    package->file_count = 0;
    package->file_array = NULL;
    if (ft_table) {
        const void* ft_table_cur = ft_table;
        
        // Allocate main table
        _pspl_runtime_arc_file_t* dest_table = pspl_allocate_indexing_block(off_header->file_table_c *
                                                                            sizeof(_pspl_runtime_arc_file_t));
        
        // Read in each file record
        unsigned int file_count = 0;
        for (j=0 ; j<off_header->file_table_c ; ++j) {
            const pspl_hash* hash = ft_table_cur;
            ft_table_cur += sizeof(pspl_hash);
            const pspl_file_stub_t* ent = ft_table_cur;
            if (IS_PSPLP_BI) {
                const pspl_file_stub_bi_t* bi_ent = ft_table_cur;
                ent = &bi_ent->native;
                ft_table_cur += sizeof(pspl_file_stub_t);
            }
            ft_table_cur += sizeof(pspl_file_stub_t);
            
            // Determine if platform is (not) supported
            if (!((ent->platform_availability_bits >> package->runtime_platform_index) & 1))
                continue;
            
            // Populate file record
            _pspl_runtime_arc_file_t* dest = &dest_table[file_count++];
            pspl_hash_cpy((pspl_hash*)&dest->public.hash, hash);
            dest->public.file_len = ent->file_len;
            dest->public.file_data = NULL;
            dest->public.parent = package;
            dest->ref_count = 0;
            dest->file_off = ent->file_off;
        }
        
        // Free file table if loaded using stdio
        if (package->provider_hooks != &membuf)
            pspl_free_indexing_block(ft_table);
            
        // Apply to package
        package->file_count = file_count;
        package->file_array = dest_table;
    }
    
    
    // All good!!
    return 0;
}


/* Various ways to load a PSPLP */


/* stdio.h package file */

static int stdio_open(void* handle, const char* path) {
    struct stdio_handle* stdio = ((struct stdio_handle*)handle);
    strlcpy(stdio->original_path, path, MAXPATHLEN);
    stdio->file = fopen(path, "r");
    if (!stdio->file)
        return -1;
    stdio->mem_ctx.object_arr = NULL;
    return 0;
}
static void stdio_close(const void* handle) {
    struct stdio_handle* stdio = ((struct stdio_handle*)handle);
    if (stdio->mem_ctx.object_arr)
        pspl_malloc_context_destroy(&stdio->mem_ctx);
    fclose(stdio->file);
}
static size_t stdio_len(const void* handle) {
    size_t prev = ftell(((struct stdio_handle*)handle)->file);
    fseek(((struct stdio_handle*)handle)->file, 0, SEEK_END);
    size_t len = ftell(((struct stdio_handle*)handle)->file);
    fseek(((struct stdio_handle*)handle)->file, prev, SEEK_SET);
    return len;
}
static int stdio_seek(const void* handle, size_t seek_set) {
    return fseek(((struct stdio_handle*)handle)->file, seek_set, SEEK_SET);
}
static size_t stdio_tell(const void* handle) {
    return ftell(((struct stdio_handle*)handle)->file);
}
static size_t stdio_read(const void* handle, size_t num_bytes, void** data_out) {
    struct stdio_handle* stdio = ((struct stdio_handle*)handle);
    if (!stdio->mem_ctx.object_arr)
        pspl_malloc_context_init(&stdio->mem_ctx);
#   if PSPL_RUNTIME_PLATFORM_GX
    void* buf = pspl_malloc_memalign(&stdio->mem_ctx, num_bytes, 32);
#   else
    void* buf = pspl_malloc(&stdio->mem_ctx, num_bytes);
#   endif
    *data_out = buf;
    return fread(buf, 1, num_bytes, stdio->file);
}
static size_t stdio_read_direct(const void* handle, size_t num_bytes, void* data_buf) {
    struct stdio_handle* stdio = ((struct stdio_handle*)handle);
    return fread(data_buf, 1, num_bytes, stdio->file);
}
static void stdio_duplicate_handle(pspl_dup_data_provider_handle_t* dup_handle, const void* handle) {
    struct stdio_handle* stdio = ((struct stdio_handle*)handle);
    struct stdio_handle* dup_stdio = ((struct stdio_handle*)dup_handle);
    dup_stdio->file = fopen(stdio->original_path, "r");
    dup_stdio->mem_ctx.object_arr = NULL;
}
static void stdio_destroy_duplicate_handle(pspl_dup_data_provider_handle_t* dup_handle) {
    struct stdio_handle* dup_stdio = ((struct stdio_handle*)dup_handle);
    fclose(dup_stdio->file);
    if (dup_stdio->mem_ctx.object_arr)
        pspl_malloc_context_destroy(&dup_stdio->mem_ctx);
}
static const pspl_data_provider_t stdio = {
    .open = stdio_open,
    .close = stdio_close,
    .len = stdio_len,
    .seek = stdio_seek,
    .tell = stdio_tell,
    .read = stdio_read,
    .read_direct = stdio_read_direct,
    .duplicate_handle = stdio_duplicate_handle,
    .destroy_duplicate_handle = stdio_destroy_duplicate_handle
};

/**
 * Load a PSPLP package file using `<stdio.h>` routines
 *
 * @param package_path path (absolute or relative to working directory)
 *        expressing location to PSPLP file
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_file(const char* package_path,
                                   const pspl_runtime_package_t** package_out) {
    pspl_runtime_package_t* package = pspl_malloc(&package_mem_ctx, sizeof(pspl_runtime_package_t));
    package->provider_hooks = &stdio;
    package->provider_local = 1;
    if (stdio.open(&package->provider.stdio, package_path)) {
        pspl_warn("Unable to load PSPLP", "unable to open `%s`", package_path);
        pspl_malloc_free(&package_mem_ctx, package);
        if (package_out)
            *package_out = NULL;
        return -1;
    }
    if (package_out)
        *package_out = package;
    int err = load_psplp(package);
    if (err) {
        stdio.close(&package->provider.stdio);
        pspl_malloc_free(&package_mem_ctx, package);
        if (package_out)
            *package_out = NULL;
    }
    return err;
}



/* membuf package file */

static int mem_open(void* handle, const char* path) {
    struct mem_handle* mem = ((struct mem_handle*)handle);
    mem->cur = mem->data;
    return 0;
}
static void mem_close(const void* handle) {}
static size_t mem_len(const void* handle) {
    return ((struct mem_handle*)handle)->len;
}
static int mem_seek(const void* handle, size_t seek_set) {
    struct mem_handle* mem = ((struct mem_handle*)handle);
    if (seek_set >= mem->len)
        return -1;
    mem->cur = mem->data + seek_set;
    return 0;
}
static size_t mem_tell(const void* handle) {
    struct mem_handle* mem = ((struct mem_handle*)handle);
    return mem->cur - mem->data;
}
static size_t mem_read(const void* handle, size_t num_bytes, void** data_out) {
    struct mem_handle* mem = ((struct mem_handle*)handle);
    size_t read_in = mem->cur - mem->data;
    if (read_in >= mem->len)
        return 0;
    size_t rem = mem->len - read_in;
    rem = (num_bytes < rem)?num_bytes:rem;
    *data_out = mem->cur;
    mem->cur += num_bytes;
    return rem;
}
static size_t mem_read_direct(const void* handle, size_t num_bytes, void* data_buf) {
    struct mem_handle* mem = ((struct mem_handle*)handle);
    size_t read_in = mem->cur - mem->data;
    if (read_in >= mem->len)
        return 0;
    size_t rem = mem->len - read_in;
    rem = (num_bytes < rem)?num_bytes:rem;
    memcpy(data_buf, mem->cur, rem);
    mem->cur += num_bytes;
    return rem;
}
static void mem_duplicate_handle(pspl_dup_data_provider_handle_t* dup_handle, const void* handle) {
    struct mem_handle* mem = ((struct mem_handle*)handle);
    struct mem_handle* dup_mem = ((struct mem_handle*)dup_handle);
    dup_mem->data = mem->data;
    dup_mem->cur = mem->data;
    dup_mem->len = mem->len;
}
static void mem_destroy_duplicate_handle(pspl_dup_data_provider_handle_t* dup_handle) {
    // Nothing to do; Just let handle go out of scope
}
static const pspl_data_provider_t membuf = {
    .open = mem_open,
    .close = mem_close,
    .len = mem_len,
    .seek = mem_seek,
    .tell = mem_tell,
    .read = mem_read,
    .read_direct = mem_read_direct,
    .duplicate_handle = mem_duplicate_handle,
    .destroy_duplicate_handle = mem_destroy_duplicate_handle
};

/**
 * Load a PSPLP package file from application-provided memory buffer
 *
 * @param package_data pointer to PSPLP data
 * @param package_len length of PSPLP buffer in memory
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_membuf(void* package_data, size_t package_len,
                                     const pspl_runtime_package_t** package_out) {
    if (!package_data)
        return -1;
    pspl_runtime_package_t* package = pspl_malloc(&package_mem_ctx, sizeof(pspl_runtime_package_t));
    package->provider_hooks = &membuf;
    package->provider_local = 1;
    package->provider.mem.data = package_data;
    package->provider.mem.len = package_len;
    if (membuf.open(&package->provider.mem, NULL)) {
        pspl_malloc_free(&package_mem_ctx, package);
        return -1;
    }
    if (package_out)
        *package_out = package;
    int err = load_psplp(package);
    if (err) {
        pspl_malloc_free(&package_mem_ctx, package);
        membuf.close(&package->provider.mem);
    }
    return err;
}


/* Generic provider package file */

/**
 * Load a PSPLP package file using data-providing hook routines provided by application
 *
 * @param package_path path expressing location to PSPLP file
 *        (supplied to 'open' hook)
 * @param data_provider_handle application-allocated handle object for use
 *        with provided hooks
 * @param data_provider_hooks application-populated data structure with hook
 *        implementations for data-handling routines
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_provider(const char* package_path,
                                       void* data_provider_handle,
                                       const pspl_data_provider_t* data_provider_hooks,
                                       const pspl_runtime_package_t** package_out) {
    if (!package_path || !data_provider_handle || !data_provider_hooks)
        return -1;
    pspl_runtime_package_t* package = pspl_malloc(&package_mem_ctx, sizeof(pspl_runtime_package_t));
    package->provider_hooks = data_provider_hooks;
    package->provider.provider = data_provider_handle;
    package->provider_local = 0;
    if (data_provider_hooks->open(data_provider_handle, package_path)) {
        pspl_malloc_free(&package_mem_ctx, package);
        return -1;
    }
    if (package_out)
        *package_out = package;
    int err = load_psplp(package);
    if (err) {
        pspl_malloc_free(&package_mem_ctx, package);
        data_provider_hooks->close(data_provider_handle);
    }
    return err;
}

/**
 * Unload PSPL package
 *
 * Frees all allocated objects represented through this package.
 * All extension and platform runtimes will be instructed to unload
 * their instances of the objects.
 *
 * @param package Package representation to unload
 */
void pspl_runtime_unload_package(const pspl_runtime_package_t* package) {
    if (!package)
        return;
    int i,j;
    for (i=0 ; i<package->psplc_count ; ++i) {
        const _pspl_runtime_psplc_t* obj = &package->psplc_array[i];
        _pspl_runtime_release_psplc((pspl_runtime_psplc_t*)obj, 1);
        for (j=0 ; j<obj->ext_arr_c ; ++j) {
            const _pspl_object_index_t* idx = &obj->ext_arr[j];
            pspl_free_indexing_block((void*)idx->h_arr);
            pspl_free_indexing_block((void*)idx->i_arr);
        }
        pspl_free_indexing_block((void*)obj->ext_arr);
        for (j=0 ; j<obj->plat_arr_c ; ++j) {
            const _pspl_object_index_t* idx = &obj->plat_arr[j];
            pspl_free_indexing_block((void*)idx->h_arr);
            pspl_free_indexing_block((void*)idx->i_arr);
        }
        pspl_free_indexing_block((void*)obj->plat_arr);
    }
    pspl_free_indexing_block((void*)package->psplc_array);
    for (i=0 ; i<package->file_count ; ++i) {
        const _pspl_runtime_arc_file_t* obj = &package->file_array[i];
        _pspl_runtime_release_archived_file((pspl_runtime_arc_file_t*)obj, 1);
    }
    pspl_free_indexing_block((void*)package->file_array);
    package->provider_hooks->close(PACKAGE_PROVIDER(package));
    pspl_malloc_free(&package_mem_ctx, (void*)package);
}


#pragma mark PSPLC Objects

/**
 * Count embedded PSPLCs within package
 *
 * @param package Package representation to count from
 * @return PSPLC count
 */
unsigned int pspl_runtime_count_psplcs(const pspl_runtime_package_t* package) {
    if (!package)
        return -1;
    return package->psplc_count;
}

/**
 * Enumerate embedded PSPLCs within package
 *
 * @param package Package representation to enumerate from
 * @param hook Function to call for each PSPLC
 */
void pspl_runtime_enumerate_psplcs(const pspl_runtime_package_t* package,
                                   pspl_runtime_enumerate_psplc_hook hook) {
    if (!package)
        return;
    int i;
    for (i=0 ; i<package->psplc_count ; ++i) {
        if ((hook(&package->psplc_array[i].public) < 0))
            break;
    }
}

/**
 * Get PSPLC representation from key string and optionally perform retain
 *
 * @param key Key-string to hash and use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_t* pspl_runtime_get_psplc_from_key(const pspl_runtime_package_t* package,
                                                            const char* key, int retain) {
    if (!package)
        return NULL;
    
    // Hash key
    pspl_hash_ctx_t hash_ctx;
    pspl_hash_init(&hash_ctx);
    pspl_hash_write(&hash_ctx, key, strlen(key));
    pspl_hash* result;
    pspl_hash_result(&hash_ctx, result);
    
    // Pass to hashed lookup
    return pspl_runtime_get_psplc_from_hash(package, result, retain);
    
}

/**
 * Get PSPLC representation from hash and optionally perform retain
 *
 * @param hash Hash to use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_t* pspl_runtime_get_psplc_from_hash(const pspl_runtime_package_t* package,
                                                             pspl_hash* hash, int retain) {
    if (!package)
        return NULL;
    
    // Perform lookup
    int i,j;
    _pspl_runtime_psplc_t* psplc = NULL;
    for (i=0 ; i<package->psplc_count ; ++i) {
        psplc = &package->psplc_array[i];
        for (j=0 ; j<(sizeof(pspl_hash)/4) ; ++j) {
            if (hash->w[j] != psplc->public.hash.w[j]) {
                psplc = NULL;
                break;
            }
        }
        if (psplc) {
            
            // Retain if requested
            if (retain)
                pspl_runtime_retain_psplc(&psplc->public);
            
            // Done
            return &psplc->public;
            
        }
    }
    
    return NULL;
    
}

/**
 * Increment reference-count of PSPLC representation
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_retain_psplc(const pspl_runtime_psplc_t* psplc) {
    if (!psplc)
        return;
    
    _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)psplc;
    const pspl_data_provider_t* package_provider = PACKAGE_PROVIDER(obj->public.parent);
    int i;
    
    // See if it needs loading
    if (!obj->ref_count) {
        ++obj->ref_count;
        
        // Load data objects
        psplc->parent->provider_hooks->seek(package_provider, obj->blobs_off);
        if (psplc->parent->provider_hooks == &membuf)
            psplc->parent->provider_hooks->read(package_provider, obj->blobs_len, &obj->blobs_buf);
        else {
            obj->blobs_buf = pspl_allocate_media_block(obj->blobs_len);
            psplc->parent->provider_hooks->read_direct(package_provider, obj->blobs_len, obj->blobs_buf);
        }
        
        // Run extension hooks
        pspl_api_set_load_state(PSPL_LOADING_EXT);
        for (i=0 ; i<obj->ext_arr_c ; ++i) {
            const _pspl_object_index_t* idx = &obj->ext_arr[i];
            const pspl_extension_t* extension = psplc->parent->ext_array[idx->extension_index];
            
            // Notify extension of loading
            pspl_api_set_load_subject_index(i);
            if (extension->runtime_extension && extension->runtime_extension->load_object_hook)
                extension->runtime_extension->load_object_hook((pspl_runtime_psplc_t*)psplc);
        }
        
        // Run platform hooks
        pspl_api_set_load_state(PSPL_LOADING_PLAT);
        for (i=0 ; i<obj->plat_arr_c ; ++i) {
            
            // Notify platform of loading
            pspl_api_set_load_subject_index(i);
            if (pspl_runtime_platform->runtime_platform && pspl_runtime_platform->runtime_platform->load_object_hook)
                pspl_runtime_platform->runtime_platform->load_object_hook((pspl_runtime_psplc_t*)psplc);
            
            break;
        }
        
        pspl_api_set_load_state(PSPL_LOADING_NONE);
        
    } else
        ++obj->ref_count;
    
}

/**
 * Decrement reference-count of PSPLC representation
 *
 * @param psplc PSPLC representation
 */
static void _pspl_runtime_release_psplc(pspl_runtime_psplc_t* psplc, int total) {
    if (!psplc)
        return;
    
    _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)psplc;
    if (!obj->ref_count)
        return;
    int i;
    
    // See if it needs unloading
    if (obj->ref_count == 1 || total) {
        --obj->ref_count;
        
        // Run extension hooks
        pspl_api_set_load_state(PSPL_LOADING_EXT);
        for (i=0 ; i<obj->ext_arr_c ; ++i) {
            const _pspl_object_index_t* idx = &obj->ext_arr[i];
            const pspl_extension_t* extension = psplc->parent->ext_array[idx->extension_index];
            
            // Notify extension of unloading
            pspl_api_set_load_subject_index(i);
            if (extension->runtime_extension && extension->runtime_extension->unload_object_hook)
                extension->runtime_extension->unload_object_hook(psplc);
        }
        
        // Run platform hooks
        pspl_api_set_load_state(PSPL_LOADING_PLAT);
        for (i=0 ; i<obj->plat_arr_c ; ++i) {
            
            // Notify platform of unloading
            pspl_api_set_load_subject_index(i);
            if (pspl_runtime_platform->runtime_platform && pspl_runtime_platform->runtime_platform->unload_object_hook)
                pspl_runtime_platform->runtime_platform->unload_object_hook(psplc);
            
            break;
        }
        
        pspl_api_set_load_state(PSPL_LOADING_NONE);
        
        // Free data buffer if allocated with stdio
        if (psplc->parent->provider_hooks != &membuf)
            pspl_free_media_block(obj->blobs_buf);
        obj->blobs_buf = NULL;
        
    } else
        --obj->ref_count;
    
    if (total)
        obj->ref_count = 0;
}
void pspl_runtime_release_psplc(const pspl_runtime_psplc_t* psplc) {
    if (!psplc)
        return;
    _pspl_runtime_release_psplc((pspl_runtime_psplc_t*)psplc, 0);
}


/**
 * Bind PSPLC representation to GPU (implicitly retains if unloaded).
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_bind_psplc(const pspl_runtime_psplc_t* psplc) {
    if (!psplc)
        return;
    
    _pspl_runtime_psplc_t* obj = (_pspl_runtime_psplc_t*)psplc;
    if (!obj->ref_count)
        pspl_runtime_retain_psplc(psplc);
    int i;
    
    // Run platform hooks
    pspl_api_set_load_state(PSPL_LOADING_PLAT);
    for (i=0 ; i<obj->plat_arr_c ; ++i) {
        
        // Notify platform of binding
        pspl_api_set_load_subject_index(i);
        if (pspl_runtime_platform->runtime_platform && pspl_runtime_platform->runtime_platform->bind_object_hook)
            pspl_runtime_platform->runtime_platform->bind_object_hook((pspl_runtime_psplc_t*)psplc);
        
        break;
    }
    
    // Run extension hooks
    pspl_api_set_load_state(PSPL_LOADING_EXT);
    for (i=0 ; i<obj->ext_arr_c ; ++i) {
        const _pspl_object_index_t* idx = &obj->ext_arr[i];
        const pspl_extension_t* extension = psplc->parent->ext_array[idx->extension_index];
        
        // Notify extension of binding
        pspl_api_set_load_subject_index(i);
        if (extension->runtime_extension && extension->runtime_extension->bind_object_hook)
            extension->runtime_extension->bind_object_hook((pspl_runtime_psplc_t*)psplc);
    }
    
    pspl_api_set_load_state(PSPL_LOADING_NONE);
    
}


#pragma mark Archived Files

/**
 * Count archived files within package
 *
 * @param package Package representation to count from
 * @return Archived file count
 */
unsigned int pspl_runtime_count_archived_files(const pspl_runtime_package_t* package) {
    if (!package)
        return 0;
    return package->file_count;
}

/**
 * Enumerate archived files within package
 *
 * @param package Package representation to enumerate from
 * @param hook Function to call for each file
 */
void pspl_runtime_enumerate_archived_files(const pspl_runtime_package_t* package,
                                           pspl_runtime_enumerate_archived_file_hook hook) {
    if (!package || !hook)
        return;
    int i;
    for (i=0 ; i<package->file_count ; ++i) {
        if ((hook(&package->file_array[i].public) < 0))
            break;
    }
}

/**
 * Get archived file from hash and optionally perform retain
 *
 * @param hash Hash to use to look up archived file
 * @param retain If non-zero, the archived file will have internal
 *        reference count set to 1 when found
 * @return File representation (or NULL if not available)
 */
const pspl_runtime_arc_file_t* pspl_runtime_get_archived_file_from_hash(const pspl_runtime_package_t* package,
                                                                        const pspl_hash* hash, int retain) {
    if (!package || !hash)
        return NULL;
    
    // Perform lookup
    int i,j;
    const _pspl_runtime_arc_file_t* file = NULL;
    for (i=0 ; i<package->file_count ; ++i) {
        file = &package->file_array[i];
        char hash_str[PSPL_HASH_STRING_LEN];
        pspl_hash_fmt(hash_str, &file->public.hash);
        for (j=0 ; j<(sizeof(pspl_hash)/4) ; ++j) {
            if (hash->w[j] != file->public.hash.w[j]) {
                file = NULL;
                break;
            }
        }
        if (file) {
            
            // Retain if requested
            if (retain)
                pspl_runtime_retain_archived_file(&file->public);
            
            // Done
            return &file->public;
            
        }
    }
    
    return NULL;
    
}

/**
 * Increment reference-count of archived file
 *
 * @param file File representation
 */
void pspl_runtime_retain_archived_file(const pspl_runtime_arc_file_t* file) {
    if (!file)
        return;
    
    _pspl_runtime_arc_file_t* obj = (_pspl_runtime_arc_file_t*)file;
    const pspl_data_provider_t* package_provider = PACKAGE_PROVIDER(file->parent);
    
    // See if it needs loading
    if (!obj->ref_count) {
        ++obj->ref_count;
        
        // Load data objects
        file->parent->provider_hooks->seek(package_provider, obj->file_off);
        if (file->parent->provider_hooks == &membuf)
            file->parent->provider_hooks->read(package_provider, obj->public.file_len, &obj->public.file_data);
        else {
            obj->public.file_data = pspl_allocate_media_block(obj->public.file_len);
            file->parent->provider_hooks->read_direct(package_provider, obj->public.file_len, obj->public.file_data);
        }
        
    } else
        ++obj->ref_count;
    
}

/**
 * Decrement reference-count of archived file
 *
 * @param file File representation
 */
static void _pspl_runtime_release_archived_file(const pspl_runtime_arc_file_t* file, int total) {
    if (!file)
        return;
    
    _pspl_runtime_arc_file_t* obj = (_pspl_runtime_arc_file_t*)file;
    if (!obj->ref_count)
        return;
    
    // See if it needs unloading
    if (obj->ref_count == 1 || total) {
        --obj->ref_count;
        
        // Free data buffer if allocated with stdio
        if (file->parent->provider_hooks != &membuf)
            pspl_free_media_block(obj->public.file_data);
        obj->public.file_data = NULL;
        
    } else
        --obj->ref_count;
    
    if (total)
        obj->ref_count = 0;
}
void pspl_runtime_release_archived_file(const pspl_runtime_arc_file_t* file) {
    if (!file)
        return;
    _pspl_runtime_release_archived_file(file, 0);
}

/**
 * Create duplicate, pre-seeked FILE pointer and length of archived file
 *
 * An advanced API to bypass the reference-counted loading mechanism of
 * pspl-rt and receive a FILE pointer ready to load data from disk directly
 *
 * The handle is duplicated so that it may be used from a different thread (if need be)
 *
 * @param file Archived file object
 * @param provider_hooks_out Hook structure used to access data
 * @param provider_handle_out File instance containing requested data (pre-seeked)
 * @param len_out Length of file record
 * @return 0 if successful, otherwise negative
 */
int pspl_runtime_access_archived_file(const pspl_runtime_arc_file_t* file,
                                      const pspl_data_provider_t** provider_hooks_out,
                                      pspl_dup_data_provider_handle_t* provider_handle_out,
                                      size_t* len_out) {
    if (!file || !provider_handle_out || !provider_hooks_out)
        return -1;
    const _pspl_runtime_arc_file_t* obj = (_pspl_runtime_arc_file_t*)file;
    *provider_hooks_out = file->parent->provider_hooks;
    const void* provider_handle = PACKAGE_PROVIDER(file->parent);
    (*provider_hooks_out)->duplicate_handle(provider_handle_out, provider_handle);
    if (len_out)
        *len_out = obj->public.file_len;
    (*provider_hooks_out)->seek(provider_handle_out, obj->file_off);
    return 0;
}


