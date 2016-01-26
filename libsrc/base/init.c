/*
 * init.c
 *
 * Perform the early initialization. Include CP0.status, install exception
 * handlers, fill all interrupt entries.
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

#include <archdefs.h>
#include <mipsregs.h>
#include "jz4740.h"

//unsigned int g_stack[0x8000]; 

extern void except_common_entry(void);

void CONSOL_SendCh(unsigned char ch)
{
	serial_putc(ch);
}

void CONSOL_GetChar(unsigned char *ch)
{
	int r;
	r = serial_getc();
	if (r > 0)
		*ch = (unsigned char)r;
	else
		*ch = 0;
}

typedef void (*pfunc)(void);

extern pfunc __CTOR_LIST__[];
extern pfunc __CTOR_END__[];

void init_perihery()
{
    int i;

    //Initial DMA
    REG_DMAC_DMACR = 0;     //Global DMA disable
    REG_DMAC_DMAIPR = 0;    //Clear DMA interrupt pending
    for(i= 0; i < MAX_DMA_NUM; i++) {
        REG_DMAC_DCCSR(i) = 0;  //Disable DMA channel
    }
}

#define life_test()                     \
do{                                     \
    REG_GPIO_PXFUNC(4) = 0x000C0000;	\
    REG_GPIO_PXDIRS(4) = 0x000C0000;	\
                                        \
    while(1)                            \
    {                                   \
        REG_GPIO_PXDATS(4) = 0x000C0000;\
        REG_GPIO_PXDATC(4) = 0x000C0000;\
    }                                   \
} while(0)                              \

void ds2_init(void)
{
	pfunc *p;
	write_c0_status(0x10000400);

	memcpy((void *)A_K0BASE, except_common_entry, 0x20);
	memcpy((void *)(A_K0BASE + 0x180), except_common_entry, 0x20);
	memcpy((void *)(A_K0BASE + 0x200), except_common_entry, 0x20);

	__dcache_writeback_all();
	__icache_invalidate_all();

    init_perihery();
    InitExcept();
	_intc_init();

	detect_clock();

//	gpio_init();
//	serial_init();
    pm_init();
	//dgprintf("\n\nOS initial!\n");

//	OSInit();
    /* Invoke constroctor functions when needed. */
#if 1	
	for (p=&__CTOR_END__[-1]; p >= __CTOR_LIST__; p--)
	{
		printf("create class %08x\n",p);
		
		(*p)();
    }
	
	//dgprintf("Before main function\n");
#endif

	//Start system ticker
	_StartSysTimer();

	//enable global interrupt
	sti();
}

