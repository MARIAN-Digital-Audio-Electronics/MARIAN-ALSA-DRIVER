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

#ifndef MARIAN_GENERIC_H
#define MARIAN_GENERIC_H

#include <linux/pci.h>
#include <linux/atomic.h>
#include <sound/core.h>
#include <sound/pcm.h>

#define write_reg32_bar0(chip, reg, val) \
	iowrite32((val), (chip)->bar0 + (reg))
#define read_reg32_bar0(chip, reg) \
	ioread32((chip)->bar0 + (reg))
#define HIGH_ADDR(x) (sizeof (x) > 4 ? (x) >> 32 & 0xffffffff : 0)
#define LOW_ADDR(x) ((x) & 0xffffffff)

struct generic_chip;
enum dma_status {
	DMA_STATUS_UNKNOWN,
	DMA_STATUS_IDLE,
	DMA_STATUS_PREPARED,
	DMA_STATUS_RUNNING,
};
enum clock_mode {
	CLOCK_MODE_48 = 0,
	CLOCK_MODE_96 = 1,
	CLOCK_MODE_192 = 2,
};
extern char *clock_mode_names[];
typedef void (*timer_callback_func)(struct generic_chip *chip);

// ALSA specific free operation
int generic_chip_dev_free(struct snd_device *device);
typedef void (*chip_free_func)(struct generic_chip *chip);
// this needs to be public in case we need to rollback in driver_probe()
void generic_chip_free(struct generic_chip *chip);
struct generic_chip {
	struct snd_card *card;
	struct pci_dev *pci_dev;
	int irq;
	unsigned long bar0_addr;
	void __iomem *bar0;
	struct snd_pcm *pcm;
	// used in critical sections start
	spinlock_t lock;
	struct snd_pcm_substream *playback_substream;
	struct snd_pcm_substream *capture_substream;
	enum dma_status dma_status;
	unsigned int num_buffer_frames;
	// used in critical sections end
	struct snd_dma_buffer playback_buf;
	struct snd_dma_buffer capture_buf;
	struct task_struct *timer_thread;
	timer_callback_func timer_callback;
	unsigned long timer_interval_ms;
	atomic_t current_sample_rate;
	atomic_t clock_mode;
	// we want to store this control id to notify the user space of
	// sample rate changes
	atomic_t ctl_id_sample_rate;
	void *specific;
	chip_free_func specific_free;
};

int generic_chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip);

enum state_indicator {
	STATE_OFF = 0,
	STATE_SUCCESS = 1,
	STATE_FAILURE = 2,
	STATE_RESET = 3,
};
void generic_indicate_state(struct generic_chip *chip,
	enum state_indicator state);
int generic_dma_channel_offset(struct snd_pcm_substream *substream,
	struct snd_pcm_channel_info *info, unsigned long alignment);
int generic_pcm_ioctl(struct snd_pcm_substream *substream, unsigned int cmd,
	void *arg);
inline u32 generic_get_sample_counter(struct generic_chip *chip);
inline u32 generic_get_irq_status(struct generic_chip *chip);
inline u32 generic_get_build_no(struct generic_chip *chip);
void generic_timer_callback(struct generic_chip *chip);
enum clock_mode generic_sample_rate_to_clock_mode(unsigned int sample_rate);
unsigned int generic_measure_wordclock_hz(struct generic_chip *chip,
	unsigned int source);
int generic_read_wordclock_control_create(struct generic_chip *chip,
	char *label, unsigned int idx, unsigned int *rcontrol_id);
void generic_clear_dma_buffer(struct snd_dma_buffer *buf);

#endif
