//
//  Driver.c
//  PSPL
//
//  Created by Jack Andersen on 4/15/13.
//
//
#define PSPL_INTERNAL
#define PSPL_TOOLCHAIN

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
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

/* Maximum count of target platforms
 * (logical 32-bit bitfields are used to relate objects to platforms) */
#define PSPL_MAX_PLATFORMS 32

/* Maximum source file size (512K) */
#define PSPL_MAX_SOURCE_SIZE (512*1024)



/* Escape character sequences to control xterm */
#define NOBKD "\E[47;49m"
#define RED "\E[47;31m"NOBKD
#define GREEN "\E[47;32m"NOBKD
#define YELLOW "\E[47;33m"NOBKD
#define BLUE "\E[47;34m"NOBKD
#define MAGENTA "\E[47;35m"NOBKD
#define CYAN "\E[47;36m"NOBKD
#define NORMAL "\033[0m"
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define SGR0 "\E[m\017"


/* Global driver state */
pspl_toolchain_driver_state_t driver_state;


/* Are we using xterm? */
static uint8_t xterm_colour = 0;

/* Way to get terminal width (for line wrapping) */
int term_cols() {
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    return w.ws_col;
}

/* Transform string into wrapped string */
#define truncate_string(str, max_len, to_buf) strncpy(to_buf, str, (max_len)-3); strncpy(to_buf+(max_len)-3, "...", 4);
char* wrap_string(const char* str_in, int indent) {
    
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
    
    for (;;) {
        if (!word_str) // Done
            break;
        
        // Available space remaining on line
        int avail_line_space = num_cols-cur_line_col;
        
        // Actual word length (0 length is actually a space)
        size_t word_length = strlen(word_str);
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
    return result_str;
    
}


void print_help(const char* prog_name) {
    
    if (xterm_colour) {
        
        // We have xterm colour support!
        
        fprintf(stderr,
                BOLD RED "PSPL Toolchain\n"
                BOLD MAGENTA "Copyright 2013 Jack Andersen "GREEN"<jackoalan@gmail.com>\n"
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
        pspl_runtime_platform_t* plat = NULL;
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
        /*
                BOLD BLUE"Directory Usage:"NORMAL"\n"
                BLUE"  pspl [-d] [-o "UNDERLINE"output-path"NORMAL BLUE"] "UNDERLINE"input-directory-path"NORMAL"\n"
                "\n"
                "  This usage will transform a valid intermediate directory structure ("UNDERLINE"PSPLD"NORMAL")\n"
                "  or package directory structure ("UNDERLINE"PSPLPD"NORMAL") into a "UNDERLINE"PSPLP"NORMAL" flat-file package.\n"
                "\n"
                "    "BOLD"-d"NORMAL"    Emit a "UNDERLINE"PSPLPD"NORMAL" directory rather than a "UNDERLINE"PSPLP"NORMAL" flat-file.\n"
                "\n"
                "    "BOLD"-o"NORMAL" "UNDERLINE"output-path"NORMAL"\n"
                "          Save the output file in the same directory with appropriate\n"
                "          file-extension added.\n"
                "\n"
                "\n"
                BOLD BLUE"File Usage:"NORMAL"\n"
                BLUE"  pspl [-o "UNDERLINE"output-path"NORMAL BLUE"] "UNDERLINE"input-file-path"NORMAL"\n"
                "  It's also possible to "
                );
         */
        
    } else {
        fprintf(stderr, "No colour version\n");
    }
}

void pspl_error(int exit_code, const char* brief, const char* msg, ...) {
    brief = wrap_string(brief, 1);
    char* msg_str = NULL;
    if (msg) {
        va_list va;
        va_start(va, msg);
        vasprintf(&msg_str, msg, va);
        va_end(va);
        char* new_msg = wrap_string(msg_str, 1);
        free(msg_str);
        msg_str = new_msg;
    }
    
    char* err_head = NULL;
    if (xterm_colour) {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string(BOLD RED"ERROR "CYAN"INITIALISING"RED" TOOLCHAIN:\n"SGR0, 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                sprintf(err_head, BOLD RED"ERROR WHILE "CYAN"PREPROCESSING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                sprintf(err_head, BOLD RED"ERROR WHILE "CYAN"COMPILING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                sprintf(err_head, BOLD RED"ERROR WHILE "CYAN"PACKAGING "BLUE"`%s`\n"SGR0,
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "%s", err_head);
        free(err_head);
        fprintf(stderr, BOLD"%s:\n"SGR0, brief);
    } else {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string("ERROR INITIALISING TOOLCHAIN:\n", 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                sprintf(err_head, "ERROR WHILE PREPROCESSING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                sprintf(err_head, "ERROR WHILE COMPILING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                sprintf(err_head, "ERROR WHILE PACKAGING `%s`\n",
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "%s", err_head);
        free(err_head);
        fprintf(stderr, "%s:\n", brief);
    }
    
    fprintf(stderr, "%s\n", msg_str);
    
    exit(exit_code);
}

void pspl_warn(const char* brief, const char* msg, ...) {
    brief = wrap_string(brief, 1);
    char* msg_str = NULL;
    if (msg) {
        va_list va;
        va_start(va, msg);
        vasprintf(&msg_str, msg, va);
        va_end(va);
        char* new_msg = wrap_string(msg_str, 1);
        free(msg_str);
        msg_str = new_msg;
    }
    
    char* err_head = NULL;
    if (xterm_colour) {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string(BOLD YELLOW"WARNING WHILE "CYAN"INITIALISING"YELLOW" TOOLCHAIN:\n"SGR0, 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                sprintf(err_head, BOLD YELLOW"WARNING WHILE "CYAN"PREPROCESSING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                sprintf(err_head, BOLD YELLOW"WARNING WHILE "CYAN"COMPILING "BLUE"`%s`"GREEN" LINE %u:\n"SGR0,
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                sprintf(err_head, BOLD RED"WARNING WHILE "CYAN"PACKAGING "BLUE"`%s`\n"SGR0,
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "%s", err_head);
        free(err_head);
        fprintf(stderr, BOLD"%s:\n"SGR0, brief);
    } else {
        switch (driver_state.pspl_phase) {
            case PSPL_PHASE_INIT:
                err_head = wrap_string("WARNING WHILE INITIALISING TOOLCHAIN:\n", 1);
                break;
            case PSPL_PHASE_PREPROCESS:
                sprintf(err_head, "WARNING WHILE PREPROCESSING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_COMPILE:
                sprintf(err_head, "WARNING WHILE COMPILING `%s` LINE %u:\n",
                        driver_state.file_name, driver_state.line_num);
                err_head = wrap_string(err_head, 1);
                break;
            case PSPL_PHASE_PACKAGE:
                sprintf(err_head, "WARNING WHILE PACKAGING `%s`\n",
                        driver_state.file_name);
                err_head = wrap_string(err_head, 1);
                break;
            default:
                break;
        }
        fprintf(stderr, "%s", err_head);
        free(err_head);
        fprintf(stderr, "%s:\n", brief);
    }
    
    fprintf(stderr, "%s\n", msg_str);
}

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
static void add_target_platform(pspl_toolchain_driver_opts_t* driver_opts,
                                const char* name) {
    if (driver_opts->platform_c >= PSPL_MAX_PLATFORMS) {
        pspl_error(-1, "Platforms exceeded maximum count",
                   "up to %u platforms supported", (unsigned int)PSPL_MAX_PLATFORMS);
    }
    
    // Look up platform
    pspl_runtime_platform_t* plat = NULL;
    int i = 0;
    while ((plat = pspl_available_target_platforms[i++])) {
        if (!strcmp(plat->platform_name, name)) {
            driver_opts->platform_a[driver_opts->platform_c] = plat;
            break;
        }
    }
    
    if (!plat)
        pspl_error(-1, "Unable to find requested platform",
                   "requested `%s`", name);
    
    ++driver_opts->platform_c;
}

int main(int argc, char** argv) {
    
    // Check for xterm
    const char* term_type = getenv("TERM");
    if (term_type && strlen(term_type) >= 5 && !strncmp("xterm", term_type, 5))
        xterm_colour = 1;
    
    // Initial driver state
    driver_state.pspl_phase = PSPL_PHASE_INIT;
    
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
    
    
    // Initial argument pass
    char expected_arg = 0;
    int i;
    for (i=1;i<argc;++i) {
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
                
            }
            
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
                   "PSPL can't preprocess-only `-E` *and* compile-only `-c` simultaneously");
        return -1;
    }
    
    
    // Ensure *all* provided files and paths exist
    
    // Sources
    for (i=0;i<driver_opts.source_c;++i) {
        FILE* file;
        if ((file = fopen(driver_opts.source_a[i], "r")))
            fclose(file);
        else
            pspl_error(-1, "Unable to open provided source", "%s", driver_opts.source_a[i]);
    }
    
    // Reflist (just the path)
    if (driver_opts.reflist_out_path) {
        FILE* file;
        if ((file = fopen(driver_opts.reflist_out_path, "w")))
            fclose(file);
        else
            pspl_error(-1, "Unable to write reflist", "Can't open `%s` for writing",
                       driver_opts.reflist_out_path);
    }
    
    // Staging area path
    if (!driver_opts.staging_path) {
        driver_opts.staging_path = malloc(512);
        if(!getcwd((char*)driver_opts.staging_path, 512))
            pspl_error(-1, "Unable to get current working directory",
                       "errno %d - `%s`", errno, strerror(errno));
    }
    
    // Stat path to ensure it's a directory
    struct stat s;
    if (stat(driver_opts.staging_path, &s))
        pspl_error(-1, "Unable to `stat` requested staging path",
                   "`%s`, errno %d - `%s`", driver_opts.staging_path,
                   errno, strerror(errno));
    else if(!(s.st_mode & S_IFDIR)) {
        pspl_error(-1, "Unable to use requested staging path",
                   "`%s`, errno %d - `%s`", driver_opts.staging_path,
                   errno, strerror(errno));
    }
    
    
    // Arguments valid, now time to compile each source!!
    pspl_toolchain_driver_source_t sources[PSPL_MAX_SOURCES];
    for (i=0;i<driver_opts.source_c;++i) {
        pspl_toolchain_driver_source_t* source = &sources[i];
        
        
        // Populate filename members
        source->file_path = driver_opts.source_a[i];
        char* base = strrchr(source->file_path, '/');
        source->file_name = (base&&(*(base+1)))?(base+1):source->file_path;
        
        // Set error handling state for preprocessor
        driver_state.pspl_phase = PSPL_PHASE_PREPROCESS;
        driver_state.file_name = source->file_name;
        driver_state.line_num = 0;
        
        // Load original source
        FILE* source_file = fopen(driver_opts.source_a[i], "r");
        fseek(source_file, 0, SEEK_END);
        long source_len = ftell(source_file);
        fseek(source_file, 0, SEEK_SET);
        if (source_len > PSPL_MAX_SOURCE_SIZE)
            pspl_error(-1, "PSPL Source file exceeded filesize limit",
                       "source file `%s` is %l bytes in length; exceeding %u byte limit",
                       driver_opts.source_a[i], source_len, (unsigned)PSPL_MAX_SOURCE_SIZE);
        source->original_source = malloc(source_len);
        if (!source->original_source)
            pspl_error(-1, "Unable to allocate memory buffer for PSPL source",
                       "errno %d - `%s`", errno, strerror(errno));
        size_t read_len = fread((void*)source->original_source, source_len, 1, source_file);
        fclose(source_file);
        if (read_len != source_len)
            pspl_error(-1, "Didn't read expected amount from PSPL source",
                       "expected %u bytes; read %u bytes", source_len, read_len);
        
        
        
        // Now run preprocessor
        _pspl_run_preprocessor(source, &driver_opts);
        
        
        // Now run compiler
        driver_state.pspl_phase = PSPL_PHASE_COMPILE;
        driver_state.file_name = source->file_name;
        driver_state.line_num = 0;
        _pspl_run_compiler(source, &driver_opts);
        
    }
    
    
    // Now run packager
    driver_state.pspl_phase = PSPL_PHASE_PACKAGE;
    driver_state.file_name = driver_opts.out_path;
    driver_state.line_num = 0;
    _pspl_run_packager(sources, &driver_opts);
    
    
    return 0;
    
}

/* Test extension and platform */

pspl_extension_t test_ext = {
    .extension_name = "Test Extension",
    .extension_desc = "A test of the PSPL extension system"
};

pspl_extension_t test_ext2 = {
    .extension_name = "Test Extension 2",
    .extension_desc = "Another test of the PSPL extension system"
};

pspl_extension_t* pspl_available_extensions[] = {
    &test_ext,
    &test_ext2,
    NULL
};

pspl_runtime_platform_t test_plat = {
    .platform_name = "GL2LE",
    .platform_desc = "OpenGL[ES] 2.0 for Little-Endian platforms",
    .byte_order = PSPL_LITTLE_ENDIAN
};

pspl_runtime_platform_t* pspl_available_target_platforms[] = {
    &test_plat,
    NULL
};

pspl_runtime_platform_t* pspl_default_target_platform = &test_plat;




