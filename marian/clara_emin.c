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

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <sound/pcm.h>
#include <sound/control.h>
#include <sound/pcm_params.h>
#include "dbg_out.h"
#include "device_abstraction.h"
#include "device_generic.h"
#include "clara.h"
#include "clara_e.h"
#include "clara_emin.h"
#include "dma_ng.h"

#define TIMER_INTERVAL_MS 1000

static struct snd_pcm_ops const playback_ops;
static struct snd_pcm_ops const capture_ops;
static void timer_callback(struct generic_chip *chip);
static int create_controls(struct generic_chip *chip);

static bool hw_revision_valid(u8 rev)
{
	if (rev < CLARA_EMIN_MIN_HW_REVISION)
		return false;
	if (rev > CLARA_EMIN_MAX_HW_REVISION)
		return false;
	return true;
}

static void get_hw_revision_range(struct valid_hw_revision_range *range)
{
	if (range == NULL)
		return;
	range->min = CLARA_EMIN_MIN_HW_REVISION;
	range->max = CLARA_EMIN_MAX_HW_REVISION;
}

/*
	CHIP MANAGEMENT FUNCTIONS
*/

// TODO ToG: the max period size does not only depend on the clock mode but
// also the actual number of used channels.
static const unsigned int period_sizes_cm48[] =	{ 16, 32, 48, 64, 96, 128, 192,
	256, 384, 512, 768, 1024, 2048, 4096};
static const unsigned int period_sizes_cm96[] = { 16, 32, 48, 64, 96, 128, 192,
	256, 384, 512, 768, 1024, 2048, 4096};
static const unsigned int period_sizes_cm192[] = { 16, 32, 48, 64, 96, 128, 192,
	256, 384, 512, 768, 1024, 1536, 2048, 4096};

static int chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip)
{
	int err = 0;
	struct generic_chip *chip = NULL;
	struct clara_chip *clara_chip = NULL;
	struct clara_e_chip *clara_e_chip = NULL;

	static const struct snd_pcm_hw_constraint_list 
		hw_constraints_period_sizes[CLOCK_MODE_CNT] = {
			{	.count = ARRAY_SIZE(period_sizes_cm48),
				.list = period_sizes_cm48,
				.mask = 0},
			{	.count = ARRAY_SIZE(period_sizes_cm96),
				.list = period_sizes_cm96,
				.mask = 0},
			{	.count = ARRAY_SIZE(period_sizes_cm192),
				.list = period_sizes_cm192,
				.mask = 0},
			{	.count = 0,
				.list = NULL,
				.mask = 0},
		};
	static const u16 max_channels[CLOCK_MODE_CNT] = {128, 128, 128, 0};

	err = clara_chip_new(card, pci_dev, &chip);
	if (err < 0)
		return err;
	clara_chip = chip->specific;

	clara_e_chip = kzalloc(sizeof(*clara_e_chip), GFP_KERNEL);
	if (clara_e_chip == NULL)
		return -ENOMEM;
	// clara e specific constraints
	{
		int i = 0;
		for (i = 0; i < CLOCK_MODE_CNT; i++) {
			clara_e_chip->hw_constraints_period_sizes[i] =
				hw_constraints_period_sizes[i];
			clara_e_chip->max_channels[i] = max_channels[i];
		}
	}
	chip->min_num_channels = 1;
	chip->max_num_channels = 512;
	clara_chip->max_num_dma_blocks = 128;
	clara_chip->channels_per_dma_slice = 128;
	// caps are the same for playback and capture
	chip->hw_caps_playback = (struct snd_pcm_hardware const) {
		.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_NONINTERLEAVED |
			SNDRV_PCM_INFO_JOINT_DUPLEX |
			SNDRV_PCM_INFO_SYNC_START |
			SNDRV_PCM_INFO_BLOCK_TRANSFER),
		.formats = SNDRV_PCM_FMTBIT_S24_3LE, //SNDRV_PCM_FMTBIT_S32_LE,
		.rates = (SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |
			SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 |
			SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000),
		.rate_min = 44100,
		.rate_max = 192000,
		.channels_min = chip->min_num_channels,
		.channels_max = chip->max_num_channels,
		.buffer_bytes_max = DMA_BLOCK_SIZE_BYTES *
			clara_chip->max_num_dma_blocks *
			DMA_NUM_PERIODS * chip->max_num_channels,
		.period_bytes_min = DMA_BLOCK_SIZE_BYTES *
			chip->min_num_channels,
		.period_bytes_max = DMA_BLOCK_SIZE_BYTES *
			clara_chip->max_num_dma_blocks *
			chip->max_num_channels,
		.periods_min = DMA_NUM_PERIODS,
		.periods_max = DMA_NUM_PERIODS,
		.fifo_size = 0,
	};

	clara_chip->specific = clara_e_chip;
	clara_chip->specific_free = clara_e_chip_free;

	*rchip = chip;
	return 0;

	/* If some more resources are acquired and something can go wrong
	 * do not forget to clean up! */
	// error:
	// kfree(clara_e_chip);
	// return err;
}

void clara_emin_register_device_specifics(struct device_specifics
	*dev_specifics)
{
	if (dev_specifics == NULL)
		return;
	dev_specifics->hw_revision_valid = hw_revision_valid;
	dev_specifics->get_hw_revision_range = get_hw_revision_range;
	dev_specifics->card_name = CLARA_EMIN_CARD_NAME;
	dev_specifics->chip_new = chip_new;
	dev_specifics->chip_free = generic_chip_free;
	dev_specifics->detect_hw_presence = clara_detect_hw_presence;
	dev_specifics->soft_reset = clara_soft_reset;
	dev_specifics->indicate_state = generic_indicate_state;
	dev_specifics->alloc_dma_buffers = clara_alloc_dma_buffers;
	dev_specifics->irq_handler = dma_ng_irq_handler;
	dev_specifics->pcm_playback_ops = &playback_ops;
	dev_specifics->pcm_capture_ops = &capture_ops;
	dev_specifics->timer_callback = timer_callback;
	dev_specifics->timer_interval_ms = TIMER_INTERVAL_MS;
	dev_specifics->create_controls = create_controls;
}

/*
	PCM FUNCTIONS
*/
static struct snd_pcm_ops const playback_ops = {
	.open = clara_e_pcm_open,
	.close = clara_e_pcm_close,
	.ioctl = clara_e_pcm_ioctl,
	.hw_params = clara_e_pcm_hw_params,
	.hw_free = clara_e_pcm_hw_free,
	.prepare = clara_e_pcm_prepare,
	.trigger = clara_e_pcm_trigger,
	.pointer = clara_pcm_pointer,
};

static struct snd_pcm_ops const capture_ops = {
	.open = clara_e_pcm_open,
	.close = clara_e_pcm_close,
	.ioctl = clara_e_pcm_ioctl,
	.hw_params = clara_e_pcm_hw_params,
	.hw_free = clara_e_pcm_hw_free,
	.prepare = clara_e_pcm_prepare,
	.trigger = clara_e_pcm_trigger,
	.pointer = clara_pcm_pointer,
};

static int create_controls(struct generic_chip *chip)
{
	int err = 0;
	unsigned int ctl_id = 0;
	err = generic_read_wordclock_control_create(chip, "Sample Rate", 0,
		&ctl_id);
	if (err < 0)
		return err;
	else
		atomic_set(&chip->ctl_id_sample_rate, ctl_id);

	return 0;
}

static void timer_callback(struct generic_chip *chip)
{
	clara_timer_callback(chip);
	// update the clock mode
	atomic_set(&chip->clock_mode, clara_e_get_clock_mode(chip));
}
