// TODO ToG: Add license header

#include <linux/pci.h>
#include <sound/core.h>
#include "device_generic.h"

struct clara_chip {
	unsigned long bar1_addr;
	void __iomem *bar1;
	void *specific;
	chip_free_func specific_free;
};

#define write_reg32_bar1(chip, reg, val) \
	iowrite32((val), ((struct clara_chip *)(chip->specific))->bar1 + (reg))
#define read_reg32_bar1(chip, reg) \
	ioread32(((struct clara_chip *)(chip->specific))->bar1 + (reg))

int clara_chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip);
bool clara_detect_hw_presence(struct generic_chip *chip);
/* Resets all relevant registers to default values mainly
 * to avoid spurious interrupts. */
void clara_soft_reset(struct generic_chip *chip);
