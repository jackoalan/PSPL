//
//  ReferenceGatherer.c
//  PSPL
//
//  Created by Jack Andersen on 5/12/13.
//
//

#define PSPL_INTERNAL

#include <stdio.h>
#include <string.h>

#include <PSPLExtension.h>
#include "ReferenceGatherer.h"

#define PSPL_INITIAL_REF_CAP 50

#define PSPL_REFLIST_BEGIN "--PSPL REFERENCE LIST BEGIN--"
#define PSPL_REFLIST_END "--PSPL REFERENCE LIST END--"

static void __pspl_gather_add_old_file(pspl_gatherer_context_t* ctx, const char* file) {
    
    // Ensure file isn't already in list
    int i;
    for (i=0 ; i<ctx->ref_old_count ; ++i) {
        if (!strcmp(file, ctx->ref_old_array[i]))
            return;
    }
    
    // Increment ref count (and reallocate if necessary)
    ++ctx->ref_old_count;
    if (ctx->ref_old_count >= ctx->ref_old_cap) {
        ctx->ref_old_cap *= 2;
        ctx->ref_old_array = realloc(ctx->ref_old_array, ctx->ref_old_cap*sizeof(char*));
    }
    
    // Add to list
    ctx->ref_old_array[ctx->ref_old_count-1] = file;
    
}

/* Init context and load (or stage creation of) include file */
void pspl_gather_load_cmake(pspl_gatherer_context_t* ctx, const char* cmake_path) {
    
    // Initialise context
    ctx->cmake_path = cmake_path;
    ctx->ref_cap = PSPL_INITIAL_REF_CAP;
    ctx->ref_count = 0;
    ctx->ref_array = calloc(PSPL_INITIAL_REF_CAP, sizeof(char*));
    ctx->ref_old_cap = PSPL_INITIAL_REF_CAP;
    ctx->ref_old_count = 0;
    ctx->ref_old_array = calloc(PSPL_INITIAL_REF_CAP, sizeof(char*));
    
    // See if there is already an include file and load it
    FILE* cmake_file = fopen(ctx->cmake_path, "r");
    if (cmake_file) {
        
        // Read in include file
        fseek(cmake_file, 0, SEEK_END);
        size_t cmake_size = ftell(cmake_file);
        fseek(cmake_file, 0, SEEK_SET);
        
        if (cmake_size) {
        
            // Read in
            char* cmake_buf = malloc(cmake_size+1);
            fread((void*)cmake_buf, 1, cmake_size, cmake_file);
            cmake_buf[cmake_size] = '\0';
            
            // Advance to list `set(`
            char* read_in = strstr(cmake_buf, "# "PSPL_REFLIST_BEGIN"\nset(");
            read_in = strchr(read_in, '\n') + 1;
            read_in = strchr(read_in, '\n') + 2;
            char* read_end = strstr(read_in, ")\n# "PSPL_REFLIST_END);
            if (read_in && read_end) {
                char* save_ptr;
                read_in = strtok_r(read_in, "\";\n\"", &save_ptr);
                do
                    __pspl_gather_add_old_file(ctx, read_in);
                while ((read_in = strtok_r(save_ptr, "\";\n\"", &save_ptr)) && read_in < read_end);
            }
            
        }
        
        fclose(cmake_file);
        
    }
    
    // This ensures the file isn't re-written if it doesn't need to be
    // (thereby easing CMake's dependency resolution)
    ctx->changes_made = 0;
    
}

/* Add referenced file (absolute path) to gatherer */
void pspl_gather_add_file(pspl_gatherer_context_t* ctx, const char* file) {
    
    // Ensure file isn't already in list
    int i;
    for (i=0 ; i<ctx->ref_count ; ++i) {
        if (!strcmp(file, ctx->ref_array[i]))
            return;
    }
    
    // Buffer string separately
    size_t len = strlen(file) + 1;
    char* buf = malloc(len);
    strncpy(buf, file, len);
    
    // Increment ref count (and reallocate if necessary)
    ++ctx->ref_count;
    if (ctx->ref_count >= ctx->ref_cap) {
        ctx->ref_cap *= 2;
        ctx->ref_array = realloc(ctx->ref_array, ctx->ref_cap*sizeof(char*));
    }
    
    // Add to list
    ctx->ref_array[ctx->ref_count-1] = buf;
    
    // Check for changes against old references
    for (i=0 ; i<ctx->ref_old_count ; ++i) {
        if (!strcmp(file, ctx->ref_old_array[i]))
            return;
    }
    
    // This is set if the file reference is not in the old array
    ctx->changes_made = 1;
    
}

/* Finish context and write to disk */
void pspl_gather_finish(pspl_gatherer_context_t* ctx, const char* psplc_file) {
    
    if (ctx->ref_count != ctx->ref_old_count || ctx->changes_made) {
        
        // Write out include file
        FILE* cmake_file = fopen(ctx->cmake_path, "w");
        fprintf(cmake_file, "# DO NOT EDIT!!!\n"
                "# Auto-generated PSPL source includes for:\n# `%s`\n"
                "cmake_minimum_required(VERSION 2.8)\n\n"
                "# "PSPL_REFLIST_BEGIN"\nset(PSPL_REFLIST", psplc_file);
        
        uint8_t add_semicolon = 0;
        int i;
        for (i=0 ; i<ctx->ref_count ; ++i) {
            if (add_semicolon)
                fprintf(cmake_file, ";");
            fprintf(cmake_file, "\n\"%s\"", ctx->ref_array[i]);
            add_semicolon = 1;
        }
        
        fprintf(cmake_file, ")\n# "PSPL_REFLIST_END"\n\n");
        
        fclose(cmake_file);
        
    }
    
}

