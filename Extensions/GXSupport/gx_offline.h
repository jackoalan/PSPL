#ifndef _GX_OFFLINE_H
#define _GX_OFFLINE_H

void pspl_gx_offline_begin_transaction();
size_t pspl_gx_offline_end_transaction(void** buf_out);

void pspl_gx_offline_add_u8(uint8_t val);
void pspl_gx_offline_add_u16(uint16_t val);
void pspl_gx_offline_add_u32(uint32_t val);
void pspl_gx_offline_add_float(float val);

#endif // _GX_OFFLINE_H
