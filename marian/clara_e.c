// TODO ToG: Add license header

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
#include "dma_ng.h"

#define MIN_NUM_CHANNELS 1 // Clara E
#define MAX_NUM_CHANNELS 512 // Clara E
#define MAX_NUM_BLOCKS 128 // Clara E
#define DMA_BLOCK_SIZE_BYTES (DMA_SAMPLES_PER_BLOCK*sizeof(u32))
#define TIMER_INTERVAL_MS 1000
#define ADDR_CLOCK_MODE_READ_REG 0x80
#define MASK_CLOCK_MODE 0x3

struct clara_e_chip {
	u32 _dummy;
};
static struct snd_pcm_ops const playback_ops;
static struct snd_pcm_ops const capture_ops;
static void timer_callback(struct generic_chip *chip);
static int create_controls(struct generic_chip *chip);

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
}

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
}

static int alloc_dma_buffers(struct pci_dev *pci_dev,
	struct generic_chip *chip)
{
	return clara_alloc_dma_buffers(pci_dev, chip,
		DMA_BLOCK_SIZE_BYTES * MAX_NUM_BLOCKS * MAX_NUM_CHANNELS,
		DMA_BLOCK_SIZE_BYTES * MAX_NUM_BLOCKS * MAX_NUM_CHANNELS);
}

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
	dev_specifics->indicate_state = generic_indicate_state;
	dev_specifics->alloc_dma_buffers = alloc_dma_buffers;
	dev_specifics->irq_handler = dma_ng_irq_handler;
	dev_specifics->pcm_playback_ops = &playback_ops;
	dev_specifics->pcm_capture_ops = &capture_ops;
	dev_specifics->timer_callback = timer_callback;
	dev_specifics->timer_interval_ms = TIMER_INTERVAL_MS;
	dev_specifics->create_controls = create_controls;
}

// capabilities are the same for playback and capture on Clara E
struct snd_pcm_hardware const hw_caps = {
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
	.buffer_bytes_max = DMA_BLOCK_SIZE_BYTES * MAX_NUM_BLOCKS *
		DMA_NUM_PERIODS * MAX_NUM_CHANNELS,
	.period_bytes_min = DMA_BLOCK_SIZE_BYTES * MIN_NUM_CHANNELS,
	.period_bytes_max = DMA_BLOCK_SIZE_BYTES * MAX_NUM_BLOCKS *
		MAX_NUM_CHANNELS,
	.periods_min = DMA_NUM_PERIODS,
	.periods_max = DMA_NUM_PERIODS,
	.fifo_size = 0,
};

// TODO ToG: the max period size does not only depend on the clock mode but
// also the actual number of used channels.
static const unsigned int period_sizes_cm48[] =	{ 16, 32, 48, 64, 96, 128, 192,
	256, 384, 512, 768, 1024};
static const unsigned int period_sizes_cm96[] = { 16, 32, 48, 64, 96, 128, 192,
	256, 384, 512, 768, 1024, 2048};
static const unsigned int period_sizes_cm192[] = { 16, 32, 48, 64, 96, 128, 192,
	256, 384, 512, 768, 1024, 1536, 2048, 4096};

static const struct snd_pcm_hw_constraint_list hw_constraints_period_sizes[] = {
	{	.count = ARRAY_SIZE(period_sizes_cm48),
		.list = period_sizes_cm48,
		.mask = 0},
	{	.count = ARRAY_SIZE(period_sizes_cm96),
		.list = period_sizes_cm96,
		.mask = 0},
	{	.count = ARRAY_SIZE(period_sizes_cm192),
		.list = period_sizes_cm192,
		.mask = 0},
};

static const u16 max_channels[] = {512, 256, 128};

