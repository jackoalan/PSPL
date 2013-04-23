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
#define temp 12

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
 *  r3: `wgPipe` pointer (where the stream words should be repeatedly stored)
 *  r4: `stream_start` pointer (incremented as stream is read; must be 64-byte aligned, stream starts with magic word)
 *  r5: `stream_len` size (decremented as stream is read)
 *
 *  r0: word loader/storer; other temp use (to copy a word value from the stream into the write-gather pipe)
 * r11: stream frame header shifting area (loaded at each 64-byte interval; shifted every store)
 * r12: temporary register for various immediate calculations
 */

/* Our magic word to validate stream */
	.data
pspl_gx_magic:
	.long  0x50534758 // "PSGX"
	.align 2

/* Error codes:
 *  0: No Error
 * -1: Stream too short
 * -2: No valid magic word (perhaps not PSPL GX bytecode)
 */

/* Playback our bytecode stream */
// extern int pspl_gx_playback(void* wgPipe, void* stream_start, size_t stream_len)
	.text
	.globl pspl_gx_playback
pspl_gx_playback:

	// We need at least 12 bytes in stream (4 magic, 4 header, 4 first word)
	cmplwi stream_len, 12
	li     3, -1
	blt    9f

	// Check for magic
	lis    temp, pspl_gx_magic@ha
	lwz    temp, pspl_gx_magic@l(temp)
	lwz    shifter, 0(stream_start)
	cmplw  temp, shifter
	li     3, -2
	bne    9f

1:  // Frame interval (and first word of frame)

	lwzu   shifter, 4(stream_start)
	subi   stream_len, stream_len, 4

2:  // Word interval

	// Load our word
	lwzu   loader, 4(stream_start)
	subi   stream_len, stream_len, 4

	// Determine word type
	andi.  temp, shifter, 0x2
	beq    5f

3:  // stw (0x0) or sth (0x1)

	andi.  temp, shifter, 0x1
	beq    4f
	stw    loader, 0(wgPipe)
	b      7f

4:  // sth

	sth    loader, 0(wgPipe)
	b      7f

5:  // stb (0x2) or stfs (0x3)

	andi.  temp, shifter, 0x1
	beq    6f
	stb    loader, 0(wgPipe)
	b      7f

6:  // stfs

	lfs    loader, 0(stream_start)
	stfs   loader, 0(wgPipe)

7:  // Just loaded into wgPipe

	// Check to see if we're done
	cmplwi stream_len, 0
	beq    8f

	// We're not done yet
	// Branch back to frame interval if `and` result is 0 (64align+4 is our next header)
	andi.  temp, stream_start, 0x3f
	bne    1b

	// Otherwise shift right and branch back for next word
	srwi   shifter, shifter, 2
	b      2b

8:

	// We're done (no error)
	li     3, 0

9:

	// We're done
	blr


