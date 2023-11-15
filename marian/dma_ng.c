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

#include <linux/irqreturn.h>
#include "dbg_out.h"
#include "clara.h"
#include "dma_ng.h"

#define REG_ADDR_INCREASE 4 // register byte alignment
#define ADDR_RESET_DMA_ENGINE_REG 0x00
#define ADDR_PREPARE_RUN_REG 0x84
#define MASK_ENGINE_RUN 1
#define MASK_ENGINE_PREPARE (1<<1)
#define ADDR_BASE_PLAYBACK_CHANNELS_REGS 0x100
#define ADDR_BASE_CAPTURE_CHANNELS_REGS 0x180
#define ADDR_NUM_BLOCKS_REG 0x10
#define ADDR_IRQ_DISABLE_REG 0xAC
#define MASK_IRQ_DISABLE_PLAYBACK (1<<1)
#define MASK_IRQ_DISABLE_CAPTURE (1<<2)
#define MASK_IRQ_DMA_LOOPBACK (1<<3)
#define MASK_STATUS_IDLE (1<<4)
#define MASK_IRQ_STATUS_CAPTURE (1<<11)
#define MASK_STATUS_CAPTURE_PAGE (1<<12)
#define MASK_IRQ_STATUS_PREPARED (1<<14)
#define ADDR_NUM_SLICES_REG 0xB0
#define ADDR_BASE_PLAYBACK_HOST_ADDR_REGS 0x200
#define ADDR_BASE_CAPTURE_HOST_ADDR_REGS 0x300
#define ADDR_XILINX_H2C_REG 0x4
#define ADDR_XILINX_C2H_REG 0x1004
#define ADDR_XILINX_IRQ_ENABLE_REG 0x2004
// to make things not too complicated, we fix the number of channels per slice
#define CHANNELS_PER_SLICE 512
#define NUM_CHANNEL_ENABLE_REGS 16

static int enable_interrupts(struct generic_chip *chip)
{
	// enable xilinx core interrupts and transport engine
	write_reg32_bar1(chip, ADDR_XILINX_H2C_REG, 1);
	write_reg32_bar1(chip, ADDR_XILINX_C2H_REG, 1);
	write_reg32_bar1(chip, ADDR_XILINX_IRQ_ENABLE_REG, 1);
	// enable capture interrupts (besides the prepare IRQ the only one
	// we are interested in)
	// since at the moment we have no shadow registers, we need to
	// take care of the DMA loopback as well which is located in the
	// same register
	write_reg32_bar0(chip, ADDR_IRQ_DISABLE_REG,
		0xFFFFFFFF & ~(MASK_IRQ_DMA_LOOPBACK | MASK_IRQ_DISABLE_CAPTURE));
	return 0;
}

int dma_ng_disable_interrupts(struct generic_chip *chip)
{
	// disable xilinx core interrupts and transport engine
	write_reg32_bar1(chip, ADDR_XILINX_H2C_REG, 0);
	write_reg32_bar1(chip, ADDR_XILINX_C2H_REG, 0);
	write_reg32_bar1(chip, ADDR_XILINX_IRQ_ENABLE_REG, 0);
	// disable all interrupts
	write_reg32_bar0(chip, ADDR_IRQ_DISABLE_REG,
		MASK_IRQ_DISABLE_CAPTURE | MASK_IRQ_DISABLE_PLAYBACK);
	return 0;
}

static int reset_engine(struct generic_chip *chip)
{
	u32 val = 0;
	int retries = 5;
	PRINT_DEBUG("reset_engine");
	write_reg32_bar0(chip, ADDR_PREPARE_RUN_REG, 0);

	while (retries-- > 0) {
		val = generic_get_irq_status(chip);
		if (val & MASK_STATUS_IDLE) {
			chip->dma_status = DMA_STATUS_IDLE;
			PRINT_DEBUG("reset_engine: "
				"%d tries", 5 - retries);
			return 0;
		}
		write_reg32_bar0(chip, ADDR_RESET_DMA_ENGINE_REG, 0);
	}

	PRINT_ERROR("reset_engine: machine not idle after "
		"reset: 0x%08X\n", val);
	chip->dma_status = DMA_STATUS_UNKNOWN;
	return -EIO;
}

