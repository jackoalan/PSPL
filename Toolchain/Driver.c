//
//  Driver.c
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//
#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

/* Should an error condition print backtrace? */
#ifndef _WIN32
#define PSPL_ERROR_PRINT_BACKTRACE 1
#endif

/* Should PSPL catch system signals as errors on its own? */
#define PSPL_ERROR_CATCH_SIGNALS 1

#include <stdio.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#if PSPL_ERROR_CATCH_SIGNALS
#include <signal.h>
#endif
#if PSPL_ERROR_PRINT_BACKTRACE
#include <execinfo.h>
#endif

#include <PSPL/PSPL.h>
#include <PSPL/PSPLExtension.h>
#include <PSPLInternal.h>

#include "Driver.h"
#include "Preprocessor.h"
#include "Compiler.h"
#include "Packager.h"


/* Maximum count of sources */
#define PSPL_MAX_SOURCES 128

/* Maximum count of defs */
#define PSPL_MAX_DEFS 128

/* Maximum source file size (512K) */
#define PSPL_MAX_SOURCE_SIZE (512*1024)


/* Global driver state */
pspl_toolchain_driver_state_t driver_state;


/* Current working directory */
static char cwd[MAXPATHLEN];

/* Are we using xterm? */
uint8_t xterm_colour = 0;


/* Putting these here */
void pspl_hash_fmt(char* out, const pspl_hash* hash) {
    int i;
    for (i=0 ; i<sizeof(pspl_hash) ; ++i) {
        sprintf(out, "%02X", (unsigned int)(hash->hash[i]));
        out += 2;
    }
}
void pspl_hash_parse(pspl_hash* out, const char* hash_str) {
    int i;
    char byte_str[3];
    byte_str[2] = '\0';
    for (i=0 ; i<sizeof(pspl_hash) ; ++i) {
        strncpy(byte_str, hash_str, 2);
        out->hash[i] = (uint8_t)strtol(byte_str, NULL, 16);
        hash_str += 2;
    }
}


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
    char* cpy_str = malloc(1024);
    char* result_str = malloc(1024);
    
    // Copy string
    strcpy(cpy_str, str_in);
    
    // Initialise space-tokeniser
    char* save_ptr;
    char* word_str = strtok_r(cpy_str, " ", &save_ptr);
    
    // Enumerate words (and keep track of accumulated characters)
    int cur_line_col = 0;
    int cur_line = 0;
    
    // All-indent
    if (indent == 2) {
        cur_line_col = 6;
        strcat(result_str, "      ");
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
                strcat(result_str, "\n      ");
            } else {
                cur_line_col = 0;
                strcat(result_str, "\n");
            }
            avail_line_space = num_cols-cur_line_col;
            //if (!cur_line)
            //    num_cols += 2;
            ++cur_line;
        }
        
        // Handle case where one word is longer than a line (maybe it's German)
        if (word_length >= avail_line_space) {
            char truncated_word[512];
            truncate_string(word_str, (avail_line_space>500)?500:avail_line_space, truncated_word);
            strcat(result_str, truncated_word);
            cur_line_col = num_cols;
        } else {
            strcat(result_str, word_str);
            strcat(result_str, " ");
            cur_line_col += word_length+1;
        }
        
        word_str = strtok_r(save_ptr, " ", &save_ptr);
    }
    
    free(cpy_str);
    char* last_space = strrchr(result_str, ' ');
    if (last_space)
        *last_space = '\0';
    return result_str;
    
}


#pragma mark Built-in Help

