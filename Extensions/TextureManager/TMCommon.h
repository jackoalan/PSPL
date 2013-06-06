//
//  TMCommon.h
//  PSPL
//
//  Created by Jack Andersen on 6/6/13.
//
//

#ifndef PSPL_TMCommon_h
#define PSPL_TMCommon_h

#include <stdint.h>
#include <PSPL/PSPLValue.h>

struct pspl_tm_size {
    uint16_t width, height;
};

typedef struct {
    uint8_t key1, chan_count, num_mips, data_off;
    DEF_BI_OBJ_TYPE(struct pspl_tm_size) size;
} pspl_tm_texture_head_t;

#endif
