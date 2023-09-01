// TODO ToG: Add license header

#ifndef MARIAN_DMA_NG_H
#define MARIAN_DMA_NG_H

#include "device_generic.h"

//int dma_reset_engine(struct generic_chip *chip);
irqreturn_t dma_irq_handler(int irq, void *dev_id);
int dma_prepare(struct generic_chip *chip, unsigned int channels,
	bool playback, u64 host_base_addr, unsigned int num_blocks);
int dma_start(struct generic_chip *chip);
int dma_stop(struct generic_chip *chip);
// void dma_enable_interrupts(struct generic_chip *chip);
// void dma_disable_interrupts(struct generic_chip *chip);

#endif