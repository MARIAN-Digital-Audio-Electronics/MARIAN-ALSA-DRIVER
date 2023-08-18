// TODO ToG: Add license header


#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/pcm.h>
#include "marian.h"

MODULE_AUTHOR("Tobias Groß <theguy@audio-fpga.com>");
MODULE_DESCRIPTION("ALSA driver for MARIAN PCIe soundcards");
MODULE_LICENSE("Proprietary");

__maybe_unused static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;
__maybe_unused static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;
__maybe_unused static bool enable[SNDRV_CARDS] = SNDRV_DEFAULT_ENABLE_PNP;

static unsigned dev_idx = 0;

static struct device_specifics dev_specifics[SNDRV_CARDS] = { 0, };

static int driver_probe(struct pci_dev *pci_dev, struct pci_device_id const *pci_id)
{
	struct snd_card *card = NULL;
	struct generic_chip *chip = NULL;
	int err = 0;

	if (dev_idx >= SNDRV_CARDS) {
		return -ENODEV;
	}

	if (!enable[dev_idx]) {
		dev_idx++;
		return -ENOENT;
	}

	snd_printk(KERN_DEBUG "MARIAN driver probe: Device id: 0x%4x, revision: %02d\n",
		pci_id->device, pci_dev->revision);

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
		snd_printk(KERN_ERR "MARIAN driver probe: device specific descriptor not fully defined\n");
		return -EFAULT;
	}

	if (!dev_specifics[dev_idx].hw_revision_valid(pci_dev->revision)) {
		struct valid_hw_revision_range range;
		dev_specifics[dev_idx].get_hw_revision_range(&range);
		snd_printk(KERN_ERR "MARIAN driver probe: device revision not supported\n");
		snd_printk(KERN_ERR "                     supported revisions: %02d - %02d\n", range.min, range.max);
		return -ENODEV;
	}

	snd_printk(KERN_INFO "MARIAN driver probe: Initialized module for %s\n",
		dev_specifics[dev_idx].card_name);


	err = snd_card_new(&pci_dev->dev, index[dev_idx], id[dev_idx],
		THIS_MODULE, 0, &card);
	if (err < 0)
		return err;

	/* Setup General datastructures */
	strcpy(card->driver, MARIAN_DRIVER_NAME);
	strcpy(card->shortname, dev_specifics[dev_idx].card_name);
	sprintf(card->longname, "%s %s", card->driver, card->shortname);

	// create the chip instance
	err = dev_specifics[dev_idx].chip_new(card, pci_dev, &chip);
	if (err < 0)
		goto error;

	// register as ALSA device
	err = snd_card_register(card);
	if (err < 0)
		goto error;

	// driver_remove() needs access to the card to free resources
	pci_set_drvdata(pci_dev, card);
	snd_card_set_dev(card, &pci_dev->dev);

	dev_idx++;
	return 0;

error:
	snd_card_free(card);
	return err;
};

static void driver_remove(struct pci_dev *pci)
{
	snd_printk(KERN_DEBUG "MARIAN driver remove: Device id: 0x%4x, revision: %02d\n", pci->device, pci->revision);
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

static int __init mod_init(void)
{
	return pci_register_driver(&pci_driver);
}

static void __exit mod_exit(void)
{
	pci_unregister_driver(&pci_driver);
}

module_init(mod_init);
module_exit(mod_exit);