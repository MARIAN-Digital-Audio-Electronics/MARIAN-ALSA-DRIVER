// TODO ToG: Add license header

#ifndef MARIAN_GENERIC_H
#define MARIAN_GENERIC_H

#include <linux/pci.h>
#include <sound/core.h>

enum status_indicator {
	STATUS_SUCCESS = 1,
	STATUS_FAILURE = 2,
	STATUS_RESET = 3,
};

typedef int (*specific_chip_free_func)(void *specific_chip);

struct generic_chip {
	struct snd_card *card;
	struct pci_dev *pci_dev;
	int irq;
	unsigned long bar0_addr;
	void __iomem *bar0;
	void *specific;
	specific_chip_free_func specific_free;
};

int generic_chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip);
int generic_chip_free(struct generic_chip *chip);

int generic_acquire_pci_resources(struct generic_chip *chip);
int generic_release_pci_resources(struct generic_chip *chip);
void generic_indicate_status(struct generic_chip *chip,
	enum status_indicator status);

#endif