static int pcm_open(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	enum clock_mode cmode =
		generic_sample_rate_to_clock_mode(substream->runtime->rate);
	snd_pcm_set_sync(substream);
	snd_pcm_hw_constraint_list(substream->runtime, 0,
		SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
		&hw_constraints_period_sizes[cmode]);
	substream->runtime->hw = hw_caps;

	// since Dante is the clock master, the sample rate is fixed
	substream->runtime->hw.rate_min =
		atomic_read(&chip->current_sample_rate);
	substream->runtime->hw.rate_max =
		atomic_read(&chip->current_sample_rate);
	// overwrite clock mode dependant values
	substream->runtime->hw.channels_max = max_channels[cmode];

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		PRINT_DEBUG("pcm_playback_open\n");
		generic_clear_dma_buffer(&chip->playback_buf);
		spin_lock_irq(&chip->lock);
		snd_pcm_set_runtime_buffer(substream, &chip->playback_buf);
		chip->playback_substream = substream;
		spin_unlock_irq(&chip->lock);
	} else {
		PRINT_DEBUG("pcm_capture_open\n");
		generic_clear_dma_buffer(&chip->capture_buf);
		spin_lock_irq(&chip->lock);
		snd_pcm_set_runtime_buffer(substream, &chip->capture_buf);
		chip->capture_substream = substream;
		spin_unlock_irq(&chip->lock);
	}
	return 0;
}

static int pcm_close(struct snd_pcm_substream *substream)
{
	PRINT_DEBUG("pcm_close\n");
	snd_pcm_set_runtime_buffer(substream, NULL);
	return 0;
}

static int pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *hw_params)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);

	PRINT_DEBUG("pcm_hw_params\n");
	PRINT_DEBUG("  sample rate: %d\n",
		params_rate(hw_params));
	PRINT_DEBUG("  buffer bytes: %d\n",
		params_buffer_bytes(hw_params));
	PRINT_DEBUG("  buffer size : %d\n",
		params_buffer_size(hw_params));
	PRINT_DEBUG("  period bytes: %d\n",
		params_period_bytes(hw_params));
	PRINT_DEBUG("  period size : %d\n",
		params_period_size(hw_params));
	PRINT_DEBUG("  periods     : %d\n",
		params_periods(hw_params));
	PRINT_DEBUG("  channels    : %d\n",
		params_channels(hw_params));

	spin_lock_irq(&chip->lock);
	{	// this is certainly CLARA E specific
		// other cards could adapt if not synced externally
		unsigned int current_rate =
			atomic_read(&chip->current_sample_rate);
		if (params_rate(hw_params) != current_rate) {
			spin_unlock_irq(&chip->lock);
			PRINT_ERROR(
				"pcm_hw_params: sample rate mismatch. "
				"requested: %d, current: %d\n",
				substream->runtime->rate, current_rate);
			return -EINVAL;
		}
	}

	{
		// this is in number of samples per channel
		int num_frames = params_buffer_size(hw_params);
		if (chip->num_buffer_frames != 0 &&
			chip->num_buffer_frames != num_frames) {
			spin_unlock_irq(&chip->lock);
			PRINT_ERROR("pcm_hw_params: "
				"buffer size changed from %d to %d\n",
				chip->num_buffer_frames, num_frames);
			return -EBUSY;

		}
		chip->num_buffer_frames = num_frames;
	}
	spin_unlock_irq(&chip->lock);
	return 0;
}

static int pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);

	spin_lock_irq(&chip->lock);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		chip->playback_substream = NULL;
	else
		chip->capture_substream = NULL;
	if (chip->playback_substream == NULL &&
		chip->capture_substream == NULL) {
		dma_ng_stop(chip);
		dma_ng_disable_interrupts(chip);
		chip->num_buffer_frames = 0;
	}
	spin_unlock_irq(&chip->lock);
	return 0;
}

