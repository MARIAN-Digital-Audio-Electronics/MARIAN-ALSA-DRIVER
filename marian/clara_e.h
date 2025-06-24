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

#ifndef MARIAN_CLARA_E_H
#define MARIAN_CLARA_E_H

#define CLARA_E_DEVICE_ID 0x9050

// revisions this driver is able to handle
#define CLARA_E_MIN_HW_REVISION 0x09
#define CLARA_E_MAX_HW_REVISION 0x09

#define CLARA_E_CARD_NAME "ClaraE"

struct clara_e_chip {
	struct snd_pcm_hw_constraint_list 
		hw_constraints_period_sizes[CLOCK_MODE_CNT];
	u16 max_channels[CLOCK_MODE_CNT];
};

void clara_e_register_device_specifics(struct device_specifics *dev_specifics);
void clara_e_chip_free(struct generic_chip *chip);
int clara_e_pcm_open(struct snd_pcm_substream *substream);
int clara_e_pcm_close(struct snd_pcm_substream *substream);
int clara_e_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params);
int clara_e_pcm_hw_free(struct snd_pcm_substream *substream);
int clara_e_pcm_prepare(struct snd_pcm_substream *substream);
int clara_e_pcm_ioctl(struct snd_pcm_substream *substream,
	unsigned int cmd, void *arg);
int clara_e_pcm_trigger(struct snd_pcm_substream *substream, int cmd);
enum clock_mode clara_e_get_clock_mode(struct generic_chip *chip);

#endif