int dma_ng_prepare(struct generic_chip *chip, unsigned int channels,
	bool playback, u64 host_base_addr, unsigned int num_blocks)
{

	u32 channel_enables[NUM_CHANNEL_ENABLE_REGS] = {0};
	int i = 0;

	// the caller needs to make sure that this runs in a critical section
	if (chip->dma_status != DMA_STATUS_RUNNING)
		if (reset_engine(chip) < 0)
			return -EIO;

	for (i = 0; i < channels; i++) {
		channel_enables[i / 32] |= (1 << (i % 32));
	}
	for (i = 0; i < NUM_CHANNEL_ENABLE_REGS; i++) {
		write_reg32_bar0(chip, (playback ?
			ADDR_BASE_PLAYBACK_CHANNELS_REGS :
			ADDR_BASE_CAPTURE_CHANNELS_REGS) +
			i * REG_ADDR_INCREASE, channel_enables[i]);
	}
	write_reg32_bar0(chip, ADDR_NUM_BLOCKS_REG, num_blocks);
	write_reg32_bar0(chip, ADDR_NUM_SLICES_REG, CHANNELS_PER_SLICE);
	if (playback) {
		write_reg32_bar0(chip,
			ADDR_BASE_PLAYBACK_HOST_ADDR_REGS,
			LOW_ADDR(host_base_addr));
		write_reg32_bar0(chip,
			ADDR_BASE_PLAYBACK_HOST_ADDR_REGS + 4,
			HIGH_ADDR(host_base_addr));
	}
	else {
		write_reg32_bar0(chip,
			ADDR_BASE_CAPTURE_HOST_ADDR_REGS,
			LOW_ADDR(host_base_addr));
		write_reg32_bar0(chip,
			ADDR_BASE_CAPTURE_HOST_ADDR_REGS + 4,
			HIGH_ADDR(host_base_addr));
	}

	enable_interrupts(chip);
	return 0;
}

int dma_ng_start(struct generic_chip *chip)
{
	if (chip->dma_status != DMA_STATUS_IDLE)
		return -EIO;
	write_reg32_bar0(chip, ADDR_PREPARE_RUN_REG,
		MASK_ENGINE_PREPARE | MASK_ENGINE_RUN);
	chip->dma_status = DMA_STATUS_RUNNING;
	return 0;
}

int dma_ng_stop(struct generic_chip *chip)
{
	write_reg32_bar0(chip, ADDR_PREPARE_RUN_REG, 0);
	chip->dma_status = DMA_STATUS_IDLE;
	return 0;
}

int dma_ng_disable_channels(struct generic_chip *chip, bool playback)
{
	int i = 0;
	for (i = 0; i < NUM_CHANNEL_ENABLE_REGS; i++) {
		write_reg32_bar0(chip, (playback ?
			ADDR_BASE_PLAYBACK_CHANNELS_REGS :
			ADDR_BASE_CAPTURE_CHANNELS_REGS) +
			i * REG_ADDR_INCREASE, 0);
	}
	return 0;
}

irqreturn_t dma_ng_irq_handler(int irq, void *dev_id)
{
	struct generic_chip *chip = dev_id;
	u32 val = generic_get_irq_status(chip);
	if (val == 0)
		return IRQ_NONE;
	if (val & MASK_IRQ_STATUS_PREPARED) {
		PRINT_DEBUG("dma_ng_irq_handler: prepare IRQ\n");
	}
	if (val & MASK_IRQ_STATUS_CAPTURE) {
		if (chip->playback_substream)
			snd_pcm_period_elapsed(chip->playback_substream);
		if (chip->capture_substream)
			snd_pcm_period_elapsed(chip->capture_substream);
	}
	if (!chip->playback_substream && !chip->capture_substream) {
		dma_ng_disable_interrupts(chip);
		PRINT_ERROR("dma_ng_irq_handler: caught dangling IRQ\n");
	}
	return IRQ_HANDLED;
}
