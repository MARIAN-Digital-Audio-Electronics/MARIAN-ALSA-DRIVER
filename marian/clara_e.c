// TODO ToG: Add license header

#include <linux/types.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include "device_abstraction.h"
#include "device_generic.h"
#include "clara.h"
#include "clara_e.h"
#include "dma_ng.h"

#define MIN_NUM_CHANNELS 1
#define MAX_NUM_CHANNELS 2
#define NUM_PERIODS 2
#define MAX_NUM_BLOCKS 128
#define DMA_BLOCK_SIZE_BYTES (16*sizeof(u32))

struct clara_e_chip {
	u32 _dummy;
};
static struct snd_pcm_ops const playback_ops;
static struct snd_pcm_ops const capture_ops;

static bool hw_revision_valid(u8 rev)
{
	if (rev < CLARA_E_MIN_HW_REVISION)
		return false;
	if (rev > CLARA_E_MAX_HW_REVISION)
		return false;
	return true;
}

static void get_hw_revision_range(struct valid_hw_revision_range *range)
{
	if (range == NULL)
		return;
	range->min = CLARA_E_MIN_HW_REVISION;
	range->max = CLARA_E_MAX_HW_REVISION;
}

static void chip_free(struct generic_chip *chip)
{
	struct clara_chip *clara_chip = chip->specific;
	struct clara_e_chip *clara_e_chip = clara_chip->specific;

	if (clara_chip == NULL)
		return;
	if (clara_e_chip == NULL)
		return;
	kfree(clara_e_chip);
	clara_chip->specific = NULL;
	clara_chip->specific_free = NULL;
};

static int chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip)
{
	int err = 0;
	struct generic_chip *chip = NULL;
	struct clara_chip *clara_chip = NULL;
	struct clara_e_chip *clara_e_chip = NULL;

	err = clara_chip_new(card, pci_dev, &chip);
	if (err < 0)
		return err;

	clara_e_chip = kzalloc(sizeof(*clara_e_chip), GFP_KERNEL);
	if (clara_e_chip == NULL)
		return -ENOMEM;
	clara_e_chip->_dummy = 0x5CA1AB1E;

	clara_chip = chip->specific;
	clara_chip->specific = clara_e_chip;
	clara_chip->specific_free = chip_free;

	*rchip = chip;
	return 0;

	/* If some more resources are acquired and something can go wrong
	 * do not forget to clean up! */
	// error:
	// kfree(clara_e_chip);
	// return err;
};

void clara_e_register_device_specifics(struct device_specifics *dev_specifics)
{
	if (dev_specifics == NULL)
		return;
	dev_specifics->hw_revision_valid = hw_revision_valid;
	dev_specifics->get_hw_revision_range = get_hw_revision_range;
	dev_specifics->card_name = CLARA_E_CARD_NAME;
	dev_specifics->chip_new = chip_new;
	dev_specifics->chip_free = generic_chip_free;
	dev_specifics->detect_hw_presence = clara_detect_hw_presence;
	dev_specifics->soft_reset = clara_soft_reset;
	dev_specifics->irq_handler = dma_irq_handler;
	dev_specifics->pcm_playback_ops = &playback_ops;
	dev_specifics->pcm_capture_ops = &capture_ops;
}

struct snd_pcm_hardware const hw_playback = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_NONINTERLEAVED |
		SNDRV_PCM_INFO_JOINT_DUPLEX | SNDRV_PCM_INFO_SYNC_START |
		SNDRV_PCM_INFO_BLOCK_TRANSFER),
	.formats = SNDRV_PCM_FMTBIT_S24_3LE, //SNDRV_PCM_FMTBIT_S32_LE,
	.rates = (SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |
		SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 |
		SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000),
	.rate_min = 44100,
	.rate_max = 192000,
	.channels_min = MIN_NUM_CHANNELS,
	.channels_max = MAX_NUM_CHANNELS,
	.buffer_bytes_max = DMA_BLOCK_SIZE_BYTES * MAX_NUM_BLOCKS * NUM_PERIODS * MAX_NUM_CHANNELS,
	.period_bytes_min = DMA_BLOCK_SIZE_BYTES * MIN_NUM_CHANNELS,
	.period_bytes_max = DMA_BLOCK_SIZE_BYTES * MAX_NUM_BLOCKS * MAX_NUM_CHANNELS,
	.periods_min = NUM_PERIODS,
	.periods_max = NUM_PERIODS,
	.fifo_size = 0,
};
struct snd_pcm_hardware const hw_capture = hw_playback;

static const unsigned int period_sizes[] = { 512, };
static const struct snd_pcm_hw_constraint_list hw_constraints_period_sizes = {
	.count = ARRAY_SIZE(period_sizes),
	.list = period_sizes,
	.mask = 0
};

