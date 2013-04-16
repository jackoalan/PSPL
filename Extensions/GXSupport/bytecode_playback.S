//
//  bytecode_playback.S
//  PSPL
//
//  Created by Jack Andersen on 4/16/13.
//
//

/* Args: */
#define wgPipe 3
#define stream_start 4
#define stream_len 5

/* Scratch Regs: */
#define loader 0
#define shifter 11
#define logic_applier 12

/* The bytecode stream is a simple frame-based layout.
 * There is a word-sized (32-bit) header every 64 bytes containing
 * 15 2-bit enumerated values (from LSB to MSB; with 2 MSB padding bits). 
 * This value indicates what type of PPC store instruction needs to occur 
 * in order to feed the write-gather pipe appropriately: (stw,sth,stb,stfs).
 *
 * The (up to) 15 words following the frame header contain the actual
 * values to be loaded. The routine will branch back to link-register
 * when the stream is exhausted. */

/* Register layout:
 * r3:  `wgPipe` pointer (where the stream words should be repeatedly stored)
 * r4:  `stream_start` pointer (incremented as stream is read; must be 64-byte aligned minus 4)
 * r5:  `stream_len` size (decremented as stream is read)
 * r0:  word loader/storer; other temp use (to copy a word value from the stream into the write-gather pipe)
 * r11: stream frame header shifting area (loaded at each 64-byte interval; shifted every store)
 * r12: stream frame header logic area (to exclude bits for each store)
 */

/* Playback our bytecode stream */
// extern void pspl_gx_playback(void* wgPipe, void* stream_start, size_t stream_len)
	.globl pspl_gx_playback
pspl_gx_playback:

1:  // Frame interval (and first word)
	lwzu shifter, 4(stream_start)
	lwzu loader, 4(stream_start)

2:  // Word interval
	andi. r12, stream_start, 0x3f       // Branch back to frame interval if `and` result is 0
	bne   1b

	


	

