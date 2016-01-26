/*
 * timer.c
 *
 * Perform the system ticks.
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
#include "mipsregs.h"

#define USE_RTC_CLOCK 0
void _tickerHander(unsigned int arg)
{
//	OSTimeTick();
}

void _StartTicker(unsigned int tps)
{ 
	unsigned int latch;
	__cpm_start_tcu();
	
	__tcu_disable_pwm_output(0);
	__tcu_mask_half_match_irq(0); 
	__tcu_unmask_full_match_irq(0);

#if USE_RTC_CLOCK
	__tcu_select_rtcclk(0);
	__tcu_select_clk_div1(0);
    latch = (__cpm_get_rtcclk() + (tps>>1)) / tps;
	
#else	
	__tcu_select_extalclk(0);
	__tcu_select_clk_div4(0);
	
	latch = (EXTAL_CLK / 4 + (tps>>1)) / tps;
#endif
	REG_TCU_TDFR(0) = latch;
	REG_TCU_TDHR(0) = latch;
	__tcu_set_count(0,0);
	__tcu_clear_full_match_flag(0);
	__tcu_start_counter(0);
	
    dgprintf("TCSR = 0x%04x\r\n",*(volatile u16 *)0xb000204C);
	
	request_irq(IRQ_TCU2, _tickerHander,0);

printf("IE %08x\n", read_c0_status());

/*
while(1)
{
	if(__tcu_full_match_flag(0))
	{
		printf("match\n");
		printf("REG_INTC_IPR %08x\n", REG_INTC_IPR);
//		__tcu_clear_full_match_flag(0);
//		REG_INTC_IPR = -1;

		printf("IE %08x\n", read_c0_status());
//		sti();
	}
}
*/


}

void _StopTicker(void)
{
	__tcu_stop_counter(0);
}

#if 0
static volatile unsigned int _timer_h = 0; 
void _timerHander(unsigned int arg)
{
	_timer_h++; 
	__tcu_clear_full_match_flag(1);
}

//time unit is 1/6 usecond
static unsigned int time_last;

void StartTimer(void)
{
    REG_TCU_OSTCSR = TCU_TCSR_PWM_SD;   //Shut down PWM

    REG_TCU_OSTDR = -1L;                //Set match
    REG_TCU_OSTCR = 0;

    REG_TCU_OSTCSR |= TCU_TCSR_EXT_EN | (1<<3); //Select external clock, 1/4 extclk

    __tcu_clear_ost_match_flag();       //Clear match flag
    __tcu_mask_ost_match_irq();         //mask match irq
    __tcu_start_ost_clock();            //Supply clock
    __tcu_start_ost_counter();          //Start conuter

//	request_irq(IRQ_TCU0,timerHander,0);//Register irq handle
    time_last = 0;
}

void StopTimer(void)
{
    __tcu_stop_ost_counter();
    __tcu_stop_ost_clock();
}

void ResetTimer(void)
{
//    time_last = 0;
    REG_TCU_OSTCR = 0;
}

unsigned int GetTimer(void)
{
    unsigned int end, diff;

    end = REG_TCU_OSTCR;
//    diff = end - time_last;
//    time_last = end;
//    return diff;

	return end;
}

void SyncStartTimer(void)
{
	__tcu_disable_pwm_output(5);
	__tcu_select_extalclk(5);
	__tcu_select_clk_div16(5);
	
	__tcu_mask_half_match_irq(5);
	__tcu_mask_full_match_irq(5);

	REG_TCU_TDFR(5) = 65535;
	REG_TCU_TDHR(5) = 65535;
	REG_TCU_TCNT(5) = 0;

	__tcu_clear_full_match_flag(5);
	__tcu_start_counter(5);
}

void SyncResetTimer(void)
{
	REG_TCU_TCNT(5) = 0;
	__tcu_clear_full_match_flag(5);
	__tcu_start_counter(5);
}

inline unsigned int SyncGetTimer(void)
{
	return(REG_TCU_TCNT(5));
}

void SyncPauseTimer()
{
	__tcu_stop_counter(5);
}
#endif

