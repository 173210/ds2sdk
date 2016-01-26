#ifndef __DMA_H__
#define __DMA_H__

//register a DMA transfer request
//ch: channel id request, there are 6 channles, 
//irq_handler: the DMA interruption handle
//arg: argument to the handle
//mode: DMA mode, such as port width, address increased/fixed, and so on
//type: DMA request type
extern int dma_request(int ch, void (*irq_handler)(unsigned int), unsigned int arg,
		 unsigned int mode, unsigned int type);

//start DMA transfer, must request a DMA first
//ch: channel id
//srcAddr: DMA source address
//dstAddr: DMA destination address
//count: DMA transfer count, the total bytes due the mode in dma_request
extern void dma_start(int ch, unsigned int srcAddr, unsigned int dstAddr,
	       unsigned int count);

//Stop DMA transfer
extern void dma_stop(int ch);

//Wait DMA transfer over
extern int dma_wait_finish(int ch);

#endif //__DMA_H__

