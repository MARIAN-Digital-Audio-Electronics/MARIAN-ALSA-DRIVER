// TODO ToG: Add license header

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <sound/core.h>
#include <sound/control.h>
#include "device_generic.h"

#define ADDR_IRQ_STATUS_REG 0x00
#define ADDR_LED_REG 0xF4
#define ADDR_SAMPLE_COUNTER_REG 0x8C
#define ADDR_BUILD_NO_REG 0xFC
#define ADDR_WC_SCAN_SOURCE_REG 0xC8
#define ADDR_WC_SCAN_RESULT_REG 0x94
#define MASK_WC_SCAN_READY 0x80000000
#define MASK_WC_SCAN_RESULT 0x3FFFF
#define WC_ACCURACY_HZ 100

char *clock_mode_names[] = {
	"CLOCK_MODE_48",
	"CLOCK_MODE_96",
	"CLOCK_MODE_192",
};

static int acquire_pci_resources(struct generic_chip *chip);
static void release_pci_resources(struct generic_chip *chip);

/*
	CHIP MANAGEMENT FUNCTIONS
*/

int generic_chip_dev_free(struct snd_device *device)
{
	struct generic_chip *chip = device->device_data;
	snd_device_free(chip->card, device);
	generic_chip_free(chip);
	device->device_data = NULL;
	return 0;
}

int generic_chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip)
{
	int err = 0;
	struct generic_chip *chip = NULL;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->card = card;
	chip->pci_dev = pci_dev;
	chip->bar0_addr = 0;
	chip->bar0 = NULL;
	chip->irq = -1;
	chip->pcm = NULL;
	chip->playback_substream = NULL;
	chip->capture_substream = NULL;
	chip->dma_status = DMA_STATUS_UNKNOWN;
	memset(&chip->playback_buf, 0, sizeof(chip->playback_buf));
	memset(&chip->capture_buf, 0, sizeof(chip->capture_buf));
	chip->num_buffer_frames = 0;
	chip->timer_thread = NULL;
	chip->timer_callback = NULL;
	chip->timer_interval_ms = 0;
	chip->specific = NULL;
	chip->specific_free = NULL;
	atomic_set(&chip->current_sample_rate, 0);
	atomic_set(&chip->clock_mode, CLOCK_MODE_48);

	err = acquire_pci_resources(chip);
	if (err < 0)
		goto error;

	*rchip = chip;
	snd_printk(KERN_DEBUG "generic_chip_new: success\n");
	return 0;

error:
	// we might have partially acquired PCI resources
	release_pci_resources(chip);
	kfree(chip);
	return err;
}

void generic_chip_free(struct generic_chip *chip)
{
	if (chip == NULL)
		return;
	if (chip->specific_free != NULL)
		chip->specific_free(chip);
	if (chip->irq) {
		free_irq(chip->irq, chip);
		chip->irq = -1;
		snd_printk(KERN_DEBUG "free_irq\n");
	}
	snd_dma_free_pages(&chip->playback_buf);
	snd_dma_free_pages(&chip->capture_buf);
	release_pci_resources(chip);
	kfree(chip);
	snd_printk(KERN_DEBUG "chip_free\n");
}

static int acquire_pci_resources(struct generic_chip *chip)
{
	int err = 0;
	if (chip == NULL)
		return -EINVAL;

	err = pci_enable_device(chip->pci_dev);
	if (err < 0)
		return err;

	if (!dma_set_mask(&chip->pci_dev->dev, DMA_BIT_MASK(64))) {
		dma_set_coherent_mask(&chip->pci_dev->dev, DMA_BIT_MASK(64));
	}
	else if (!dma_set_mask(&chip->pci_dev->dev, DMA_BIT_MASK(32))) {
		dma_set_coherent_mask(&chip->pci_dev->dev, DMA_BIT_MASK(32));
	}
	else {
		snd_printk(KERN_ERR "No suitable DMA possible.\n");
		return -EINVAL;
	}
	pci_set_master(chip->pci_dev);
	pci_disable_msi(chip->pci_dev);

	err = pci_request_regions(chip->pci_dev, chip->card->driver);
	if (err < 0)
		return err;

	chip->bar0_addr = pci_resource_start(chip->pci_dev, 0);
	chip->bar0 =
		ioremap(chip->bar0_addr, pci_resource_len(chip->pci_dev, 0));
	if (chip->bar0 == NULL) {
		snd_printk(KERN_ERR "BAR0: ioremap error\n");
		return -ENXIO;
	}

	generic_indicate_state(chip, STATE_SUCCESS);

	snd_printk(KERN_DEBUG "acquire_pci_resources\n");
	return 0;
}