static void print_help(const char* prog_name) {
    
    if (xterm_colour) {
        
        // We have xterm colour support!
        
        fprintf(stderr,
                BOLD RED "PSPL Toolchain\n"
                BOLD MAGENTA "By Jack Andersen "GREEN"<jackoalan@gmail.com>\n"
                UNDERLINE CYAN "https://github.com/jackoalan/PSPL\n" NORMAL
                "\n"
                "\n"
                BOLD"Available Extensions:\n"NORMAL);
        pspl_extension_t* ext = NULL;
        int i = 0;
        while ((ext = pspl_available_extensions[i++])) {
            fprintf(stderr, "  "BOLD"* %s"NORMAL" - %s\n",
                    ext->extension_name, ext->extension_desc);
        }
        if (i==1)
            fprintf(stderr, "  "BOLD"-- No extensions available --"NORMAL"\n");
        fprintf(stderr,
                "\n"
                "\n"
                BOLD"Available Target Platforms:\n"NORMAL);
        pspl_platform_t* plat = NULL;
        i = 0;
        while ((plat = pspl_available_target_platforms[i++])) {
            fprintf(stderr, "  "BOLD"* %s"NORMAL" - %s",
                    plat->platform_name, plat->platform_desc);
            if (plat == pspl_default_target_platform)
                fprintf(stderr, " "BOLD"[DEFAULT]"NORMAL);
            fprintf(stderr, "\n");
        }
        if (i==1)
            fprintf(stderr, "  "BOLD"-- No target platforms available --"NORMAL"\n");
        fprintf(stderr,
                "\n"
                "\n");
        
        // Now print usage info
        fprintf(stderr, BOLD BLUE"Command Synopsis:\n"NORMAL);
        const char* help =
        wrap_string("pspl ["BOLD"-o"NORMAL" "UNDERLINE"out-path"NORMAL"] ["BOLD"-E"NORMAL"|"BOLD"-c"NORMAL"] ["BOLD"-G"NORMAL" "UNDERLINE"reflist-out-path"NORMAL"] ["BOLD"-S"NORMAL" "UNDERLINE"staging-root-path"NORMAL"] ["BOLD"-D"NORMAL" "UNDERLINE"def-name"NORMAL"[="UNDERLINE"def-value"NORMAL"]]... ["BOLD"-T"NORMAL" "UNDERLINE"target-platform"NORMAL"]... ["BOLD"-e"NORMAL" <"UNDERLINE"LITTLE"NORMAL", "UNDERLINE"BIG"NORMAL", "UNDERLINE"BI"NORMAL">] "UNDERLINE"source1"NORMAL" ["UNDERLINE"source2"NORMAL" ["UNDERLINE"sourceN"NORMAL"]]...", 1);
        fprintf(stderr, "%s\n\n\n", help);
        free((char*)help);
        
    } else {
        
        // No colour support
        
        fprintf(stderr,
                "PSPL Toolchain\n"
                "By Jack Andersen <jackoalan@gmail.com>\n"
                "https://github.com/jackoalan/PSPL\n"
                "\n"
                "\n"
                "Available Extensions:\n");
        pspl_extension_t* ext = NULL;
        int i = 0;
        while ((ext = pspl_available_extensions[i++])) {
            fprintf(stderr, "  * %s - %s\n",
                    ext->extension_name, ext->extension_desc);
        }
        if (i==1)
            fprintf(stderr, "  -- No extensions available --\n");
        fprintf(stderr,
                "\n"
                "\n"
                "Available Target Platforms:\n");
        pspl_platform_t* plat = NULL;
        i = 0;
        while ((plat = pspl_available_target_platforms[i++])) {
            fprintf(stderr, "  * %s - %s",
                    plat->platform_name, plat->platform_desc);
            if (plat == pspl_default_target_platform)
                fprintf(stderr, " [DEFAULT]");
            fprintf(stderr, "\n");
        }
        if (i==1)
            fprintf(stderr, "  -- No target platforms available --\n");
        fprintf(stderr,
                "\n"
                "\n");
        
        // Now print usage info
        fprintf(stderr, "Command Synopsis:\n");
        const char* help =
        wrap_string("pspl [-o out-path] [-E|-c] [-G reflist-out-path] [-S staging-root-path] [-D def-name[=def-value]]... [-T target-platform]... source1 [source2 [sourceN]]...", 1);
        fprintf(stderr, "%s\n\n\n", help);
        free((char*)help);
        
    }
}


#pragma mark Error Handling

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

/* Toolchain-wide error handler
 * uses a global context state to print context-sensitive error info */
