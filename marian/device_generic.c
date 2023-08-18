// TODO ToG: Add license header

#include <linux/pci.h>
#include <sound/core.h>
#include "device_generic.h"

static int generic_chip_dev_free(struct snd_device *device)
{
	struct generic_chip *chip = device->device_data;
	return generic_chip_free(chip);
}

int generic_chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip)
{
	int err = 0;
	struct generic_chip *chip = NULL;
	static const struct snd_device_ops ops = {
		.dev_free = generic_chip_dev_free,
	};

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->card = card;
	chip->pci_dev = pci_dev;
	chip->bar0_addr = 0;
	chip->bar0 = NULL;
	chip->irq = -1;
	chip->specific = NULL;
	chip->specific_free = NULL;

	err = generic_acquire_pci_resources(chip);
	if (err < 0)
		goto error;

	snd_device_new(card, SNDRV_DEV_LOWLEVEL, chip, &ops);

	*rchip = chip;
	snd_printk(KERN_DEBUG "generic_chip_new: success\n");
	return 0;

error:
	kfree(chip);
	return err;
};

int generic_chip_free(struct generic_chip *chip)
{
	int err	= 0;
	if (chip == NULL)
		return -EINVAL;
	if (chip->specific_free != NULL)
		err = chip->specific_free(chip->specific);
	if (err < 0)
		return err;
	err = generic_release_pci_resources(chip);
	if (err < 0)
		return err;
	kfree(chip);
	snd_printk(KERN_DEBUG "generic_chip_free\n");
	return 0;
}

int generic_acquire_pci_resources(struct generic_chip *chip)
{
	int err = 0;
	if (chip == NULL)
		return -EINVAL;

	err = pci_enable_device(chip->pci_dev);
	if (err < 0)
		return err;

	pci_set_master(chip->pci_dev);

	err = pci_request_regions(chip->pci_dev, chip->card->driver);
	if (err < 0)
		return err;

	chip->bar0_addr = pci_resource_start(chip->pci_dev, 0);
	chip->bar0 =
		ioremap(chip->bar0_addr, pci_resource_len(chip->pci_dev, 0));
	if (chip->bar0 == NULL) {
		snd_printk(KERN_ERR "ioremap error\n");
		return -ENXIO;
	}

	generic_indicate_status(chip, STATUS_SUCCESS);

	snd_printk(KERN_DEBUG "generic_acquire_pci_resources\n");
	return 0;
}

int generic_release_pci_resources(struct generic_chip *chip)
{
	if (chip == NULL)
		return -EINVAL;

	if (chip->bar0 != NULL) {
		generic_indicate_status(chip, STATUS_RESET);
		iounmap(chip->bar0);
		chip->bar0 = NULL;
	}

	if (chip->bar0_addr != 0) {
		pci_release_regions(chip->pci_dev);
		chip->bar0_addr = 0;
	}

	pci_disable_device(chip->pci_dev);

	snd_printk(KERN_DEBUG "generic_release_pci_resources\n");
	return 0;
}

void generic_indicate_status(struct generic_chip *chip,
	enum status_indicator status)
{
	if (chip == NULL)
		return;
	if (chip->bar0 == NULL)
		return;
	iowrite32(status, chip->bar0 + 0xF4);
}