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
#include <sound/core.h>
#include "device_generic.h"

struct clara_chip {
	unsigned long bar1_addr;
	void __iomem *bar1;
	void *specific;
	chip_free_func specific_free;
};

#define write_reg32_bar1(chip, reg, val) \
	iowrite32((val), ((struct clara_chip *)(chip->specific))->bar1 + (reg))
#define read_reg32_bar1(chip, reg) \
	ioread32(((struct clara_chip *)(chip->specific))->bar1 + (reg))

int clara_chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip);
int clara_alloc_dma_buffers(struct pci_dev *pci_dev,
	struct generic_chip *chip,
	size_t playback_size, size_t capture_size);
bool clara_detect_hw_presence(struct generic_chip *chip);
/* Resets all relevant registers to default values mainly
 * to avoid spurious interrupts. */
void clara_soft_reset(struct generic_chip *chip);
void clara_timer_callback(struct generic_chip *chip);
