/*
 * linux/arch/mips/jz4740/common/pm.c
 * 
 * JZ4740 Power Management Routines
 * 
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

#include <bsp.h>
#include <jz4740.h>
#include <mipsregs.h>

#define LCD_CLK 40000000    //40MHz

#define PLL_M       0
#define PLL_N       1
#define PLL_CCLK    2
#define PLL_HCLK    3
#define PLL_MCLK    4
#define PLL_PCLK    5

static unsigned char pll_m_n[16][6] = {
        //M, N, CCLK, HCLK, MCLK, PCLK, EXT_CLK=24MHz
        {10-2, 2-2, 1, 1, 1, 1},    //0    60, 60, 1/1
        {10-2, 2-2, 0, 1, 1, 1},    //1    120, 60, 1/2
        {10-2, 2-2, 0, 0, 0, 0},    //2    120, 120, 1/1
        {12-2, 2-2, 0, 1, 1, 1},    //3    144, 72, 1/2
        {16-2, 2-2, 0, 1, 1, 1},    //4    192, 96, 1/2
        {17-2, 2-2, 0, 0, 1, 1},    //5    204, 102, 1/2
        {20-2, 2-2, 0, 1, 1, 1},    //6    240, 120, 1/2
        {22-2, 2-2, 0, 2, 2, 2},    //7    264, 88, 1/3
        {24-2, 2-2, 0, 2, 2, 2},    //8    288, 96, 1/3
        {25-2, 2-2, 0, 2, 2, 2},    //9    300, 100, 1/3
        {28-2, 2-2, 0, 2, 2, 2},    //10   336, 112, 1/3
        {30-2, 2-2, 0, 2, 2, 2},    //11   360, 120, 1/3
        {32-2, 2-2, 0, 2, 2, 2},    //12   384, 128, 1/3
        {33-2, 2-2, 0, 2, 2, 2},    //13   396, 132, 1/3
        
        {33-2, 2-2, 0, 2, 2, 2},    //14   396, 132, 1/3
        {33-2, 2-2, 0, 2, 2, 2}     //15   396, 132, 1/3
    };

static int _pm_do_hibernate(void)
{
	printf("Put CPU into hibernate mode.\n");
	serial_waitfinish();
	__rtc_clear_hib_stat_all();
	// __rtc_set_scratch_pattern(0x12345678);
	__rtc_enable_alarm_wakeup();
	__rtc_set_hrcr_val(0xfe0);
	__rtc_set_hwfcr_val((0xFFFF << 4));
   	__rtc_power_down();
	
	while(1);
        
	return 0;
}

static int _pm_do_sleep(void)
{
	//unsigned long imr = REG_INTC_IMR;

	/* Preserve current time */

	/* Mask all interrupts */
//	REG_INTC_IMSR = 0xffffffff;

	/* Just allow next interrupts to wakeup the system.
	 * Note: modify this according to your system.
	 */

	/* Enter SLEEP mode */
	REG_CPM_LCR &= ~CPM_LCR_LPM_MASK;
	REG_CPM_LCR |= CPM_LCR_LPM_SLEEP;

	//REG_CPM_LCR &= ~CPM_LCR_LPM_MASK;
	//REG_CPM_LCR |= CPM_LCR_LPM_IDLE;

	__asm__(".set\tmips3\n\t"
		"sync\n\t"
		"wait\n\t"
		"nop\n\t"
		".set\tmips0");

	/* Restore to IDLE mode */
	REG_CPM_LCR &= ~CPM_LCR_LPM_MASK;
	REG_CPM_LCR |= CPM_LCR_LPM_IDLE;

	/* Restore interrupts */
	//REG_INTC_IMR = imr;

	/* Restore current time */

	return 0;
}

static int _sdram_convert(unsigned int pllin,unsigned int *sdram_dmcr, unsigned int *sdram_div, unsigned int *sdram_tref)
{
	register unsigned int ns, dmcr,tmp;

    dmcr = ~(EMC_DMCR_TRAS_MASK | EMC_DMCR_RCD_MASK | EMC_DMCR_TPC_MASK |
                EMC_DMCR_TRWL_MASK | EMC_DMCR_TRC_MASK) & REG_EMC_DMCR;

    /* Set sdram operation parameter */
    //pllin unit is KHz
	ns = 1000000*1024 / pllin;
	tmp = SDRAM_TRAS*1024/ns;
	if (tmp < 4) tmp = 4;
	if (tmp > 11) tmp = 11;
	dmcr |= ((tmp-4) << EMC_DMCR_TRAS_BIT);

	tmp = SDRAM_RCD*1024/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_RCD_BIT);

	tmp = SDRAM_TPC*1024/ns;
	if (tmp > 7) tmp = 7;
	dmcr |= (tmp << EMC_DMCR_TPC_BIT);

	tmp = SDRAM_TRWL*1024/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_TRWL_BIT);

	tmp = (SDRAM_TRAS + SDRAM_TPC)*1024/ns;
	if (tmp > 14) tmp = 14;
	dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

    *sdram_dmcr = dmcr;

	/* Set refresh registers */
    unsigned int div;
	tmp = SDRAM_TREF*1024/ns;
    div = (tmp + 254)/255;
    if(div <= 4) div = 1;       //  1/4
    else if(div <= 16) div = 2; //  1/16
    else div = 3;               //  1/64

    *sdram_div = ~EMC_RTCSR_CKS_MASK & REG_EMC_RTCSR | div;

    unsigned int divm= 4;
    while(--div) divm *= 4;

	tmp = tmp/divm + 1;
    *sdram_tref = tmp;

	return 0;
}