static int pcm_prepare(struct snd_pcm_substream *substream)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	u64 base_addr = substream->runtime->dma_addr;
	unsigned int channels = substream->runtime->channels;
	unsigned int no_blocks = 0;
	int err = 0;
	PRINT_DEBUG("pcm_prepare\n");

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		PRINT_DEBUG("pcm_prepare: playback base: %p\n",
			(void *)base_addr);
	}
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		PRINT_DEBUG("pcm_prepare: capture base: %p\n",
			(void *)base_addr);
	}

	spin_lock_irq(&chip->lock);
	no_blocks = substream->runtime->period_size /
		DMA_SAMPLES_PER_BLOCK * DMA_NUM_PERIODS;
	if (chip->num_buffer_frames != no_blocks * DMA_SAMPLES_PER_BLOCK) {
		spin_unlock_irq(&chip->lock);
		PRINT_ERROR("pcm_prepare: "
			"buffer size changed from %d to %d\n",
			chip->num_buffer_frames,
			no_blocks * DMA_SAMPLES_PER_BLOCK);
		return -EBUSY;
	}
	err = dma_ng_prepare(chip, channels,
		(substream->stream == SNDRV_PCM_STREAM_PLAYBACK),
		base_addr, no_blocks);
	spin_unlock_irq(&chip->lock);
	PRINT_DEBUG("pcm_prepare: no_blocks: %d\n", no_blocks);
	return err;
}

static int pcm_ioctl(struct snd_pcm_substream *substream,
	unsigned int cmd, void *arg)
{
	switch (cmd) {
	case SNDRV_PCM_IOCTL1_CHANNEL_INFO:
		return generic_dma_channel_offset(substream, arg,
			SNDRV_PCM_FMTBIT_S24_3LE);
	default:
		break;
	}

	return generic_pcm_ioctl(substream, cmd, arg);
}

static int pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		PRINT_DEBUG("pcm_trigger: start %s\n",
			substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
			"playback" : "capture");
		spin_lock_irq(&chip->lock);
		if (chip->dma_status != DMA_STATUS_RUNNING)
			dma_ng_start(snd_pcm_substream_chip(substream));
		spin_unlock_irq(&chip->lock);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			PRINT_DEBUG("pcm_trigger: stop capture\n");
			spin_lock_irq(&chip->lock);
			if (chip->playback_substream == NULL) {
				dma_ng_stop(chip);
			}
			spin_unlock_irq(&chip->lock);
			dma_ng_disable_channels(chip, false);
			generic_clear_dma_buffer(&chip->capture_buf);
		} else {
			PRINT_DEBUG("pcm_trigger: stop playback\n");
			generic_clear_dma_buffer(&chip->playback_buf);
			dma_ng_disable_channels(chip, true);
			spin_lock_irq(&chip->lock);
			if (chip->capture_substream == NULL) {
				dma_ng_stop(chip);
			}
			spin_unlock_irq(&chip->lock);
		}
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
	.open = pcm_open,
	.close = pcm_close,
	.ioctl = pcm_ioctl,
	.hw_params = pcm_hw_params,
	.hw_free = pcm_hw_free,
	.prepare = pcm_prepare,
	.trigger = pcm_trigger,
	.pointer = pcm_pointer,
};

static struct snd_pcm_ops const capture_ops = {
	.open = pcm_open,
	.close = pcm_close,
	.ioctl = pcm_ioctl,
	.hw_params = pcm_hw_params,
	.hw_free = pcm_hw_free,
	.prepare = pcm_prepare,
	.trigger = pcm_trigger,
	.pointer = pcm_pointer,
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

// One might wonder why this is not in the generic part. Well the Dante card
// is the only one to set the clock only from the Dante side (yet).
static enum clock_mode get_clock_mode(struct generic_chip *chip)
{
	u32 reg = read_reg32_bar0(chip, ADDR_CLOCK_MODE_READ_REG);
	enum clock_mode cmode = CLOCK_MODE_48;

	switch (reg & MASK_CLOCK_MODE) {
	case 0b11:
		cmode = CLOCK_MODE_48;
		break;
	case 0b10:
		cmode = CLOCK_MODE_96;
		break;
	case 0b01:
		cmode = CLOCK_MODE_192;
		break;
	default:
		PRINT_ERROR("get_clock_mode: invalid clock mode: %d\n", reg);
		break;
	}
	return cmode;
}

static void timer_callback(struct generic_chip *chip)
{
	clara_timer_callback(chip);
	// update the clock mode
	atomic_set(&chip->clock_mode, get_clock_mode(chip));
}
