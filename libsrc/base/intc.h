#ifndef __INTC_H__
#define __INTC_H__

//register a handle for irq
extern int request_irq(unsigned int irq, void (*handler)(unsigned int), unsigned arg);

//clear CPU's interrupt state and enable global interrupt
extern void sti(void);

//disable global interrupt
extern void cli(void);

//disable global interrupt and store the global interrupt state
//return: interrupt state
extern unsigned int spin_lock_irqsave(void);

//restore global interrupt state
extern void spin_unlock_irqrestore(unsigned int val);

#endif //__INTC_H__

