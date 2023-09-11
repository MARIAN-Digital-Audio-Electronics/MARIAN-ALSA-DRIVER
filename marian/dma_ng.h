// TODO ToG: Add license header

#ifndef MARIAN_DMA_NG_H
#define MARIAN_DMA_NG_H

#include "device_generic.h"

#define DMA_SAMPLES_PER_BLOCK 16
#define DMA_NUM_PERIODS 2
#define DMA_MAX_NUM_BLOCKS 1024

irqreturn_t dma_ng_irq_handler(int irq, void *dev_id);
int dma_ng_prepare(struct generic_chip *chip, unsigned int channels,
	bool playback, u64 host_base_addr, unsigned int num_blocks);
int dma_ng_start(struct generic_chip *chip);
int dma_ng_stop(struct generic_chip *chip);
int dma_ng_disable_interrupts(struct generic_chip *chip);
int dma_ng_disable_channels(struct generic_chip *chip, bool playback);

#endif
