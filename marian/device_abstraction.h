// TODO ToG: Add license header

#ifndef MARIAN_DEVICE_ABSTRACTION_H
#define MARIAN_DEVICE_ABSTRACTION_H

#include <linux/types.h>
#include <linux/pci.h>
#include <sound/core.h>
#include "device_generic.h"


typedef bool (*hw_revision_valid_func)(u8 rev);

struct valid_hw_revision_range {
	u8 min;
	u8 max;
};

typedef void (*get_hw_revision_range_func)(struct valid_hw_revision_range *range);
typedef int (*chip_new_func)(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip);
typedef bool (*detect_hw_presence_func)(struct generic_chip *chip);
typedef void (*soft_reset_func)(struct generic_chip *chip);
typedef irqreturn_t (*irq_handler_func)(int irq, void *dev_id);

/* This structure holds the device specific functions
	and descriptors that can only be determined at runtime.
	It may only contain pointers so it's possible to determine
	the correct number of elements at runtime.
	CAUTION: If editing this structure also be sure to
	edit the corresponding verify and clear functions accordingly! */
struct device_specifics {
	hw_revision_valid_func hw_revision_valid;
	get_hw_revision_range_func get_hw_revision_range;
	char *card_name;
	chip_new_func chip_new;
	// always use the generic chip_free function to make sure
	// all resources are freed correctly unless you REALLY
	// know what you're doing!
	chip_free_func chip_free;
	detect_hw_presence_func detect_hw_presence;
	soft_reset_func soft_reset;
	irq_handler_func irq_handler;
};

void clear_device_specifics(struct device_specifics *dev_specifics);
bool verify_device_specifics(struct device_specifics *dev_specifics);

#endif