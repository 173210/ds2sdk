/*
 * clock.c
 *
 * Detect PLL out clk frequency and implement a udelay routine.
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
#include <bsp.h>
#include <jz4740.h>
#include <ds2io.h>

const static int FR2n[] = {
	1, 2, 3, 4, 6, 8, 12, 16, 24, 32
};

static unsigned int _pllout;
static unsigned int _iclk;

#if 0
extern int sdram_convert(unsigned int pllin,unsigned int *sdram_freq);

void sys_pll_init(unsigned int clock)
{
	register unsigned int cfcr, plcr1;
	unsigned int sdramclock = 0;
	int n2FR[33] = {
		0, 0, 1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0,
		7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,
		9
	};
	//int div[5] = {1, 4, 4, 4, 4}; /* divisors of I:S:P:L:M */
	int div[5] = {1, 3, 3, 3, 3}; /* divisors of I:S:P:L:M */
	int nf, pllout2;

	cfcr = CPM_CPCCR_CLKOEN |
		(n2FR[div[0]] << CPM_CPCCR_CDIV_BIT) | 
		(n2FR[div[1]] << CPM_CPCCR_HDIV_BIT) | 
		(n2FR[div[2]] << CPM_CPCCR_PDIV_BIT) |
		(n2FR[div[3]] << CPM_CPCCR_MDIV_BIT) |
		(n2FR[div[4]] << CPM_CPCCR_LDIV_BIT);

	pllout2 = (cfcr & CPM_CPCCR_PCS) ? clock : (clock / 2);

	/* Init UHC clock */
	REG_CPM_UHCCDR = pllout2 / 48000000 - 1;

	nf = clock * 2 / CFG_EXTAL;
	plcr1 = ((nf - 2) << CPM_CPPCR_PLLM_BIT) | /* FD */
		(0 << CPM_CPPCR_PLLN_BIT) |	/* RD=0, NR=2 */
		(0 << CPM_CPPCR_PLLOD_BIT) |    /* OD=0, NO=1 */
		(0x20 << CPM_CPPCR_PLLST_BIT) | /* PLL stable time */
		CPM_CPPCR_PLLEN;                /* enable PLL */          

	/* init PLL */
	REG_CPM_CPCCR = cfcr;
	REG_CPM_CPPCR = plcr1;

	// Config SDRAM to auto Power Down Mode,2008-6-27 14:22
	REG_EMC_DMCR |= (1 << 18);

	sdram_convert(clock,&sdramclock);
	if(sdramclock > 0)
	{
		REG_EMC_RTCOR = sdramclock;
		REG_EMC_RTCNT = sdramclock;	  	
	}else{
	  	printf("sdram init fail!\n");
  		while(1);
	}
}
#endif

void detect_clock(void)
{
	_pllout = (__cpm_get_pllm() + 2)* EXTAL_CLK / (__cpm_get_plln() + 2);
	_iclk = _pllout / FR2n[__cpm_get_cdiv()];
//    mclk = pllout / FR2n[__cpm_get_mdiv()];
//	dprintf("\n\nEXTAL_CLK = %dM PLL = %d iclk = %d mclk = %d\r\n",EXTAL_CLK / 1000 /1000,pllout,iclk, mclk);
}

void printf_clock(void)
{
	printf("CPU Clock = %dM\r\n", _pllout/1000/1000);
}

void udelay(unsigned int usec)
{
	unsigned int i = usec * (_iclk / 2000000);

	__asm__ __volatile__ (
		"\t.set noreorder\n"
		"1:\n\t"
		"bne\t%0, $0, 1b\n\t"
		"addi\t%0, %0, -1\n\t"
		".set reorder\n"
		: "=r" (i)
		: "0" (i)
	);
}

void mdelay(unsigned int msec)
{
	int i;
	for(i=0;i<msec;i++)
	{
		udelay(1000);
	}
}

//2011-2-22 10:32:38 by lhm
/*
*	Functin: get time
*/

//struct rtc{
//    vu8 year;		//add 2000 to get 4 digit year
//    vu8 month;		//1 to 12
//    vu8 day;		//1 to (days in month)
//
//    vu8 weekday;	// day of week
//    vu8 hours;		//0 to 11 for AM, 52 to 63 for PM
//    vu8 minutes;	//0 to 59
//    vu8 seconds;	//0 to 59
//} ;
//
//void ds2_getTime(struct rtc *time)
//{
//
//    time->year= 0;
//
//    time->month= 0;
//
//    time->day= 0;
//
//    time->weekday= 0;
//
//    time->hours= 0;
//
//    time->minutes= 0;
//
//    time->seconds= 0;
//
//}

