/*
 * init.S
 *
 * Initialization code for JzRISC.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <mips.h>

	.text

	.extern real_exception_entry

	.global except_common_entry
	.type	except_common_entry,@function
	.set noreorder
	.set noat
except_common_entry:
	la	k0, real_exception_entry
	jr	k0
	nop
	nop
	nop
	.set reorder

	.extern C_IntHandle
	.extern exception_handler
	.global real_exception_entry
	.type	real_exception_entry,@function
	.set noreorder
	.set noat
real_exception_entry:
	la	k1, (0x1f << 2)
	mfc0	k0, C0_CAUSE
	and	k0, k0, k1
	beq	zero, k0, intr_handler
	nop
	la	k0, exception_handler
	jr	k0
	nop
intr_handler:
	//save context
	subu sp,128
	sw ra,0(sp)
	sw fp,4(sp)
	sw gp,8(sp)
	sw t9,12(sp)
	sw t8,16(sp)
	sw s7,20(sp)
	sw s6,24(sp)
	sw s5,28(sp)
	sw s4,32(sp)
	sw s3,36(sp)
	sw s2,40(sp)
	sw s1,44(sp)
	sw s0,48(sp)
	sw t7,52(sp)
	sw t6,56(sp)
	sw t5,60(sp)
	sw t4,64(sp)
	sw t3,68(sp)
	sw t2,72(sp)
	sw t1,76(sp)
	sw t0,80(sp)
	sw a3,84(sp)
	sw a2,88(sp)
	sw a1,92(sp)
	sw a0,96(sp)
	sw v1,100(sp)
	sw v0,104(sp)
	sw AT,108(sp)
	MFC0(k1,C0_STATUS)
	sw k1,112(sp)
	MFC0(k1,C0_EPC)
	sw k1,116(sp)
	MFLo k1
	sw k1,120(sp)
	MFHi k1
	sw k1,124(sp)
	nop
	//Call interrupt handle
	la	k0, C_IntHandle
	jr	k0
	nop
	//restore context
	lw ra,0(sp)
	lw fp,4(sp)
	lw gp,8(sp)
	lw t9,12(sp)
	lw t8,16(sp)
	lw s7,20(sp)
	lw s6,24(sp)
	lw s5,28(sp)
	lw s4,32(sp)
	lw s3,36(sp)
	lw s2,40(sp)
	lw s1,44(sp)
	lw s0,48(sp)
	lw t7,52(sp)
	lw t6,56(sp)
	lw t5,60(sp)
	lw t4,64(sp)
	lw t3,68(sp)
	lw t2,72(sp)
	lw t1,76(sp)
	lw t0,80(sp)
	lw a3,84(sp)
	lw a2,88(sp)
	lw a1,92(sp)
	lw a0,96(sp)
	lw v1,100(sp)
	lw v0,104(sp)
	lw AT,108(sp)
	lw k0,112(sp)
	MTC0(k0,C0_STATUS)
	lw k0,116(sp)
	MTC0(k0,C0_EPC)
	lw k0,120(sp)
	MTLo k0
	lw k0,124(sp)
	MTHi  k0
	addu sp,128
	eret
	nop

	.set reorder

	.global exception_handler
	.type	exception_handler,@function
	.set noreorder
	.set noat
exception_handler:
	addiu   sp, sp, -128
	sw	ra, (4*0)(sp);
	sw	fp, (4*1)(sp);
	sw	gp, (4*2)(sp);
	sw	t9, (4*3)(sp);
	sw	t8, (4*4)(sp);
	sw	s7, (4*5)(sp);
	sw	s6, (4*6)(sp);
	sw	s5, (4*7)(sp);
	sw	s4, (4*8)(sp);
	sw	s3, (4*9)(sp);
	sw	s2, (4*10)(sp);
	sw	s1, (4*11)(sp);
	sw	s0, (4*12)(sp);
	sw	t7, (4*13)(sp);
	sw	t6, (4*14)(sp);
	sw	t5, (4*15)(sp);
	sw	t4, (4*16)(sp);
	sw	t3, (4*17)(sp);
	sw	t2, (4*18)(sp);
	sw	t1, (4*19)(sp);
	sw	t0, (4*20)(sp);
	sw	a3, (4*21)(sp);
	sw	a2, (4*22)(sp);
	sw	a1, (4*23)(sp);
	sw	a0, (4*24)(sp);
	sw	v1, (4*25)(sp);
	sw	v0, (4*26)(sp);
	sw	AT, (4*27)(sp);
	mflo k0
	sw	k0, (4*28)(sp);
	mfhi k0
	sw	k0, (4*29)(sp);
	mfc0 k0, C0_EPC         //return address
	sw	k0, (4*30)(sp);
	mfc0 k0, C0_STATUS      //return address
	sw	k0, (4*31)(sp);
	
	mfc0 k0, C0_CAUSE
	ori  k1, zero, (0x08<<2)    //Only detect SYSTEM CALL exception
	andi  k0, k0, (0x1F<<2)
	beq  k1, k0, 1f           //is SYSTEM CALL exception
    move	a0, sp
	la	k1, c_except_handler
	jr	k1
	srl a1, k0, 2

2:	
	lw	ra, (4*0)(sp);
	lw	fp, (4*1)(sp);
	lw	gp, (4*2)(sp);
	lw	t9, (4*3)(sp);
	lw	t8, (4*4)(sp);
	lw	s7, (4*5)(sp);
	lw	s6, (4*6)(sp);
	lw	s5, (4*7)(sp);
	lw	s4, (4*8)(sp);
	lw	s3, (4*9)(sp);
	lw	s2, (4*10)(sp);
	lw	s1, (4*11)(sp);
	lw	s0, (4*12)(sp);
	lw	t7, (4*13)(sp);
	lw	t6, (4*14)(sp);
	lw	t5, (4*15)(sp);
	lw	t4, (4*16)(sp);
	lw	t3, (4*17)(sp);
	lw	t2, (4*18)(sp);
	lw	t1, (4*19)(sp);
	lw	t0, (4*20)(sp);
	lw	a3, (4*21)(sp);
	lw	a2, (4*22)(sp);
	lw	a1, (4*23)(sp);
	lw	a0, (4*24)(sp);
	lw	v1, (4*25)(sp);
	lw	v0, (4*26)(sp);
	lw	AT, (4*27)(sp);
	lw	k0, (4*28)(sp);
	mtlo k0
	lw	k0, (4*29)(sp);	
	mthi k0
    lw	k0, (4*30)(sp);
    addiu k0, k0, 4
	mtc0 k0, C0_EPC         //return address= EPC +4
	lw	k0, (4*31)(sp);	
	mtc0 k0, C0_STATUS      //return address
    addiu sp, sp, 128
    
    eret                    //return

1:
    mfc0 k0, C0_EPC
    lw  k1, 0(k0)
    nop
    sll a0, k1, 6           //system call number
    sra a0, a0, 12
    or  a1, zero, k0        //syscall address
    or  a2, zero, sp        //sp
    la  k0, syscall_fun
    jal k0
    nop
    j   2b
    nop
    	
	.set reorder


