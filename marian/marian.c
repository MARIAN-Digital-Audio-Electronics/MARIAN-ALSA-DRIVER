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

int driver_probe(struct pci_dev *dev, struct pci_device_id const *id)
{
	int err = 0;

	snd_printk(KERN_DEBUG "MARIAN driver probe: Device id: 0x%4x, revision: %02d\n", id->device, dev->revision);

	// cleanly initialize all specific function pointers
	clear_device_specifics(&dev_specifics[dev_idx]);

	switch (id->device) {
	case CLARA_E_DEVICE_ID:
		clara_e_register_device_specifics(&dev_specifics[dev_idx]);
		// TODO ToG: register ALSA card
		// TODO ToG: register ALSA PCM device
		break;

	default:
		err = -ENODEV;
		break;
	}

	if (!verify_device_specifics(&dev_specifics[dev_idx])) {
		snd_printk(KERN_ERR "MARIAN driver probe: device specific descriptor not fully defined\n");
		err = -EFAULT;
	}

	if (!dev_specifics[dev_idx].hw_revision_valid(dev->revision)) {
		struct valid_hw_revision_range range;
		dev_specifics[dev_idx].get_hw_revision_range(&range);
		snd_printk(KERN_ERR "MARIAN driver probe: device revision not supported\n");
		snd_printk(KERN_ERR "                     supported revisions: %02d - %02d\n", range.min, range.max);
		err = -ENODEV;
	}

	if (0 == err)
		dev_idx++;

	return err;
};

void driver_remove(struct pci_dev *dev)
{
	snd_printk(KERN_DEBUG "MARIAN driver remove: Device id: 0x%4x, revision: %02d\n", dev->device, dev->revision);
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