static void release_pci_resources(struct generic_chip *chip)
{
	if (chip == NULL)
		return;

	pci_clear_master(chip->pci_dev);

	if (chip->bar0 != NULL) {
		generic_indicate_state(chip, STATE_RESET);
		iounmap(chip->bar0);
		chip->bar0 = NULL;
	}

	if (chip->bar0_addr != 0) {
		pci_release_regions(chip->pci_dev);
		chip->bar0_addr = 0;
	}

	pci_disable_device(chip->pci_dev);

	snd_printk(KERN_DEBUG "release_pci_resources\n");
}

/*
	HARDWARE SPECIFIC FUNCTIONS
*/

int generic_dma_channel_offset(struct snd_pcm_substream *substream,
	struct snd_pcm_channel_info *info, unsigned long alignment)
{
	struct generic_chip *chip = snd_pcm_substream_chip(substream);
	unsigned int channel = info->channel;
	info->offset = 0;
	info->step = 32;
	switch (alignment) {
	case SNDRV_PCM_FMTBIT_S24_3LE:
		info->first = channel * chip->num_buffer_frames *
			sizeof(u32) * 8 + 8;
		break;
	case SNDRV_PCM_FMTBIT_S32_LE:
		info->first = channel * chip->num_buffer_frames *
			sizeof(u32) * 8;
		break;
	default:
		return -EINVAL;
	}
	snd_printk(KERN_DEBUG "generic_dma_channel_offset: channel: %d, "
		"offset: %d\n", channel, info->first/8);
	return 0;
}

int generic_pcm_ioctl(struct snd_pcm_substream *substream, unsigned int cmd,
	void *arg)
{
	// nothing specific here yet
	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

void generic_indicate_state(struct generic_chip *chip,
	enum state_indicator state)
{
	write_reg32_bar0(chip, ADDR_LED_REG, state);
}

u32 generic_get_sample_counter(struct generic_chip *chip)
{
	return read_reg32_bar0(chip, ADDR_SAMPLE_COUNTER_REG);
}

u32 generic_get_irq_status(struct generic_chip *chip)
{
	return read_reg32_bar0(chip, ADDR_IRQ_STATUS_REG);
}

u32 generic_get_build_no(struct generic_chip *chip)
{
	return read_reg32_bar0(chip, ADDR_BUILD_NO_REG);
}

enum clock_mode generic_sample_rate_to_clock_mode(unsigned int sample_rate)
{
	if (sample_rate <= 50000)
		return CLOCK_MODE_48;
	else if (sample_rate <= 100000)
		return CLOCK_MODE_96;
	else
		return CLOCK_MODE_192;
}

static unsigned int standard_wordclocks_hz[] = {22050, 32000, 44100, 48000,
	64000, 88200, 96000, 128000, 176400, 192000, 256000, 352800, 384000,};

static unsigned int snap_to_standard_wc_hz(unsigned int freq_hz)
{
	unsigned int i;
	unsigned int standard_freq_hz;
	// setup a tolerance of 1%
	unsigned int max_freq_hz = freq_hz * 101;
	unsigned int min_freq_hz = freq_hz * 99;

	for (i = 0; i < ARRAY_SIZE(standard_wordclocks_hz); i++) {
		standard_freq_hz = standard_wordclocks_hz[i] * 100;
		if (standard_freq_hz >= min_freq_hz &&
			standard_freq_hz <= max_freq_hz) {
			return standard_wordclocks_hz[i];
		}
	}
	return freq_hz;
}

unsigned int generic_measure_wordclock_hz(struct generic_chip *chip, unsigned int source) {
	u32 reg_val;
	unsigned int freq_hz;
	int retries = 3;
	write_reg32_bar0(chip, ADDR_WC_SCAN_SOURCE_REG, source & 0x7);
	while (retries > 0) {
		reg_val = read_reg32_bar0(chip, ADDR_WC_SCAN_RESULT_REG);
		if (reg_val & MASK_WC_SCAN_READY)
			break;
		msleep(3);
		retries--;
	}
	if (retries > 0) {
		 freq_hz = (1280000000 / ((reg_val & MASK_WC_SCAN_RESULT)+1));
		 return snap_to_standard_wc_hz(freq_hz);
	}
	return 0;
}

/*
	GENERIC CONTROLS
*/

static int wordclock_frequency_info(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 22050;
	uinfo->value.integer.max = 192000;
	uinfo->value.integer.step = 1;
	return 0;
}

static int wordclock_frequency_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct generic_chip *chip = snd_kcontrol_chip(kcontrol);

	switch ((unsigned int)(kcontrol->private_value)) {
	case 0:
		ucontrol->value.integer.value[0] =
			atomic_read(&chip->current_sample_rate);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

int generic_read_wordclock_control_create(struct generic_chip *chip, char *label,
	unsigned int idx) {
	struct snd_kcontrol_new c = {
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = label,
		.private_value = idx,
		.access = SNDRV_CTL_ELEM_ACCESS_READ | SNDRV_CTL_ELEM_ACCESS_VOLATILE,
		.info = wordclock_frequency_info,
		.get = wordclock_frequency_get
	};
	return snd_ctl_add(chip->card, snd_ctl_new1(&c, chip));
}
