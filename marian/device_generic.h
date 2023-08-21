// TODO ToG: Add license header

#ifndef MARIAN_GENERIC_H
#define MARIAN_GENERIC_H

#include <linux/pci.h>
#include <sound/core.h>

#define write_reg32_bar0(chip, reg, val) \
	iowrite32((val), (chip)->bar0 + (reg))
#define read_reg32_bar0(chip, reg) \
	ioread32((chip)->bar0 + (reg))

struct generic_chip;

// ALSA specific free operation
int generic_chip_dev_free(struct snd_device *device);
typedef void (*chip_free_func)(struct generic_chip *chip);
// this needs to be public in case we need to rollback in driver_probe()
void generic_chip_free(struct generic_chip *chip);

enum status_indicator {
	STATUS_SUCCESS = 1,
	STATUS_FAILURE = 2,
	STATUS_RESET = 3,
};

struct generic_chip {
	struct snd_card *card;
	struct pci_dev *pci_dev;
	int irq;
	unsigned long bar0_addr;
	void __iomem *bar0;
	void *specific;
	chip_free_func specific_free;
};

int generic_chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip);

void generic_indicate_state(struct generic_chip *chip,
	enum status_indicator state);

#endif
