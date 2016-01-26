//memory.c

#include<stdio.h>

#define MEM_U8 unsigned char 
#define MEM_ULONG unsigned int 

#define TRUE		1
#define FALSE		0

#define FREE		0
#define RESERVED	1

/* Memory block header = 9 bytes = 4 previous + 4 next + status */
#define SIZE_HEADER	16

#define prev(i)			(*((MEM_ULONG *) (i)))
#define next(i)			(*((MEM_ULONG *) (i+4)))

#define status(i)		(*((MEM_U8 *) (i+8)))
#define setalign(i,y)	(*((MEM_U8 *)(i-1))) = y
#define getalign(i)		(*((MEM_U8 *)(i-1)))

#define size(i)			(next(i)-i-SIZE_HEADER)

/* if going to split free block, need at least 8 bytes in new free part */
#define MIN_FREE_BYTES   4
//static MEM_U8 memory[MEM_LENGHT] __attribute__ ((aligned (MIN_FREE_BYTES)));

static MEM_ULONG first;				/*stores address of first byte of heap*/
static MEM_ULONG last;				/*store address of last byte of heap + 1*/

void heapInit(unsigned int start, unsigned int end)
{
//	first = (MEM_ULONG)&memory[0];
//	last = (MEM_ULONG)&memory[MEM_LENGHT - 1];

	first = (start+3) & (~3);
	last = end & (~3);

	prev(first)=0;
	next(first)=0;
	status(first)=FREE;
}

static int currentNodeAlloc(MEM_ULONG i,MEM_ULONG nbytes)
{
	MEM_ULONG free_size;

	/*handle case of current block being the last*/
 	if(next(i) == 0){
		free_size = last - i - SIZE_HEADER;
	}else{
	    free_size = size(i);
	}
	
	/*either split current block, use entire current block, or fail*/
	if(free_size >= nbytes + SIZE_HEADER + MIN_FREE_BYTES)
	{
		MEM_ULONG old_next;
		MEM_ULONG old_block;
		MEM_ULONG new_block;

		old_next = next(i);
		old_block = i;

		/*fix up original block*/
		next(i) = i+SIZE_HEADER+nbytes;
		new_block = next(i);
		status(i)=RESERVED;

		/*set up new free block*/
		i = next(i);

		next(i)=old_next;
		prev(i)=old_block;
		status(i)=FREE;

		/*right nieghbor must point to new free block*/
		if(next(i)!=0)			
		{
			i = next(i);
			prev(i)=new_block;
		}
		
		return(TRUE);
	}
	else if(free_size >= nbytes)
	{
		status(i)=RESERVED;

		return(TRUE);
	}

	return(FALSE);
}

static MEM_ULONG loc_alloc(MEM_ULONG nbytes)
{
	int ret;
	MEM_ULONG i;

	nbytes = ((nbytes  + MIN_FREE_BYTES - 1)/ MIN_FREE_BYTES )  * MIN_FREE_BYTES;
	i=first;

	if(status(i)==FREE)
	{
		ret = currentNodeAlloc(i,nbytes);
		if(ret==TRUE) {
			return(i+SIZE_HEADER);
		}
	}

	while(next(i)!=0)
	{
		i=next(i);
		if(status(i)==FREE)
		{
			ret = currentNodeAlloc(i,nbytes);
			if(ret==TRUE) {
				return(i+SIZE_HEADER);
			}
		}
	}

    return 0;
}

void* Drv_alloc(MEM_ULONG nbytes)
{
	MEM_ULONG i = loc_alloc(nbytes);
	if(i != 0) setalign(i,0);

	return ((void*)i);
}

#if 0
MEM_ULONG alignAlloc(MEM_ULONG align, MEM_ULONG size)
{
	MEM_ULONG i = loc_alloc(size + align);
	if(i != 0)
	{
		align = i - (i / align) * align;
		i += align;
		setalign(i,align);    
	}

	return i;
}
#endif

static void loc_free(MEM_ULONG address)
{
	MEM_ULONG block;
	MEM_ULONG lblock;
	MEM_ULONG rblock;
  
	block = address-SIZE_HEADER;
	lblock = prev(block);
	rblock = next(block);

	/*
	4 cases: FFF->F, FFR->FR, RFF->RF, RFR 
	always want to merge free blocks 
	*/

	if((lblock!=0)&&(rblock!=0)&&(status(lblock)==FREE)&&(status(rblock)==FREE))
	{
		next(lblock)=next(rblock);
		status(lblock)=FREE;
		if(next(rblock)!=0)	prev(next(rblock))=lblock;
	}
	else if((lblock!=0)&&(status(lblock)==FREE))
	{
		next(lblock)=next(block);
		status(lblock)=FREE;
		if(next(block)!=0)	prev(next(block))=lblock;
	}
	else if((rblock!=0)&&(status(rblock)==FREE))
	{
		next(block)=next(rblock);
		status(block)=FREE;
		if(next(rblock)!=0)	prev(next(rblock))=block;
	}
	else
	{
		status(block)=FREE;
	}
}

/*Note: disaster will strike if fed wrong address*/
void Drv_deAlloc(void* address)
{
	address -= getalign(address);
	loc_free((unsigned int)address);
}

void* Drv_realloc(void* address,MEM_ULONG nbytes)
{
	MEM_ULONG addr,oldsize;
	MEM_ULONG block,rblock,rrblock;
	MEM_ULONG bsize,rbsize,align;
	void* rr;

	oldsize = nbytes;
	nbytes = ((nbytes  + MIN_FREE_BYTES - 1)/ MIN_FREE_BYTES )  * MIN_FREE_BYTES;

	rr = address;
	if (nbytes == 0) {
		Drv_deAlloc(rr);

		return ((void*)0);
	}
	if (address == 0)
	{
		addr = loc_alloc(nbytes);
	    if(addr != 0)	setalign(addr,0);

	    return ((void*)addr);
	}

	align = getalign(address);
	address -= getalign(address);
	address -= SIZE_HEADER;    
	bsize = size((unsigned int)address);

	if(nbytes <= bsize-align) {
		return ((void*)rr);
	}

	rblock = next((unsigned int)address);
	if((rblock != 0) &&(status(rblock) == FREE) )
	{
		bsize += size(rblock);
		if(bsize >= nbytes + align)
		{
			rrblock = next(rblock);
			next((unsigned int)address) = ((unsigned int)address + nbytes + align + SIZE_HEADER);
			block = next((unsigned int)address);
			prev(block) = (unsigned int)address;
			next(block) = rrblock; 
			status(block) = FREE;
			return ((void*)rr);
		}
	}
	addr = loc_alloc(nbytes+align);

	if(addr == 0) {
		return ((void*)0);
	}

	addr += align;		
	setalign(addr,align);	
	memcpy(addr,rr,bsize);
	Drv_deAlloc(rr);

	return ((void*)addr);
}

void* Drv_calloc(MEM_ULONG nmem,MEM_ULONG size)
{
	void* rr;

	rr = Drv_alloc(size * nmem);
	memset(rr,0,size * nmem);

	return ((void*)rr);
}

#if 0
static const char *statStr[] = {"FREE","RESERVED"};

void printMemory()
{
	MEM_ULONG i;
	i=first;
	printf("[0x%08x,0x%08x,0x%08x,%s]\n",prev(i),i,next(i),statStr[status(i)]);
	while(next(i)!=0)
	{
		i=next(i);
		printf("[0x%08x,0x%08x,0x%08x,%s]\n",prev(i),i,next(i),statStr[status(i)]);
	}
	return;
}
#endif