static void pll_bypass(void)
{
	unsigned int freq;
         /* sdram convert */
        
//        sdram_convert(12000000,&freq);
        REG_EMC_RTCOR = freq;
	REG_EMC_RTCNT = freq;

        REG_CPM_CPPCR |= CPM_CPPCR_PLLBP;
        REG_CPM_CPCCR  = ((REG_CPM_CPCCR & (~0xffff)) | CPM_CPCCR_CE);

       	REG_CPM_CPCCR &= ~CPM_CPCCR_CE;

}

/* convert pll while program is running */
int _pm_pllconvert(unsigned int level)
{
    unsigned int freq_b;
    unsigned int dmcr;
    unsigned int rtcsr;
    unsigned int tref;
    unsigned int cpccr;
    unsigned int cppcr;

    if(level > 15) return -1;

    freq_b = (pll_m_n[level][PLL_M]+2)*(EXTAL_CLK/1000)/(pll_m_n[level][PLL_N]+2);

    //freq_b unit is KHz
    _sdram_convert(freq_b/pll_m_n[level][PLL_MCLK], &dmcr, &rtcsr, &tref);
 
    cpccr = REG_CPM_CPCCR;
    cppcr = REG_CPM_CPPCR;

    REG_CPM_CPCCR = ~CPM_CPCCR_CE & cpccr;

    cppcr &= ~(CPM_CPPCR_PLLM_MASK | CPM_CPPCR_PLLN_MASK);
    cppcr |= (pll_m_n[level][PLL_M] << CPM_CPPCR_PLLM_BIT) | (pll_m_n[level][PLL_N] << CPM_CPPCR_PLLN_BIT);

    cpccr &= ~(CPM_CPCCR_CDIV_MASK | CPM_CPCCR_HDIV_MASK | CPM_CPCCR_PDIV_MASK |
                CPM_CPCCR_MDIV_MASK | CPM_CPCCR_LDIV_MASK);
    cpccr |= (pll_m_n[level][PLL_CCLK] << CPM_CPCCR_CDIV_BIT) | (pll_m_n[level][PLL_HCLK] << CPM_CPCCR_HDIV_BIT) |
                (pll_m_n[level][PLL_MCLK] << CPM_CPCCR_MDIV_BIT) | (pll_m_n[level][PLL_PCLK] << CPM_CPCCR_PDIV_BIT) |
                (31 << CPM_CPCCR_LDIV_BIT);

    REG_CPM_CPCCR = cpccr;
    REG_CPM_CPPCR = cppcr;

    REG_CPM_CPCCR |= CPM_CPCCR_CE;
    //Wait PLL stable
    while(!(CPM_CPPCR_PLLS & REG_CPM_CPPCR));

    //REG_EMC_DMCR = dmcr;
    REG_EMC_RTCOR = tref;
	REG_EMC_RTCNT = tref;

    detect_clock();
    return 0;
}

/* Put CPU to HIBERNATE mode */
int pm_hibernate(void)
{
	return _pm_do_hibernate();
}

/* Put CPU to SLEEP mode */
int pm_sleep(void)
{
	return _pm_do_sleep();
}

/* Put CPU to IDLE mode */
void pm_idle(void)
{
	__asm__(
        ".set\tmips3\n\t"
        "wait\n\t"
		".set\tmips0"
        );
		//cpu_wait();
}

struct pll_opt
{
	unsigned int cpuclock;
	int div;
};

static struct pll_opt opt_pll[4];
void pm_init()
{
	unsigned int CFG_CPU_SPEED = (*(volatile unsigned int*)((0x08000000 | 0x80000000) + 0x20000+0x80 -4));
	opt_pll[0].cpuclock= CFG_CPU_SPEED/4;
	opt_pll[0].div=3;
	opt_pll[1].cpuclock=(CFG_CPU_SPEED*2)/4;
	opt_pll[1].div=1;
	opt_pll[2].cpuclock= CFG_CPU_SPEED;
	opt_pll[2].div=0; 

	opt_pll[3].cpuclock= CFG_CPU_SPEED;
	opt_pll[3].div=0; 
}

//return -1 when failure
int ds2_setCPUclocklevel(unsigned int num)
{
    return _pm_pllconvert(num);
}

int pm_control(int level)
{
	if(level<0 || level >3)
		return -1;

	if(level==3)
	{
		return pm_sleep();
	}
	else
	{
		//return _pm_pllconvert(opt_pll[level].cpuclock,opt_pll[level].div);
	}
}

void pm_reset(void)
{
    __wdt_select_clk_div1024();
    __wdt_set_data(256);
    __wdt_set_count(100);
    __wdt_select_pclk();
    __wdt_start();

	while(1) {
		printf("WDT %d %d\n", REG16(WDT_BASE+8), REG16(WDT_BASE+0));
	}
}