/*
*	Function: start system timer, call this function will reset the timer and
*		counting from zero
*/
void _StartSysTimer(void)
{
	//stop OST
	REG_TCU_OSTCSR = 0;
	__tcu_stop_ost_counter();

	//24MHz, 1/1024, T= 42.667us
	REG_TCU_OSTCSR = 0x822C;
	REG_TCU_OSTDR = -1;	//compare
	REG_TCU_OSTCR = 0;	//count
	__tcu_mask_ost_match_irq();
	__tcu_clear_ost_match_flag();

	//go
	__tcu_start_ost_counter();
}

/*
*	Function: get system time
*/
unsigned int getSysTime(void)
{
	return(REG_TCU_OSTCR);
}

/*
*	Function: register a timer interruptting periodly
*	channel: timer id, from 0 to 1
*	period: interrupt period, unit is us, period < 2.5s
*	handle: interrupt handle
*	arg: argument to the interrupt handle
*/
int initTimer(unsigned int channel, unsigned int period, void (*handle)(unsigned int), int arg)
{
	unsigned int m, n;
	float fa;

	if(0 == channel) channel = 4;
	else if(1 == channel) channel = 5;
	else return -1;

	__tcu_disable_pwm_output(channel);
	__tcu_select_extalclk(channel);

	fa = (float)period;
	fa = fa/0.04166667;	//24MHz, 41.66667 ns

	//m = period/41667;
	//n = period%41667;
	//m *= 1000000;
	//n *= 1000000;
	//n /= 41667;
	//m += n;

	m = (unsigned int)fa;
	n = 0;
	while(m > 65535) {
		n += 1;
		m >>= 2;
	}
	if(n > 5) n= 5;
	//Set div
	REG_TCU_TCSR(channel) = (REG_TCU_TCSR(channel) & ~TCU_TCSR_PRESCALE_MASK) | (n<<TCU_TCSR_PRESCALE_BIT);

	__tcu_mask_half_match_irq(channel);
	__tcu_mask_full_match_irq(channel);
	REG_TCU_TDFR(channel) = m;
	REG_TCU_TDHR(channel) = 0;
	REG_TCU_TCNT(channel) = 0;

	if(NULL != handle)
	{
		if(5 == channel)
			request_irq(IRQ_TCU1, handle, arg);
		else
			request_irq(IRQ_TCU2, handle, arg);

		__tcu_unmask_full_match_irq(channel);
	}
}
unsigned int readTimer(unsigned int channel);

void runTimer(unsigned int channel)
{
	if(0 == channel)
		__tcu_start_counter(4);
	else if(1 == channel)
		__tcu_start_counter(5);
}

void stopTimer(unsigned int channel)
{
	if(0 == channel)
		__tcu_stop_counter(4);
	else if(1 == channel)
		__tcu_stop_counter(5);
}

void resetTimer(unsigned int channel)
{
	if(0 == channel)
		REG_TCU_TCNT(4) = 0;
	else if(1 == channel)
		REG_TCU_TCNT(5) = 0;
}

unsigned int readTimer(unsigned int channel)
{
	if(0 == channel)
		return(REG_TCU_TCNT(4));
	else if(1 == channel)
		return(REG_TCU_TCNT(5));
}


//#define TEST_TIME
#ifdef TEST_TIME
void JZ_timerHander_test(unsigned int arg)
{
	__tcu_clear_full_match_flag(arg);
	printf("arg = %d!\n",arg);
	
}
void JZ_StartTimer_test(unsigned int d)
{
	cli();
	__tcu_disable_pwm_output(d);
	__tcu_select_extalclk(d);
	__tcu_select_clk_div4(d);
	
	__tcu_mask_half_match_irq(d); 
	__tcu_unmask_full_match_irq(d);

	REG_TCU_TDFR(d) = 60000;
	REG_TCU_TDHR(d) = 60000;

	__tcu_clear_full_match_flag(d);
	__tcu_start_counter(d);
	request_irq(IRQ_TCU2,JZ_timerHander_test,d);
  sti();
	
}

#endif