static int pcm_playback_open(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	snd_printk(KERN_INFO "pcm_playback_open\n");
	snd_pcm_set_sync(substream);
	substream->runtime->hw = hw_playback;
	snd_pcm_set_runtime_buffer(substream, &chip->playback_buf);
	chip->playback_substream = substream;
	snd_pcm_hw_constraint_list(substream->runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, &hw_constraints_period_sizes);
	return 0;
}

static int pcm_capture_open(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	snd_printk(KERN_DEBUG "pcm_capture_open\n");
	snd_pcm_set_sync(substream);
	substream->runtime->hw = hw_capture;
	snd_pcm_set_runtime_buffer(substream, &chip->capture_buf);
	chip->capture_substream = substream;
	snd_pcm_hw_constraint_list(substream->runtime, 0, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, &hw_constraints_period_sizes);
	return 0;
}

static int pcm_playback_close(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	snd_printk(KERN_DEBUG "pcm_playback_close\n");
	chip->playback_substream = NULL;
	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int pcm_capture_close(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	snd_printk(KERN_DEBUG "pcm_capture_close\n");
	chip->capture_substream = NULL;
	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params)
{
	int err = 0;
	struct generic_chip *chip = snd_pcm_substream_chip(substream);

	snd_printk(KERN_DEBUG "pcm_hw_params\n");
	printk(KERN_DEBUG "  buffer bytes: %d\n",
		params_buffer_bytes(hw_params));
	printk(KERN_DEBUG "  buffer size : %d\n",
		params_buffer_size(hw_params));
	printk(KERN_DEBUG "  period bytes: %d\n",
		params_period_bytes(hw_params));
	printk(KERN_DEBUG "  period size : %d\n",
		params_period_size(hw_params));
	printk(KERN_DEBUG "  periods     : %d\n",
		params_periods(hw_params));
	printk(KERN_DEBUG "  channels    : %d\n",
		params_channels(hw_params));

	chip->num_buffer_frames = params_buffer_size(hw_params);
	return 0;
}

static int pcm_hw_free(struct snd_pcm_substream *substream)
{
	snd_printk(KERN_DEBUG "pcm_hw_free\n");
	return 0;
}

static int pcm_prepare(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	u64 base_addr = substream->runtime->dma_addr;
	unsigned int channels = substream->runtime->channels;
	unsigned int no_blocks = 0;
	snd_printk(KERN_DEBUG "pcm_prepare\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_printk(KERN_DEBUG "pcm_prepare: playback base: %p\n",
			(void *)base_addr);
	}
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		snd_printk(KERN_DEBUG "pcm_prepare: capture base: %p\n",
			(void *)base_addr);
	}

	no_blocks = substream->runtime->period_size / 16 * 2;
	snd_printk(KERN_DEBUG "pcm_prepare: no_blocks: %d\n", no_blocks);
	dma_prepare(chip, channels,
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK),
		base_addr, no_blocks);
	return 0;
}

static int channel_dma_offset(struct snd_pcm_substream *substream,
				    struct snd_pcm_channel_info *info)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	unsigned int channel = info->channel;
	int size = substream->runtime->buffer_size;

	info->offset = 0;
	info->first = channel * chip->num_buffer_frames * sizeof(u32) * 8 + 8;
	info->step = 32;
	snd_printk(KERN_DEBUG "channel_dma_offset: channel: %d, offset: %d\n",
		channel, info->first/8);
	return 0;
}


static int pcm_ioctl(struct snd_pcm_substream *substream,
	unsigned int cmd, void *arg)
{
	switch (cmd) {
	case SNDRV_PCM_IOCTL1_CHANNEL_INFO:
		return channel_dma_offset(substream, arg);
	default:
		break;
	}

	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

static int pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		snd_printk(KERN_DEBUG "pcm_trigger: start\n");
		dma_start(snd_pcm_substream_chip(substream));
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		snd_printk(KERN_DEBUG "pcm_trigger: stop\n");
		dma_stop(snd_pcm_substream_chip(substream));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static snd_pcm_uframes_t pcm_pointer(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	return generic_get_sample_counter(chip);
}

static struct snd_pcm_ops const playback_ops = {
	.open = pcm_playback_open,
	.close = pcm_playback_close,
	.ioctl = pcm_ioctl,
	.hw_params = pcm_hw_params,
	.hw_free = pcm_hw_free,
	.prepare = pcm_prepare,
	.trigger = pcm_trigger,
	.pointer = pcm_pointer,
};

static struct snd_pcm_ops const capture_ops = {
	.open = pcm_capture_open,
	.close = pcm_capture_close,
	.ioctl = pcm_ioctl,
	.hw_params = pcm_hw_params,
	.hw_free = pcm_hw_free,
	.prepare = pcm_prepare,
	.trigger = pcm_trigger,
	.pointer = pcm_pointer,
};
