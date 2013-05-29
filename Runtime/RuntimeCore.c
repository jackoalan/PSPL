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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <PSPL/PSPLExtension.h>
#include <PSPLInternal.h>
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



/* Active runtime extensions */
extern const pspl_extension_t* pspl_available_extensions[];

/* Active runtime platform */
extern const pspl_platform_t* pspl_available_platforms[];
#define pspl_runtime_platform pspl_available_platforms[0]

/* Are we using xterm? */
uint8_t xterm_colour = 0;


#pragma mark Terminal Utils

/* Way to get terminal width (for line wrapping) */
static int term_cols() {
#   if _WIN32
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
    
    size = backtrace(array, BACKTRACE_DEPTH);
    strings = backtrace_symbols(array, (int)size);
    
    for (i = 0; i < size; i++)
        fprintf(stderr, "%s\n", strings[i]);
    
    free(strings);
}
#endif

/* Runtime-wide error handler
 * uses a global context state to print context-sensitive error info */
void pspl_error(int exit_code, const char* brief, const char* msg, ...) {
    if (xterm_colour)
        fprintf(stderr, RED"*** ERROR ***\n"SGR0);
    else
        fprintf(stderr, "*** ERROR ***\n");
    
    brief = wrap_string(brief, 1);
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
        fprintf(stderr, MAGENTA"*** WARNING ***\n"SGR0);
    else
        fprintf(stderr, "*** WARNING ***\n");
    
    brief = wrap_string(brief, 1);
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


#pragma mark Extension/Platform Load API

/* These *must* be called within the `load` hook of platform or extension */

/* Get embedded data object for extension by key (to hash) */
const pspl_data_object_t* pspl_get_embedded_data_object_from_key(pspl_runtime_psplc_rep_t* object,
                                                                 const char* key) {
    
}

/* Get embedded object for extension by hash */
const pspl_data_object_t* pspl_get_embedded_data_object_from_hash(pspl_runtime_psplc_rep_t* object,
                                                                  pspl_hash* hash) {
    
}

/* Get embedded object for extension by index */
const pspl_data_object_t* pspl_get_embedded_data_object_from_index(pspl_runtime_psplc_rep_t* object,
                                                                   uint32_t index) {
    
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
        pspl_runtime_package_rep_t* package = package_mem_ctx.object_arr[i];
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
    
}


#pragma mark Packages

struct stdio_handle {
    pspl_malloc_context_t mem_ctx;
    FILE* file;
};

struct mem_handle {
    const void* data;
    const void* cur;
    size_t len;
};

/**
 * Package representation type
 */
struct _pspl_loaded_package {
    
    // PSPLP data provider
    union {
        struct stdio_handle stdio;
        struct mem_handle mem;
        const void* provider;
    } provider;
    uint8_t provider_local;
    
    // Data provider hooks
    const pspl_data_provider_t* provider_hooks;
    
    // Extension count and array
    unsigned int ext_count;
    const pspl_extension_t** ext_array;
    
};


/* Private routine to load PSPLP data */
static int load_psplp(pspl_runtime_package_rep_t* package) {
    
    // Read header chunk of data
    const void* header_data = NULL;
    package->provider_hooks->read(package->provider.provider,
                                  sizeof(pspl_header_t) +
                                  sizeof(pspl_off_header_bi_t) +
                                  sizeof(pspl_psplp_header_bi_t),
                                  &header_data);
    
    // Bind header
    const pspl_header_t* pspl_head = header_data;
    
    // Validate members
    if (memcmp(pspl_head->magic, "PSPL", 4)) {
        pspl_warn("Invalid magic on provided package", "Magic should be 'PSPL'");
        return -1;
    }
    
    
    
}


/* Various ways to load a PSPLP */


/* stdio.h package file */

static int stdio_open(void* handle, const char* path) {
    struct stdio_handle* stdio = ((struct stdio_handle*)handle);
    stdio->file = fopen(path, "r");
    if (stdio->file)
        return -1;
    pspl_malloc_context_init(&stdio->mem_ctx);
    return 0;
}
static void stdio_close(const void* handle) {
    struct stdio_handle* stdio = ((struct stdio_handle*)handle);
    pspl_malloc_context_destroy(&stdio->mem_ctx);
    fclose(stdio->file);
}
static size_t stdio_len(const void* handle) {
    size_t prev = ftell((FILE*)handle);
    fseek(((struct stdio_handle*)handle)->file, 0, SEEK_END);
    size_t len = ftell((FILE*)handle);
    fseek(((struct stdio_handle*)handle)->file, prev, SEEK_SET);
    return len;
}
static int stdio_seek(const void* handle, size_t seek_set) {
    return fseek(((struct stdio_handle*)handle)->file, seek_set, SEEK_SET);
}
static size_t stdio_read(const void* handle, size_t num_bytes, const void** data_out) {
    struct stdio_handle* stdio = ((struct stdio_handle*)handle);
    void* buf = pspl_malloc_malloc(&stdio->mem_ctx, num_bytes);
    *data_out = buf;
    return fread(buf, 1, num_bytes, stdio->file);
}
static const pspl_data_provider_t stdio = {
    .open = stdio_open,
    .close = stdio_close,
    .len = stdio_len,
    .seek = stdio_seek,
    .read = stdio_read
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
                                   const pspl_runtime_package_rep_t** package_out) {
    pspl_runtime_package_rep_t* package = pspl_malloc_malloc(&package_mem_ctx, sizeof(pspl_runtime_package_rep_t));
    package->provider_hooks = &stdio;
    package->provider_local = 1;
    if (stdio.open(&package->provider.stdio, package_path)) {
        pspl_malloc_free(&package_mem_ctx, package);
        return -1;
    }
    if (package_out)
        *package_out = package;
    int err = load_psplp(package);
    if (err) {
        pspl_malloc_free(&package_mem_ctx, package);
        stdio.close(&package->provider.stdio);
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
static size_t mem_read(const void* handle, size_t num_bytes, const void** data_out) {
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
static const pspl_data_provider_t membuf = {
    .open = mem_open,
    .close = mem_close,
    .len = mem_len,
    .seek = mem_seek,
    .read = mem_read
};

/**
 * Load a PSPLP package file from application-provided memory buffer
 *
 * @param package_data pointer to PSPLP data
 * @param package_len length of PSPLP buffer in memory
 * @param package_out Output pointer conveying package representation
 * @return 0 if successful, or negative otherwise
 */
int pspl_runtime_load_package_membuf(const void* package_data, size_t package_len,
                                     const pspl_runtime_package_rep_t** package_out) {
    pspl_runtime_package_rep_t* package = pspl_malloc_malloc(&package_mem_ctx, sizeof(pspl_runtime_package_rep_t));
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
                                       const pspl_runtime_package_rep_t** package_out) {
    pspl_runtime_package_rep_t* package = pspl_malloc_malloc(&package_mem_ctx, sizeof(pspl_runtime_package_rep_t));
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
void pspl_runtime_unload_package(const pspl_runtime_package_rep_t* package) {
    package->provider_hooks->close((package->provider_local)?
                                   &package->provider.provider:
                                   package->provider.provider);
    pspl_malloc_free(&package_mem_ctx, (void*)package);
}


#pragma mark PSPLC Objects

/**
 * Count embedded PSPLCs within package
 *
 * @param package Package representation to count from
 * @return PSPLC count
 */
unsigned int pspl_runtime_count_psplcs(const pspl_runtime_package_rep_t* package) {
    
}

/**
 * Enumerate embedded PSPLCs within package
 *
 * @param package Package representation to enumerate from
 * @param hook Function to call for each PSPLC
 */
void pspl_runtime_enumerate_psplcs(const pspl_runtime_package_rep_t* package,
                                   pspl_runtime_enumerate_psplc_hook hook) {
    
}

/**
 * Get PSPLC representation from key string and optionally perform retain
 *
 * @param key Key-string to hash and use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_rep_t* pspl_runtime_get_psplc_from_key(const char* key, int retain) {
    
}

/**
 * Get PSPLC representation from hash and optionally perform retain
 *
 * @param hash Hash to use to look up PSPLC representation
 * @param retain If non-zero, the PSPLC representation will have internal
 *        reference count set to 1 when found
 * @return PSPLC representation (or NULL if not available)
 */
const pspl_runtime_psplc_rep_t* pspl_runtime_get_psplc_from_hash(pspl_hash* hash, int retain) {
    
}

/**
 * Increment reference-count of PSPLC representation
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_retain_psplc(const pspl_runtime_psplc_rep_t* psplc) {
    
}

/**
 * Decrement reference-count of PSPLC representation
 *
 * @param psplc PSPLC representation
 */
void pspl_runtime_release_psplc(const pspl_runtime_psplc_rep_t* psplc) {
    
}


#pragma mark Archived Files

/**
 * Count archived files within package
 *
 * @param package Package representation to count from
 * @return Archived file count
 */
unsigned int pspl_runtime_count_archived_files(const pspl_runtime_package_rep_t* package) {
    
}

/**
 * Enumerate archived files within package
 *
 * @param package Package representation to enumerate from
 * @param hook Function to call for each file
 */
void pspl_runtime_enumerate_archived_files(const pspl_runtime_package_rep_t* package,
                                           pspl_runtime_enumerate_archived_file_hook hook) {
    
}

/**
 * Get archived file from hash and optionally perform retain
 *
 * @param hash Hash to use to look up archived file
 * @param retain If non-zero, the archived file will have internal
 *        reference count set to 1 when found
 * @return File representation (or NULL if not available)
 */
const pspl_runtime_arc_file_rep_t* pspl_runtime_get_archived_file_from_hash(pspl_hash* hash, int retain) {
    
}

/**
 * Increment reference-count of archived file
 *
 * @param file File representation
 */
void pspl_runtime_retain_archived_file(const pspl_runtime_arc_file_rep_t* file) {
    
}

/**
 * Decrement reference-count of archived file
 *
 * @param file File representation
 */
void pspl_runtime_release_archived_file(const pspl_runtime_arc_file_rep_t* file) {
    
}


