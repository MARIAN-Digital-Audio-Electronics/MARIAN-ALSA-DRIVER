// TODO ToG: Add license header

#ifndef DEVICE_ABSTRACTION_H
#define DEVICE_ABSTRACTION_H

#include <linux/types.h>

typedef bool (*hw_revision_valid_func)(u8 rev);

struct valid_hw_revision_range {
	u8 min;
	u8 max;
};

typedef void (*get_hw_revision_range_func)(struct valid_hw_revision_range *range);

/* This structure holds the device specific functions
	and descriptors that can only be determined at runtime.
	It may only contain pointers so it's possible to determine
	the correct number of elements at runtime.
	CAUTION: If editing this structure also be sure to
	edit the corresponding verify and clear functions accordingly! */
struct device_specifics {
	hw_revision_valid_func hw_revision_valid;
	get_hw_revision_range_func get_hw_revision_range;
};

void clear_device_specifics(struct device_specifics *dev_specifics);
bool verify_device_specifics(struct device_specifics *dev_specifics);

#endif