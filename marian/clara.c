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

#include <linux/pci.h>
#include <sound/pcm.h>
#include "dbg_out.h"
#include "device_generic.h"
#include "dma_ng.h"
#include "clara.h"

#define FPGA_MAGIC_WORD 0xAD10F96A
#define ADDR_MAGIC_WORD_REG 0xF0

static int acquire_pci_resources(struct generic_chip *chip);
static void release_pci_resources(struct generic_chip *chip);

/*
	CHIP MANAGEMENT FUNCTIONS
*/

static void chip_free(struct generic_chip *chip)
{
	struct clara_chip *clara_chip = chip->specific;

	if (clara_chip == NULL)
		return;
	if (clara_chip->specific_free != NULL)
		clara_chip->specific_free(chip);
	release_pci_resources(chip);
	kfree(clara_chip);
	chip->specific = NULL;
	chip->specific_free = NULL;
}

int clara_chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip)
{
	int err = 0;
	struct generic_chip *chip = NULL;
	struct clara_chip *clara_chip = NULL;

	err = generic_chip_new(card, pci_dev, &chip);
	if (err < 0)
		return err;

	clara_chip = kzalloc(sizeof(*clara_chip), GFP_KERNEL);
	if (clara_chip == NULL)
		return -ENOMEM;

	clara_chip->bar1_addr = 0;
	clara_chip->bar1 = NULL;
	clara_chip->specific = NULL;
	clara_chip->specific_free = NULL;

	chip->specific = clara_chip;
	chip->specific_free = chip_free;

	/* get PCI resources presumes that the generic chip function has
	 * already acquired PCI regions and BAR0. */
	err = acquire_pci_resources(chip);
	if (err < 0)
		goto error;

	*rchip = chip;
	return 0;

error:
	release_pci_resources(chip);
	kfree(clara_chip);
	return err;
}

static int acquire_pci_resources(struct generic_chip *chip)
{
	struct clara_chip *clara_chip = NULL;
	if (chip == NULL)
		return -EINVAL;
	if (chip->pci_dev == NULL)
		return -EINVAL;
	if (chip->specific == NULL)
		return -EINVAL;
	clara_chip = chip->specific;

	clara_chip->bar1_addr = pci_resource_start(chip->pci_dev, 1);
	clara_chip->bar1 = ioremap(clara_chip->bar1_addr,
		pci_resource_len(chip->pci_dev, 1));
	if (clara_chip->bar1 == NULL) {
		PRINT_ERROR("BAR1: ioremap error\n");
		return -ENXIO;
	}

	PRINT_DEBUG("BAR1: ioremap success\n");
	return 0;
}

static void release_pci_resources(struct generic_chip *chip)
{
	struct clara_chip *clara_chip = NULL;
	if (chip == NULL)
		return;
	if (chip->pci_dev == NULL)
		return;
	if (chip->specific == NULL)
		return;
	clara_chip = chip->specific;

	if (clara_chip->bar1 != NULL)
		iounmap(clara_chip->bar1);
	clara_chip->bar1 = NULL;
	clara_chip->bar1_addr = 0;

	PRINT_DEBUG("BAR1: iounmap success\n");
}

/*
	HARDWARE SPECIFIC FUNCTIONS
*/

bool clara_detect_hw_presence(struct generic_chip *chip)
{
	u32 val = read_reg32_bar0(chip, ADDR_MAGIC_WORD_REG);
	if (val == FPGA_MAGIC_WORD) {
		PRINT_INFO("FPGA detected, build no: %08X\n",
			generic_get_build_no(chip));
		return true;
	}

	return false;
}

void clara_soft_reset(struct generic_chip *chip)
{
	// TODO ToG: reset IRQs / DMA engine
}

static int alloc_dma_buffers(struct pci_dev *pci_dev,
	struct generic_chip *chip,
	size_t playback_size, size_t capture_size)
{
	struct snd_dma_buffer tmp_buf;
	if (snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV, &pci_dev->dev,
		capture_size, &tmp_buf) == 0) {
		PRINT_DEBUG("area = 0x%p\n", tmp_buf.area);
		PRINT_DEBUG("addr = 0x%llu\n", tmp_buf.addr);
		PRINT_DEBUG("bytes = %zu\n", tmp_buf.bytes);
		chip->capture_buf = tmp_buf;
	} else {
		PRINT_ERROR(
			"snd_dma_alloc_dir_pages failed (capture)\n");
		return -ENOMEM;
	}
	if (snd_dma_alloc_pages(SNDRV_DMA_TYPE_DEV, &pci_dev->dev,
		playback_size, &tmp_buf) == 0) {
		PRINT_DEBUG("area = 0x%p\n", tmp_buf.area);
		PRINT_DEBUG("addr = 0x%llu\n", tmp_buf.addr);
		PRINT_DEBUG("bytes = %zu\n", tmp_buf.bytes);
		chip->playback_buf = tmp_buf;
	} else {
		PRINT_ERROR(
			"snd_dma_alloc_dir_pages failed (playback)\n");
		return -ENOMEM;
	}
	return 0;
}

int clara_alloc_dma_buffers(struct pci_dev *pci_dev,
	struct generic_chip *chip)
{
	struct clara_chip *clara_chip = chip->specific;
	return alloc_dma_buffers(pci_dev, chip,
		DMA_BLOCK_SIZE_BYTES * clara_chip->max_num_dma_blocks *
			chip->max_num_channels,
		DMA_BLOCK_SIZE_BYTES * clara_chip->max_num_dma_blocks *
			chip->max_num_channels);
}

void clara_timer_callback(struct generic_chip *chip)
{
	generic_timer_callback(chip);
}

/*
	PCM FUNCTIONS
*/

snd_pcm_uframes_t clara_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	return generic_get_sample_counter(chip);
}