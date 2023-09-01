// TODO ToG: Add license header


#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/memalloc.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include "marian.h"

MODULE_AUTHOR("Tobias Groß <theguy@audio-fpga.com>");
MODULE_DESCRIPTION("ALSA driver for MARIAN PCIe soundcards");
MODULE_LICENSE("GPL");

__maybe_unused static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;
__maybe_unused static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;
__maybe_unused static bool enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;

static unsigned dev_idx = 0;

static struct device_specifics dev_specifics[SNDRV_CARDS] = { 0, };

MODULE_PARM_DESC(index, "Index value for MARIAN soundcard.");
MODULE_PARM_DESC(id, "ID string for MARIAN soundcard.");
MODULE_PARM_DESC(enable, "Enable MARIAN soundcard.");

module_param_array(index, int, NULL, 0444);
module_param_array(id, charp, NULL, 0444);
module_param_array(enable, bool, NULL, 0444);

static int driver_probe(struct pci_dev *pci_dev,
	struct pci_device_id const *pci_id)
{
	struct snd_card *card = NULL;
	struct generic_chip *chip = NULL;
	static const struct snd_device_ops ops = {
		.dev_free = generic_chip_dev_free,
	};
	int err = 0;

	if (dev_idx >= SNDRV_CARDS) {
		return -ENODEV;
	}

	if (!enable[dev_idx]) {
		dev_idx++;
		return -ENOENT;
	}

	snd_printk(KERN_INFO "MARIAN driver probe: Device id: 0x%4x, "
		"revision: %02d\n", pci_id->device, pci_dev->revision);

	// cleanly initialize all specific function and descriptor pointers
	clear_device_specifics(&dev_specifics[dev_idx]);

	switch (pci_id->device) {
	case CLARA_E_DEVICE_ID:
		clara_e_register_device_specifics(&dev_specifics[dev_idx]);
		break;

	default:
		return -ENODEV;
	}

	if (!verify_device_specifics(&dev_specifics[dev_idx])) {
		snd_printk(KERN_ERR "MARIAN driver probe: device specific "
			"descriptor not fully defined\n");
		return -EFAULT;
	}

	if (!dev_specifics[dev_idx].hw_revision_valid(pci_dev->revision)) {
		struct valid_hw_revision_range range;
		dev_specifics[dev_idx].get_hw_revision_range(&range);
		snd_printk(KERN_ERR
			"MARIAN driver probe: device revision not supported.\n\t"
			"supported revisions: %02d - %02d\n",
			range.min, range.max);
		return -ENODEV;
	}

	err = snd_card_new(&pci_dev->dev, index[dev_idx], id[dev_idx],
		THIS_MODULE, 0, &card);
	if (err < 0)
		return err;
	// driver_remove() needs access to the card to free resources
	pci_set_drvdata(pci_dev, card);
	snd_card_set_dev(card, &pci_dev->dev);
	card->private_data = chip;

	strcpy(card->driver, MARIAN_DRIVER_NAME);
	strcpy(card->shortname, dev_specifics[dev_idx].card_name);
	sprintf(card->longname, "%s %s", card->driver, card->shortname);

	// create the chip instance
	err = dev_specifics[dev_idx].chip_new(card, pci_dev, &chip);
	if (err < 0)
		goto error_free_chip;

	if (!dev_specifics[dev_idx].detect_hw_presence(chip)) {
		snd_printk(KERN_ERR "MARIAN driver probe: device not present\n");
		err = -ENODEV;
		goto error_free_chip;
	}

	// prevent something funny happens when the irq handler is attached
	dev_specifics[dev_idx].soft_reset(chip);

