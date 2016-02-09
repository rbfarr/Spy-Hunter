//Richard Farr (rfarr6)
//Implementation of mydma.c

#ifndef MYDMA_H
#define MYDMA_H
#include "mylib.h"

#define DMA ((volatile dma_t*)0x40000B0)

typedef struct {
	const void* src;
	void* dst;
	unsigned int cnt; // Least significant 16 bits are transfer count. Higher-order bits are DMA control.
} dma_t;

#define DMA_TRANSFER(_src,_dst,_ch,_mode)		\
	do {									\
		DMA[_ch].cnt = 0;					\
		DMA[_ch].src = (const void*)(_src);	\
		DMA[_ch].dst = (void*)(_dst);			\
		DMA[_ch].cnt = (_mode);				\
	} while (0)							

#define DMA_DST_INCR 0
#define DMA_DST_DECR (1 << 21)
#define DMA_DST_FIXED (1 << 22)
#define DMA_DST_RESET (0x3 << 21)
#define DMA_SRC_INCR 0
#define DMA_SRC_DECR (1 << 23)
#define DMA_SRC_FIXED (1 << 24)
#define DMA_REPEAT (1 << 25)
#define DMA_TRANSFER_32 (1 << 26)
#define DMA_START_NOW 0
#define DMA_START_VBLANK (1 << 28)
#define DMA_START_HBLANK (1 << 29)
#define DMA_ENABLE (1 << 31)

void dma_copy(const void*, void*, u16);
void dma_fill(volatile const void*, void*, u16);
void dma_drawrect3(int, int, int, int, volatile u16);
void dma_drawimage3(int, int, int, int, const u16*);

#endif




