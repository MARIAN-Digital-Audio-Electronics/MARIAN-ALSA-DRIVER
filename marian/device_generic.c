// TODO ToG: Add license header

#include <linux/pci.h>
#include <sound/core.h>
#include "device_generic.h"

#define ADDR_STATUS_REG 0x00
#define ADDR_LED_REG 0xF4

static int acquire_pci_resources(struct generic_chip *chip);
static void release_pci_resources(struct generic_chip *chip);

int generic_chip_dev_free(struct snd_device *device)
{
	struct generic_chip *chip = device->device_data;
	generic_chip_free(chip);
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
	chip->specific = NULL;
	chip->specific_free = NULL;

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
};

void generic_chip_free(struct generic_chip *chip)
{
	if (chip == NULL)
		return;
	if (chip->specific_free != NULL) {
		chip->specific_free(chip);
	}
	if (chip->irq) {
		free_irq(chip->irq, chip);
		chip->irq = -1;
		snd_printk(KERN_DEBUG "free_irq\n");
	}
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

	pci_set_master(chip->pci_dev);

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

	generic_indicate_state(chip, STATUS_SUCCESS);

	snd_printk(KERN_DEBUG "acquire_pci_resources\n");
	return 0;
}

static void release_pci_resources(struct generic_chip *chip)
{
	if (chip == NULL)
		return;

	if (chip->bar0 != NULL) {
		generic_indicate_state(chip, STATUS_RESET);
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

void generic_indicate_state(struct generic_chip *chip,
	enum status_indicator state)
{
	write_reg32_bar0(chip, ADDR_LED_REG, state);
}

irqreturn_t generic_irq_handler(int irq, void *dev_id)
{
	struct generic_chip *chip = dev_id;
	u32 val = read_reg32_bar0(chip, ADDR_STATUS_REG);
	if (val == 0)
		return IRQ_NONE;

	return IRQ_HANDLED;
}
