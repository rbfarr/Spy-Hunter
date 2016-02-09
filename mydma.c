//Richard Farr (rfarr6)

#include "mydma.h"

void dma_copy(const void* src, void* dst, u16 count) {
	DMA_TRANSFER(src, dst, 3, count | DMA_ENABLE | DMA_SRC_INCR | DMA_DST_INCR);
}

void dma_fill(volatile const void* src, void* dst, u16 count) {
	DMA_TRANSFER(src, dst, 3, count | DMA_ENABLE | DMA_SRC_FIXED | DMA_DST_INCR);
}

void dma_drawrect3(int r, int c, int width, int height, volatile u16 color) {
	for (int i = 0; i < height; i++) {
		dma_fill(&color, &videoBuffer[OFFSET(r+i, c, SCREEN_WIDTH)], width);
	}
}

void dma_drawimage3(int r, int c, int width, int height, const u16* image) {
	for (int i = 0; i < height; i++) {
		dma_copy(image+width*i, &videoBuffer[OFFSET(r+i, c, SCREEN_WIDTH)], width);
	}
}

