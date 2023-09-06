// TODO ToG: Add license header

#ifndef MARIAN_DMA_NG_H
#define MARIAN_DMA_NG_H

#include "device_generic.h"

irqreturn_t dma_irq_handler(int irq, void *dev_id);
int dma_ng_prepare(struct generic_chip *chip, unsigned int channels,
	bool playback, u64 host_base_addr, unsigned int num_blocks);
int dma_ng_start(struct generic_chip *chip);
int dma_ng_stop(struct generic_chip *chip);

#endif