	if (request_irq
		(chip->pci_dev->irq, dev_specifics[dev_idx].irq_handler,
		IRQF_SHARED, "MARIAN_PCIe", chip) < 0) {
		snd_printk(KERN_ERR "request_irq error: %d\n", chip->pci_dev->irq);
		err = -ENXIO;
		goto error_free_chip;
	}
	chip->irq = chip->pci_dev->irq;
	card->sync_irq = chip->irq;
	snd_printk(KERN_DEBUG "MARIAN driver probe: IRQ: %d\n", chip->irq);

	// create a sound device
	err = snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops);
	if (err < 0)
		goto error_free_card;

	{ // create a PCM device
		// TODO ToG: this has to go somewhere device specific
		// to make the number of substreams variable according
		// to the DMA implementation
		struct snd_pcm *pcm;
		struct snd_dma_buffer tmp_buf;
		
		if (snd_dma_alloc_dir_pages(SNDRV_DMA_TYPE_DEV, &pci_dev->dev,
				DMA_BIDIRECTIONAL, 16 * 4 * 512 * 128,
				&tmp_buf) == 0) {
				snd_printk(KERN_DEBUG "area = 0x%p\n", tmp_buf.area);
				snd_printk(KERN_DEBUG "addr = 0x%p\n", tmp_buf.addr);
				snd_printk(KERN_DEBUG "bytes = %d\n", tmp_buf.bytes);
				chip->capture_buf = tmp_buf;
		}
		else {
			snd_printk(KERN_ERR
				"snd_dma_alloc_dir_pages failed (capture)\n");
			err = -ENOMEM;
			goto error_free_card;
		}
		if (snd_dma_alloc_dir_pages(SNDRV_DMA_TYPE_DEV, &pci_dev->dev,
			DMA_BIDIRECTIONAL, 16 * 4 * 512 * 128, &tmp_buf) == 0) {
			snd_printk(KERN_DEBUG "area = 0x%p\n", tmp_buf.area);
			snd_printk(KERN_DEBUG "addr = 0x%p\n", tmp_buf.addr);
			snd_printk(KERN_DEBUG "bytes = %d\n", tmp_buf.bytes);
			chip->playback_buf = tmp_buf;
		}
		else {
			snd_printk(KERN_ERR
				"snd_dma_alloc_dir_pages failed (playback)\n");
			err = -ENOMEM;
			goto error_free_card;
		}
		
		err = snd_pcm_new(card, card->shortname, 0, 1, 1, &pcm);
		if (err < 0)
			goto error_free_card;
		pcm->private_data = chip;
		chip->pcm = pcm;
		sprintf(pcm->name, "%s PCM", card->shortname);
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			dev_specifics[dev_idx].pcm_playback_ops);
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE,
			dev_specifics[dev_idx].pcm_capture_ops);
	}

	// register as ALSA device
	err = snd_card_register(card);
	if (err < 0)
		goto error_free_card;

	snd_printk(KERN_INFO "MARIAN driver probe: Initialized module for %s\n",
		dev_specifics[dev_idx].card_name);
	dev_idx++;
	return 0;

// up until snd_device_new() is called we need to explicitely clean up the
// and all its resources
// after calling snd_device_new() generic_chip_dev_free() is called implicitly
// which in turn calls generic_chip_free()
error_free_chip:
	generic_chip_free(chip);
error_free_card:
	snd_card_free(card);
	return err;
};

static void driver_remove(struct pci_dev *pci)
{
	snd_printk(KERN_INFO
		"MARIAN driver remove: Device id: 0x%4x, revision: %02d\n",
		pci->device, pci->revision);
	snd_card_free(pci_get_drvdata(pci));
	pci_set_drvdata(pci, NULL);
};

static struct pci_device_id pci_ids[] = {
	{ PCI_DEVICE(MARIAN_VENDOR_ID, CLARA_E_DEVICE_ID) },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, pci_ids);

static struct pci_driver pci_driver = {
	.name = "MARIAN PCIe driver",
	.id_table = pci_ids,
	.probe = driver_probe,
	.remove = driver_remove,
};

module_pci_driver(pci_driver);
