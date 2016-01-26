#ifndef __CACHE_H__
#define __CACHE_H__

//invalidate instruction cache
extern void __icache_invalidate_all(void);

//invalidate data cache
extern void __dcache_invalidate_all(void);

//data cache writeback
extern void __dcache_writeback_all(void);

//data cache writeback and invalidate
extern void _dcache_wback_inv(unsigned long addr, unsigned long size);

#endif //__CACHE_H__