void pspl_error(int exit_code, const char* brief, const char* msg, ...) {
    brief = wrap_string(brief, 1);
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
    }
    
    char err_head_buf[256] = {0};
    char* err_head = err_head_buf;
    if (xterm_colour) {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string(BOLD RED"ERROR WHILE "UNDERLINE"INITIALISING"NORMAL BOLD RED" TOOLCHAIN:\n"SGR0, 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                snprintf(err_head, 255, BOLD RED"ERROR WHILE "UNDERLINE"PREPROCESSING"NORMAL BOLD BLUE" `%s`"MAGENTA" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num+1);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                snprintf(err_head, 255, BOLD RED"ERROR WHILE "CYAN"COMPILING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                snprintf(err_head, 255, BOLD RED"ERROR WHILE "CYAN"PACKAGING "BLUE"`%s`\n"SGR0,
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "%s", err_head);
        if (err_head != err_head_buf)
            free(err_head);
        fprintf(stderr, BOLD"%s:\n"SGR0, brief);
    } else {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string("ERROR WHILE INITIALISING TOOLCHAIN:\n", 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                snprintf(err_head, 255, "ERROR WHILE PREPROCESSING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                snprintf(err_head, 255, "ERROR WHILE COMPILING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                snprintf(err_head, 255, "ERROR WHILE PACKAGING `%s`\n",
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "%s", err_head);
        if (err_head != err_head_buf)
            free(err_head);
        fprintf(stderr, "%s:\n", brief);
    }
    
    fprintf(stderr, "%s\n", msg_str);
    
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

/* Toolchain-wide warning handler
 * uses a global context state to print context-sensitive error info */
void pspl_warn(const char* brief, const char* msg, ...) {
    brief = wrap_string(brief, 1);
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
    }
    
    char err_head_buf[256] = {0};
    char* err_head = err_head_buf;
    if (xterm_colour) {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string(BOLD YELLOW"WARNING WHILE "CYAN"INITIALISING"YELLOW" TOOLCHAIN:\n"SGR0, 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                snprintf(err_head, 255, BOLD YELLOW"WARNING WHILE "CYAN"PREPROCESSING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                snprintf(err_head, 255, BOLD YELLOW"WARNING WHILE "CYAN"COMPILING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                snprintf(err_head, 255, BOLD RED"WARNING WHILE "CYAN"PACKAGING "BLUE"`%s`\n"SGR0,
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "%s", err_head);
        if (err_head != err_head_buf)
            free(err_head);
        fprintf(stderr, BOLD"%s:\n"SGR0, brief);
    } else {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string("WARNING WHILE INITIALISING TOOLCHAIN:\n", 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                snprintf(err_head, 255, "WARNING WHILE PREPROCESSING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                snprintf(err_head, 255, "WARNING WHILE COMPILING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                snprintf(err_head, 255, "WARNING WHILE PACKAGING `%s`\n",
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "%s", err_head);
        if (err_head != err_head_buf)
            free(err_head);
        fprintf(stderr, "%s:\n", brief);
    }
    
    fprintf(stderr, "%s\n", msg_str);
}

/* Report PSPLC read-in underflow */
void check_psplc_underflow(pspl_toolchain_driver_psplc_t* psplc, const void* cur_ptr) {
    size_t delta = cur_ptr-(void*)psplc->psplc_data;
    if (delta > psplc->psplc_data_len)
        pspl_error(-1, "PSPLC underflow detected",
                   "PSPLC `%s` is not long enough to load needed data @0x%x (%u)",
                   psplc->file_path, delta, (unsigned)delta);
}


#pragma mark Driver State Helpers

/* Insert def into driver opts */
static void add_def(pspl_toolchain_driver_opts_t* driver_opts,
                    const char* def) {
    if (driver_opts->def_c >= PSPL_MAX_DEFS) {
        pspl_error(-1, "Definitions exceeded maximum count",
                   "up to %u defs supported", (unsigned int)PSPL_MAX_DEFS);
    }
    
    driver_opts->def_k[driver_opts->def_c] = def;
    
    char* eq_ptr = strchr(def, '=');
    if (eq_ptr) {
        *eq_ptr = '\0';
        ++eq_ptr;
        if (*eq_ptr)
            driver_opts->def_v[driver_opts->def_c] = eq_ptr;
    }
    
    ++driver_opts->def_c;
}

/* Insert target platform into driver opts */
static pspl_platform_t* pspl_platforms[PSPL_MAX_PLATFORMS];
static void add_target_platform(pspl_toolchain_driver_opts_t* driver_opts,
                                const char* name) {
    if (driver_opts->platform_c >= PSPL_MAX_PLATFORMS) {
        pspl_error(-1, "Platforms exceeded maximum count",
                   "up to %u platforms supported", (unsigned int)PSPL_MAX_PLATFORMS);
    }
    
    // Look up platform
    pspl_platform_t* plat = NULL;
    int i = 0;
    while ((plat = pspl_available_target_platforms[i++])) {
        if (!strcmp(plat->platform_name, name)) {
            pspl_platforms[driver_opts->platform_c] = plat;
            break;
        }
    }
    
    if (!plat)
        pspl_error(-1, "Unable to find requested platform",
                   "requested `%s`", name);
    
    ++driver_opts->platform_c;
}

/* Accept default endianness argument */
static void set_default_endianness(pspl_toolchain_driver_opts_t* driver_opts,
                                   const char* arg) {
    
    if (!strcasecmp(arg, "little"))
        driver_opts->default_endianness = PSPL_LITTLE_ENDIAN;
    else if (!strcasecmp(arg, "big"))
        driver_opts->default_endianness = PSPL_BIG_ENDIAN;
    else if (!strcasecmp(arg, "bi"))
        driver_opts->default_endianness = PSPL_BI_ENDIAN;
    else
        pspl_error(-1, "Invalid default endianness",
                   "'%s' is not a valid endianness; please use one of [LITTLE, BIG, BI]", arg);
    
}

/* Lookup target extension by name */
static pspl_extension_t* lookup_ext(const char* ext_name, unsigned int* idx_out) {
    pspl_extension_t* ext = NULL;
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

/* Lookup target platform by name */
static pspl_platform_t* lookup_target_platform(const char* plat_name) {
    pspl_platform_t* plat = NULL;
    unsigned int i = 0;
    while ((plat = pspl_available_target_platforms[i++])) {
        if (!strcmp(plat->platform_name, plat_name))
            break;
    }
    return plat;
}

/* Initialise psplc structure from file */
#define IS_PSPLC_BI pspl_head->endian_flags == PSPL_BI_ENDIAN
#if __LITTLE_ENDIAN__
#define IS_PSPLC_SWAPPED pspl_head->endian_flags == PSPL_BIG_ENDIAN
#elif __BIG_ENDIAN__
#define IS_PSPLC_SWAPPED pspl_head->endian_flags == PSPL_LITTLE_ENDIAN
#endif
static void init_psplc_from_file(pspl_toolchain_driver_psplc_t* psplc, const char* path) {
    int j;
    
    // Populate filename members
    psplc->file_path = path;
    
    // Make path absolute (if it's not already)
    const char* abs_path = psplc->file_path;
    if (abs_path[0] != '/') {
        abs_path = malloc(MAXPATHLEN);
        sprintf((char*)abs_path, "%s/%s", cwd, psplc->file_path);
    }
    
    // Enclosing dir
    char* last_slash = strrchr(abs_path, '/');
    psplc->file_enclosing_dir = malloc(MAXPATHLEN);
    sprintf((char*)psplc->file_enclosing_dir, "%.*s", (int)(last_slash-abs_path+1), abs_path);
    
    // File name
    psplc->file_name = last_slash+1;
    
    
    // Load PSPLC object data
    FILE* psplc_file = fopen(path, "r");
    fseek(psplc_file, 0, SEEK_END);
    long psplc_len = ftell(psplc_file);
    fseek(psplc_file, 0, SEEK_SET);
    if (psplc_len > PSPL_MAX_SOURCE_SIZE)
        pspl_error(-1, "PSPLC file exceeded filesize limit",
                   "PSPLC file `%s` is %l bytes in length; exceeding %u byte limit",
                   path, psplc_len, (unsigned)PSPL_MAX_SOURCE_SIZE);
    uint8_t* psplc_buf = malloc(psplc_len+1);
    if (!psplc_buf)
        pspl_error(-1, "Unable to allocate memory buffer for PSPLC",
                   "errno %d - `%s`", errno, strerror(errno));
    size_t read_len = fread(psplc_buf, 1, psplc_len, psplc_file);
    psplc_buf[psplc_len] = '\0';
    psplc->psplc_data = psplc_buf;
    psplc->psplc_data_len = psplc_len;
    fclose(psplc_file);
    if (read_len != psplc_len)
        pspl_error(-1, "Didn't read expected amount from PSPLC",
                   "expected %u bytes; read %u bytes", psplc_len, read_len);
    
    // Verify file size (at least a header in length)
    if (psplc->psplc_data_len < sizeof(pspl_header_t))
        pspl_error(-1, "PSPLC file too short",
                   "`%s` is not a valid PSPLC file", psplc->file_path);
    
    // Set Header and verify magic
    const pspl_header_t* pspl_head = (pspl_header_t*)psplc->psplc_data;
    if (memcmp(pspl_head->magic, "PSPL", 4))
        pspl_error(-1, "Invalid PSPLC magic",
                   "`%s` is not a valid PSPLC file", psplc->file_path);
    // Verify version
    if (pspl_head->version != 1)
        pspl_error(-1, "Invalid PSPLC version",
                   "file `%s` is version %u; expected version %u",
                   psplc->file_path, pspl_head->version, PSPL_VERSION);
    // Verify not package
    if (pspl_head->package_flag)
        pspl_error(-1, "Unable to import PSPLP packages",
                   "`%s` is a PSPLP package, not a PSPLC object file", psplc->file_path);
    // Verify endianness
    if (!(pspl_head->endian_flags & 0x3))
        pspl_error(-1, "Invalid endian flag",
                   "`%s` has invalid endian flag", psplc->file_path);
    
    
    // Parse offset header
    pspl_off_header_t* pspl_off_head = NULL;
    if (IS_PSPLC_BI) {
        pspl_off_header_bi_t* bi_head = (pspl_off_header_bi_t*)(psplc->psplc_data + sizeof(pspl_header_t));
        pspl_off_head = &bi_head->native;
    } else {
        pspl_off_head = (pspl_off_header_t*)(psplc->psplc_data + sizeof(pspl_header_t));
        if (IS_PSPLC_SWAPPED)
            SWAP_PSPL_OFF_HEADER_T(pspl_off_head);
    }
    
    // Populate extension set
    unsigned int psplc_extension_count = pspl_off_head->extension_name_table_c;
    unsigned int psplc_extension_off = pspl_off_head->extension_name_table_off;
    const char* psplc_extension_name = (char*)(psplc->psplc_data + psplc_extension_off);
    psplc->required_extension_set = calloc(psplc_extension_count+1, sizeof(pspl_extension_t*));
    for (j=0 ; j<psplc_extension_count ; ++j) {
        check_psplc_underflow(psplc, psplc_extension_name);
        
        // Lookup
        const pspl_extension_t* ext = lookup_ext(psplc_extension_name, NULL);
        if (!ext)
            pspl_error(-1, "PSPLC-required extension not available",
                       "PSPLC `%s` requested `%s` which is not available in this build of PSPL",
                       psplc->file_path, psplc_extension_name);
        
        // Ensure this isn't a redundant add (set enforcement)
        /*
        for (k=0 ; k<j ; ++k) {
            if (psplc->required_extension_set[k] == ext) {
                --j;
                --psplc_extension_count;
                break;
            }
        }
        if (k<j)
            continue;
         */
        
        // Add to set
        psplc->required_extension_set[j] = ext;
        
        // Advance
        psplc_extension_name += strlen(psplc_extension_name) + 1;
    }
    psplc->required_extension_set[psplc_extension_count] = NULL;
    
    
    // Populate platform set
    unsigned int psplc_platform_count = pspl_off_head->platform_name_table_c;
    unsigned int psplc_platform_off = pspl_off_head->platform_name_table_off;
    const char* psplc_platform_name = (char*)(psplc->psplc_data + psplc_platform_off);
    psplc->required_platform_set = calloc(psplc_platform_count+1, sizeof(pspl_platform_t*));
    for (j=0 ; j<psplc_platform_count ; ++j) {
        check_psplc_underflow(psplc, psplc_platform_name);
        
        // Lookup
        const pspl_platform_t* plat = lookup_target_platform(psplc_platform_name);
        if (!plat)
            pspl_error(-1, "PSPLC-required target platform not available",
                       "PSPLC `%s` requested `%s` which is not available in this build of PSPL",
                       psplc->file_path, psplc_platform_name);
        
        // Ensure this isn't a redundant add (set enforcement)
        /*
        for (k=0 ; k<j ; ++k) {
            if (psplc->required_platform_set[k] == plat) {
                --j;
                --psplc_platform_count;
                break;
            }
        }
        if (k<j)
            continue;
         */
        
        // Add to set
        psplc->required_platform_set[j] = plat;
        
        // Advance
        psplc_platform_name += strlen(psplc_platform_name) + 1;
    }
    psplc->required_platform_set[psplc_platform_count] = NULL;
}


#pragma mark Public Init Other Extension Request

/* Get/set/unset bit state of extension init index */
#define GET_INIT_BIT(idx) ((driver_state.ext_init_bits[(idx)/0x20] >> (idx)%0x20) & 0x1)
#define SET_INIT_BIT(idx) (driver_state.ext_init_bits[(idx)/0x20] |= (0x1 << (idx)%0x20))
#define UNSET_INIT_BIT(idx) (driver_state.ext_init_bits[(idx)/0x20] &= ~(0x1 << (idx)%0x20))

/* Request the immediate initialisation of another extension
 * (only valid within init hook) */
int pspl_toolchain_init_other_extension(const char* ext_name) {
    if (driver_state.pspl_phase != PSPL_PHASE_INIT_EXTENSION)
        return -1;
    
    // Lookup extension
    unsigned int idx;
    const pspl_extension_t* ext = lookup_ext(ext_name, &idx);
    if (!ext) {
        pspl_warn("Unable to locate extension",
                  "extension '%s' requested initialising '%s' early; latter not available in this build of PSPL",
                  driver_state.proc_extension->extension_name, ext_name);
        return -1;
    }
    
    // Run init hook of other extension if its not yet initialised
    if (ext->toolchain_extension && ext->toolchain_extension->init_hook && !GET_INIT_BIT(idx)) {
        const pspl_extension_t* save_ext = driver_state.proc_extension;
        driver_state.proc_extension = ext;
        int err;
        err = ext->toolchain_extension->init_hook(driver_state.tool_ctx);
        if (err)
            pspl_error(-1, "Extension failed to init",
                       "extension '%s' returned %d error code when requested to initialise early by '%s'",
                       ext->extension_name, err, save_ext->extension_name);
        driver_state.proc_extension = save_ext;
        SET_INIT_BIT(idx);
    }
    
    return 0;
}


#if PSPL_ERROR_CATCH_SIGNALS
static void catch_sig(int sig) {
#   ifdef _WIN32
    pspl_error(-1, "Caught signal", "PSPL caught signal %d", sig);
#   else
    pspl_error(-1, "Caught signal", "PSPL caught signal %d (%s)", sig, sys_siglist[sig]);
#   endif
}
#endif


#pragma mark Main Driver Routine

int main(int argc, char** argv) {
    
#   if PSPL_ERROR_CATCH_SIGNALS
    // Register signal handler (throws pspl error with backtrace)
    int i;
    for (i=1 ; i<NSIG ; ++i)
        signal(i, catch_sig);
#   endif
    
    // Initial driver state
    driver_state.pspl_phase = PSPL_PHASE_INIT;
    
    // Get CWD
    if(!getcwd((char*)cwd, MAXPATHLEN))
        pspl_error(-1, "Unable to get current working directory",
                   "errno %d - `%s`", errno, strerror(errno));
    
    
    // Check for xterm
    const char* term_type = getenv("TERM");
    if (term_type && strlen(term_type) >= 5 && !strncmp("xterm", term_type, 5))
        xterm_colour = 1;
    
    
    // Start recording driver options
    pspl_toolchain_driver_opts_t driver_opts;
    memset(&driver_opts, 0, sizeof(driver_opts));
    driver_opts.source_a = calloc(PSPL_MAX_SOURCES, sizeof(const char*));
    driver_opts.def_k = calloc(PSPL_MAX_DEFS, sizeof(const char*));
    driver_opts.def_v = calloc(PSPL_MAX_DEFS, sizeof(const char*));
    
    // We need at least one argument
    if (!argc || argc == 1) {
        print_help(argv[0]);
        return 0;
    }
    
    // Default endianness
#   if __LITTLE_ENDIAN__
    driver_opts.default_endianness = PSPL_LITTLE_ENDIAN;
#   elif __BIG_ENDIAN__
    driver_opts.default_endianness = PSPL_BIG_ENDIAN;
#   endif
    
    // Initial argument pass
    char expected_arg = 0;
    for (i=1 ; i<argc ; ++i) {
        if (!expected_arg && argv[i][0] == '-') { // Process flag argument
            char token_char = argv[i][1];
            
            if (token_char == 'h') {
                
                print_help(argv[0]);
                return 0;
                
            } else if (token_char == 'o') {
                
                if (argv[i][2])
                    driver_opts.out_path = &argv[i][2];
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'E') {
                
                expected_arg = 0;
                driver_opts.pspl_mode_opts |= PSPL_MODE_PREPROCESS_ONLY;
                
            } else if (token_char == 'c') {
                
                expected_arg = 0;
                driver_opts.pspl_mode_opts |= PSPL_MODE_COMPILE_ONLY;
                
            } else if (token_char == 'G') {
                
                if (argv[i][2])
                    driver_opts.reflist_out_path = &argv[i][2];
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'S') {
                
                if (argv[i][2])
                    driver_opts.staging_path = &argv[i][2];
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'D') {
                
                if (argv[i][2])
                    add_def(&driver_opts, &argv[i][2]);
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'T') {
                
                if (argv[i][2])
                    add_target_platform(&driver_opts, &argv[i][2]);
                else
                    expected_arg = token_char;
                
            } else if (token_char == 'e') {
                
                if (argv[i][2])
                    set_default_endianness(&driver_opts, &argv[i][2]);
                else
                    expected_arg = token_char;
                
            } else
                pspl_error(-1, "Unrecognised argument flag",
                           "`-%c` flag not recognised by PSPL", token_char);
            
        } else { // Process string argument
            char* str_arg = argv[i];
            
            if (expected_arg) { // Process a mode-string
                
                if (expected_arg == 'o') {
                    
                    // Output path
                    driver_opts.out_path = str_arg;
                    
                } else if (expected_arg == 'G') {
                    
                    // Reference gathering
                    driver_opts.reflist_out_path = str_arg;
                    
                } else if (expected_arg == 'S') {
                    
                    // Staging area
                    driver_opts.staging_path = str_arg;
                    
                } else if (expected_arg == 'D') {
                    
                    // Add definition
                    add_def(&driver_opts, str_arg);
                    
                } else if (expected_arg == 'T') {
                    
                    // Add target platform
                    add_target_platform(&driver_opts, str_arg);
                    
                } else if (expected_arg == 'e') {
                    
                    set_default_endianness(&driver_opts, str_arg);
                    
                }
                
                expected_arg = 0;
                    
                
            } else { // Add a source input
                
                // Add source
                if (driver_opts.def_c >= PSPL_MAX_SOURCES) {
                    pspl_error(-1, "Sources exceeded maximum count",
                               "up to %u sources supported", (unsigned int)PSPL_MAX_SOURCES);
                }
                
                driver_opts.source_a[driver_opts.source_c] = str_arg;
                
                ++driver_opts.source_c;
                
            }
            
        }
    }
    
    // We can't compile *and* preprocess
    if ((driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY) &&
        (driver_opts.pspl_mode_opts & PSPL_MODE_COMPILE_ONLY)) {
        pspl_error(-1, "Impossible task",
                   "PSPL can't preprocess-only (-E) *and* compile-only (-c) simultaneously");
        return -1;
    }
    
    
    // Ensure *all* provided files and paths exist
    
    // Sources
    for (i=0;i<driver_opts.source_c;++i) {
        
        // Verify file extension
        const char* file_ext = strrchr(driver_opts.source_a[i], '.') + 1;
        if (!file_ext || (strcasecmp(file_ext, "pspl") && strcasecmp(file_ext, "psplc")))
            pspl_error(-1, "Source file extension matters!!",
                       "please provide PSPL sources with '.pspl' extension and compiled objects with '.psplc' extension; `%s` does not follow this convention", driver_opts.source_a[i]);
        if (!strcasecmp(file_ext, "psplc") && ((driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY) ||
                                               (driver_opts.pspl_mode_opts & PSPL_MODE_COMPILE_ONLY)))
            pspl_error(-1, "Unable to accept PSPLC",
                       "unable to accept `%s` when in compile-only or preprocess-only mode",
                       driver_opts.source_a[i]);
        
        FILE* file;
        if ((file = fopen(driver_opts.source_a[i], "r")))
            fclose(file);
        else
            pspl_error(-1, "Unable to open provided source", "Can't open `%s` for reading",
                       driver_opts.source_a[i]);
    }
    
    // Reflist (just the path)
    if (driver_opts.reflist_out_path) {
        FILE* file;
        if ((file = fopen(driver_opts.reflist_out_path, "r")))
            fclose(file);
        else if ((file = fopen(driver_opts.reflist_out_path, "w")))
            fclose(file);
        else
            pspl_error(-1, "Unable to write reflist", "Can't open `%s` for writing",
                       driver_opts.reflist_out_path);
    }
    
    // Output file
    //driver_state.out_path = driver_opts.out_path;
    if (driver_opts.out_path) {
        if (driver_opts.out_path[0] != '-') {
            FILE* file;
            if ((file = fopen(driver_opts.out_path, "w")))
                fclose(file);
            else
                pspl_error(-1, "Unable to write output", "Can't open `%s` for writing",
                           driver_opts.out_path);
        }
    }
    
    // Staging area path (make cwd if not specified)
    if (!driver_opts.staging_path)
        driver_opts.staging_path = cwd;
    
    // Stat path to ensure it's a directory
    struct stat s;
    if (stat(driver_opts.staging_path, &s))
        pspl_error(-1, "Unable to `stat` requested staging path",
                   "`%s`; errno %d - `%s`", driver_opts.staging_path,
                   errno, strerror(errno));
    else if (!(s.st_mode & S_IFDIR)) {
        pspl_error(-1, "Unable to use requested staging path",
                   "`%s`; path not a directory", driver_opts.staging_path);
    }
    snprintf(driver_state.staging_path, MAXPATHLEN, "%s/PSPLFiles/", driver_opts.staging_path);
    
    // Set target platform array ref
    driver_opts.platform_a = (const pspl_platform_t* const *)pspl_platforms;
    
    // Populate toolchain context
    pspl_toolchain_context_t tool_ctx = {
        .target_runtime_platforms_c = driver_opts.platform_c,
        .target_runtime_platforms = driver_opts.platform_a,
        .def_c = driver_opts.def_c,
        .def_k = driver_opts.def_k,
        .def_v = driver_opts.def_v
    };
    driver_state.tool_ctx = &tool_ctx;
    
    
    // Reference gathering context (if requested)
    driver_state.gather_ctx = NULL;
    if (!(driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY) &&
        driver_opts.reflist_out_path) {
        pspl_gatherer_context_t gatherer;
        pspl_gather_load_cmake(&gatherer, driver_opts.reflist_out_path);
        driver_state.gather_ctx = &gatherer;
    }
    
    
#   pragma mark Available Extension Count
    
    // Extension count
    driver_state.ext_count = 0;
    pspl_extension_t** ext_counter = pspl_available_extensions;
    while (*(ext_counter++))
        ++driver_state.ext_count;
    
    // Allocate extension init bits
    size_t ext_init_bits_words = driver_state.ext_count / 0x20 + (driver_state.ext_count&0x1f)?1:0;
    driver_state.ext_init_bits = calloc(ext_init_bits_words, sizeof(uint32_t));
    
    
#   pragma mark Begin Toolchain Process
    
    // Make packager context (if packaging)
    pspl_packager_context_t packager_ctx;
    if (!(driver_opts.pspl_mode_opts & (PSPL_MODE_PREPROCESS_ONLY|PSPL_MODE_COMPILE_ONLY)))
        pspl_packager_init(&packager_ctx, driver_state.ext_count, driver_opts.platform_c, driver_opts.source_c);
    
    // Initialise object indexer for each input file
    pspl_indexer_context_t* indexers = calloc(driver_opts.source_c, sizeof(pspl_indexer_context_t));
    
    // Arguments valid, now time to compile each source and/or load PSPLCs for packaging!!
    unsigned int sources_c = 0;
    pspl_toolchain_driver_source_t sources[PSPL_MAX_SOURCES];
    unsigned int psplcs_c = 0;
    pspl_toolchain_driver_psplc_t psplcs[PSPL_MAX_SOURCES];
    
    
    // Run through sources and objects
    for (i=0 ; i<driver_opts.source_c ; ++i) {
        pspl_indexer_context_t* indexer = &indexers[i];
        driver_state.indexer_ctx = NULL;
        if (!(driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY)) {
            pspl_indexer_init(indexer, driver_state.ext_count, driver_opts.platform_c);
            driver_state.indexer_ctx = indexer;
        }
        int j;
        
        const char* file_ext = strrchr(driver_opts.source_a[i], '.') + 1;
        if (!strcasecmp(file_ext, "pspl")) {
            driver_state.pspl_phase = PSPL_PHASE_PREPARE;
            
#           pragma mark Process PSPL Source
            pspl_toolchain_driver_source_t* source = &sources[sources_c++];
            
            // Populate filename members
            source->file_path = driver_opts.source_a[i];
            
            // Make path absolute (if it's not already)
            const char* abs_path = source->file_path;
            if (abs_path[0] != '/') {
                abs_path = malloc(MAXPATHLEN);
                sprintf((char*)abs_path, "%s/%s", cwd, source->file_path);
            }
            
            // Enclosing dir
            char* last_slash = strrchr(abs_path, '/');
            source->file_enclosing_dir = malloc(MAXPATHLEN);
            sprintf((char*)source->file_enclosing_dir, "%.*s", (int)(last_slash-abs_path+1), abs_path);
            
            // File name
            source->file_name = last_slash+1;
            
            // Allocate required extension set
            source->required_extension_set = calloc(driver_state.ext_count+1, sizeof(pspl_extension_t*));
            
            // Set error handling state for source file
            driver_state.file_name = source->file_name;
            driver_state.line_num = 0;
            
            // Toolchain context info for this source
            tool_ctx.pspl_name = source->file_name;
            tool_ctx.pspl_enclosing_dir = source->file_enclosing_dir;
            
            // Initialise each extension
            driver_state.pspl_phase = PSPL_PHASE_INIT_EXTENSION;
            pspl_extension_t* ext;
            j = 0;
            while ((ext = pspl_available_extensions[j++])) {
                driver_state.proc_extension = ext;
                if (ext->toolchain_extension && ext->toolchain_extension->init_hook && !GET_INIT_BIT(j-1)) {
                    int err;
                    if ((err = ext->toolchain_extension->init_hook(&tool_ctx)))
                        pspl_error(-1, "Extension failed to init",
                                   "extension '%s' returned %d error code", ext->extension_name, err);
                    SET_INIT_BIT(j-1);
                }
            }
            
            // Prepare to read in file
            driver_state.pspl_phase = PSPL_PHASE_PREPARE;
            
            // Load original source
            FILE* source_file = fopen(driver_opts.source_a[i], "r");
            fseek(source_file, 0, SEEK_END);
            long source_len = ftell(source_file);
            fseek(source_file, 0, SEEK_SET);
            if (source_len > PSPL_MAX_SOURCE_SIZE)
                pspl_error(-1, "PSPL Source file exceeded filesize limit",
                           "source file `%s` is %l bytes in length; exceeding %u byte limit",
                           driver_opts.source_a[i], source_len, (unsigned)PSPL_MAX_SOURCE_SIZE);
            char* source_buf = malloc(source_len+1);
            if (!source_buf)
                pspl_error(-1, "Unable to allocate memory buffer for PSPL source",
                           "errno %d - `%s`", errno, strerror(errno));
            size_t read_len = fread(source_buf, 1, source_len, source_file);
            source_buf[source_len] = '\0';
            source->original_source = source_buf;
            fclose(source_file);
            if (read_len != source_len)
                pspl_error(-1, "Didn't read expected amount from PSPL source",
                           "expected %u bytes; read %u bytes", source_len, read_len);
            
            
            
            // Now run preprocessor
            pspl_run_preprocessor(source, &tool_ctx, &driver_opts);
            
            
            // Now run compiler (if not in preprocess-only mode)
            if (!(driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY)) {
                driver_state.pspl_phase = PSPL_PHASE_COMPILE;
                driver_state.file_name = source->file_name;
                driver_state.line_num = 0;
                pspl_run_compiler(source, &tool_ctx, &driver_opts);
            }
            
            // Finish each extension
            driver_state.pspl_phase = PSPL_PHASE_FINISH_EXTENSION;
            j = 0;
            while ((ext = pspl_available_extensions[j++])) {
                driver_state.proc_extension = ext;
                if (ext->toolchain_extension && ext->toolchain_extension->finish_hook)
                    ext->toolchain_extension->finish_hook(&tool_ctx);
                UNSET_INIT_BIT(j-1);
            }
            
            // Only do first source if in preprocess or compile only modes
            if (driver_opts.pspl_mode_opts & (PSPL_MODE_PREPROCESS_ONLY|PSPL_MODE_COMPILE_ONLY))
                break;
            
            
        } else if (!strcasecmp(file_ext, "psplc")) {
            driver_state.pspl_phase = PSPL_PHASE_PREPARE;
            
#           pragma mark Process PSPLC Object
            pspl_toolchain_driver_psplc_t* psplc = &psplcs[psplcs_c++];
            init_psplc_from_file(psplc, driver_opts.source_a[i]);
            if (driver_state.indexer_ctx)
                pspl_indexer_psplc_stub_augment(driver_state.indexer_ctx, psplc);
            
        }
        
        // Add Indexer to packager (if packaging)
        if (!(driver_opts.pspl_mode_opts & (PSPL_MODE_PREPROCESS_ONLY|PSPL_MODE_COMPILE_ONLY)))
            pspl_packager_indexer_augment(&packager_ctx, indexer);
        
    }
    
    
#   pragma mark Output Stage
    
    // Open output file
    FILE* out_file;
    if (driver_opts.out_path) {
        if (driver_opts.out_path[0] == '-')
            out_file = stdout;
        out_file = fopen(driver_opts.out_path, "w");
    } else {
        if (driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY)
            out_file = fopen("a.out.pspl", "w");
        else if (driver_opts.pspl_mode_opts & PSPL_MODE_COMPILE_ONLY)
            out_file = fopen("a.out.psplc", "w");
        else
            out_file = fopen("a.out.psplp", "w");
    }
    
    // Output selected data
    if (driver_opts.pspl_mode_opts & PSPL_MODE_PREPROCESS_ONLY) {
        
        // Preprocessor only
        size_t len = strlen(sources[0].preprocessed_source);
        fwrite(sources[0].preprocessed_source, 1, len, out_file);
        
    } else if (driver_opts.pspl_mode_opts & PSPL_MODE_COMPILE_ONLY) {
        
        // Compile only
        pspl_indexer_write_psplc(driver_state.indexer_ctx, driver_opts.default_endianness, out_file);
        
    } else {
        
        // Full Package
        driver_state.pspl_phase = PSPL_PHASE_PACKAGE;
        pspl_packager_write_psplp(&packager_ctx, driver_opts.default_endianness, out_file);
        
    }
    
    if (!driver_opts.out_path || (driver_opts.out_path && driver_opts.out_path[0] != '-'))
        fclose(out_file);
    
    
    // Same for reflist
    if (driver_state.gather_ctx)
        pspl_gather_finish(driver_state.gather_ctx, driver_opts.out_path);
    
    
    return 0;
    
}


#pragma mark Windows Support Stuff

#ifdef _WIN32

/*
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

char* strtok_r(char *str,
               const char *delim,
               char **nextp)
{
    char *ret;
    
    if (str == NULL)
    {
        str = *nextp;
    }
    
    str += strspn(str, delim);
    
    if (*str == '\0')
    {
        return NULL;
    }
    
    ret = str;
    
    str += strcspn(str, delim);
    
    if (*str)
    {
        *str++ = '\0';
    }
    
    *nextp = str;
    
    return ret;
}


#endif

