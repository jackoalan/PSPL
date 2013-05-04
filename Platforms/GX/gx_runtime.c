//
//  gx_runtime.c
//  PSPL
//
//  Created by Jack Andersen on 4/16/13.
//
//


#include "gx_runtime.h"

/* Bytecode playback routine (implemented in separate assembly file) */
extern void pspl_gx_playback(void* wgPipe, void* stream_start, size_t stream_len);