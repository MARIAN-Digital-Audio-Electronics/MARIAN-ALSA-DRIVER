/*
 * MARIAN PCIe soundcards ALSA driver
 *
 * Author: Tobias Groß <theguy@audio-fpga.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details at:
 * http://www.gnu.org/licenses/gpl-2.0.html
 */

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
