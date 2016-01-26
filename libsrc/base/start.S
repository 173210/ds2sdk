//start.S

#include <mips.h>

	.text

	.extern ds2_init
	.extern ds2_main

	.global	_start
	.type	_start,@function
	.set	noreorder
	.set	noat 
_start:

    la	ra, _start
	//----------------------------------------------------
	// init cp0 registers.
	//----------------------------------------------------
	mtc0	zero, C0_WATCHLO
	mtc0	zero, C0_WATCHHI

	li	t1, 0x00800000
	mtc0	t1, C0_CAUSE
    li	    t0, 0x0040FC00
	mtc0	t0, C0_STATUS

	//----------------------------------------------------
	// init caches, assumes a 4way*128set*32byte i/d cache
	//----------------------------------------------------
	li	t0, 3			// enable cache for kseg0 accesses
	mtc0	t0, $16		// CONFIG reg
	la	t0, 0x80000000	// an idx op should use a unmappable address
	ori	t1, t0, 0x4000	// 16kB cache
	mtc0	zero, $28	// TAGLO reg
	mtc0	zero, $29	// TAGHI reg

_init_cache_loop:	
	cache	0x8, 0(t0)	// index store icache tag
	//cache	0x9, 0(t0)	// index store dcache tag
    cache	0x1, 0(t0)
    sync
	bne	t0, t1, _init_cache_loop
	addiu	t0, t0, 0x20	// 32 bytes per cache line
	nop

	//----------------------------------------------------
	// Invalidate BTB
	//----------------------------------------------------
	mfc0	t0, C0_CONFIG, 7
	nop
	ori	t0, 2
	mtc0	t0, C0_CONFIG, 7
	nop

	//----------------------------------------------------
	// clear BSS section
	//----------------------------------------------------
	la	t0, _fbss
	la	t1, _end
1:	sw	zero, 0(t0)
	bne	t0, t1, 1b
	addiu	t0, 4

	//----------------------------------------------------
	// setup stack and GP, jump to C code
	//----------------------------------------------------
	la	gp, _gp
	la	sp, g_stack+0x100000-4

	la t0, ds2_init
	jalr	t0
	nop

	la t0, ds2_main
	jr	t0
	nop

	.end	

g_stack:
	.space 0x100000


