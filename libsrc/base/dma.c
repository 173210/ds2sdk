/*******************************************************************************
*   File: dma.c
*   Version: 1.0
*   Author: jz
*   Date: 20060831
*   Description: first version
*
*-------------------------------------------------------------------------------
*   Modified by: dy
*   M_date:
*   M_version: 1.01
*   Description:
*******************************************************************************/

#include "jz4740.h"

#define NUM_DMA MAX_DMA_NUM
#define DMAC_DCCSR_DS_MASK DMAC_DCMD_DS_MASK
#define DMAC_DCCSR_DS_8b DMAC_DCMD_DS_8BIT
#define DMAC_DCCSR_DS_16b DMAC_DCMD_DS_16BIT
#define DMAC_DCCSR_DS_32b DMAC_DCMD_DS_32BIT
#define DMAC_DCCSR_DS_16B DMAC_DCMD_DS_16BYTE
#define DMAC_DCCSR_DS_32B DMAC_DCMD_DS_32BYTE
#define DMAC_DCCSR_DWDH_MASK DMAC_DCMD_DWDH_MASK
#define DMAC_DCCSR_DWDH_8 DMAC_DCMD_DWDH_8
#define DMAC_DCCSR_DWDH_16 DMAC_DCMD_DWDH_16
#define DMAC_DCCSR_DWDH_32 DMAC_DCMD_DWDH_32
#define DMAC_DCCSR_SWDH_MASK DMAC_DCMD_SWDH_MASK
#define DMAC_DCCSR_SWDH_8 DMAC_DCMD_SWDH_8
#define DMAC_DCCSR_SWDH_16 DMAC_DCMD_SWDH_16
#define DMAC_DCCSR_SWDH_32 DMAC_DCMD_SWDH_32
#define DMAC_DCCSR_TC DMAC_DCCSR_TT
#define REG_DMAC_DDAR(ch) REG_DMAC_DTAR(ch)

#define PHYSADDR(x) ((x) & 0x1fffffff)

static unsigned int dma_used[NUM_DMA];
static unsigned int dma_unit_size[NUM_DMA]={0};
static unsigned int dma_irq[NUM_DMA];
static int inited = 0;

static unsigned int UNIT_SIZE[8]= {4, 1, 2, 16, 32, 0, 0, 0};

#if 0
void dma_get_count(int ch)
{
    return (REG_DMAC_DTCR(ch) & 0x0ffffff) * dma_unit_size[ch];
}
#endif

void dma_start(int ch, unsigned int srcAddr, unsigned int dstAddr,
	       unsigned int count)
{
	//set_dma_addr
	REG_DMAC_DSAR(ch) = PHYSADDR(srcAddr);
	REG_DMAC_DDAR(ch) = PHYSADDR(dstAddr);
	//set_dma_count
	REG_DMAC_DTCR(ch) = count / dma_unit_size[ch];
	//enable_dma
	REG_DMAC_DCCSR(ch) |= DMAC_DCCSR_NDES; /* No-descriptor transfer */
	__dmac_enable_channel(ch);
	if (dma_irq[ch])
		__dmac_channel_enable_irq(ch);
}

void dma_stop(int ch)
{
	int i;
    if(0 == dma_used[ch])
        return;

	REG_DMAC_DCCSR(ch) = 0;
	if (dma_irq[ch]) 
		__dmac_channel_disable_irq(ch);

    REG_DMAC_DCKE &= ~DMAC_DCKE_CHN_ON(ch);    //Close clock to this channel
    dma_used[ch] = 0;
    dma_irq[ch] = 0;
}

int dma_request(int ch, void (*irq_handler)(unsigned int), unsigned int arg,
		 unsigned int mode, unsigned int type)
{
	if (!inited) {
		inited = 1;
    
        unsigned int i;
        for(i= 0; i < NUM_DMA; i++)
        {
            dma_used[i] = 0;                //Initial to not used
            dma_irq[i] = 0;
        }

        __cpm_start_dmac();
		__dmac_enable_module();
	}

    if(dma_used[ch]) return -1;

    dma_used[ch] = 1;
    dma_unit_size[ch] = UNIT_SIZE[((mode & DMAC_DCMD_DS_MASK) >> DMAC_DCMD_DS_BIT)];
    REG_DMAC_DCKE |= DMAC_DCKE_CHN_ON(ch);
    REG_DMAC_DCCSR(ch) = 0;
	REG_DMAC_DRSR(ch) = type;
    REG_DMAC_DCMD(ch) = mode;
	if (irq_handler) {
		request_irq(IRQ_DMA_0 + ch, irq_handler, arg);
		dma_irq[ch] = 1;
	}
    else
		dma_irq[ch] = 0;

    return 0;
}

int dma_wait_finish(int ch)
{
    while (!__dmac_channel_transmit_end_detected(ch));
    return 0;
}

#if 0
void dump_jz_dma_channel(unsigned int dmanr)
{
	struct dma_chan *chan;

	if (dmanr > MAX_DMA_NUM)
		return;
	printf("DMA%d Registers:\n", dmanr);
	printf("  DMACR  = 0x%08x\n", REG_DMAC_DMACR);
	printf("  DSAR   = 0x%08x\n", REG_DMAC_DSAR(dmanr));
	printf("  DTAR   = 0x%08x\n", REG_DMAC_DTAR(dmanr));
	printf("  DTCR   = 0x%08x\n", REG_DMAC_DTCR(dmanr));
	printf("  DRSR   = 0x%08x\n", REG_DMAC_DRSR(dmanr));
	printf("  DCCSR  = 0x%08x\n", REG_DMAC_DCCSR(dmanr));
	printf("  DCMD  = 0x%08x\n", REG_DMAC_DCMD(dmanr));
	printf("  DDA  = 0x%08x\n", REG_DMAC_DDA(dmanr));
	printf("  DMADBR = 0x%08x\n", REG_DMAC_DMADBR);
}
#endif


