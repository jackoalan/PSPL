//
//  PSPLCommon.c
//  PSPL
//
//  Created by Jack Andersen on 5/25/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <PSPL/PSPL.h>

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