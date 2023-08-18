// TODO ToG: add license header

#include <linux/types.h>
#include "device_abstraction.h"

void clear_device_specifics(struct device_specifics *dev_specifics)
{
	if (dev_specifics == NULL)
		return;
	dev_specifics->hw_revision_valid = NULL;
	dev_specifics->get_hw_revision_range = NULL;
}

bool verify_device_specifics(struct device_specifics *dev_specifics)
{
	if (dev_specifics == NULL)
		return false;
	if (dev_specifics->hw_revision_valid == NULL)
		return false;
	if (dev_specifics->get_hw_revision_range == NULL)
		return false;
	return true;
}