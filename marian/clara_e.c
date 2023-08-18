// TODO ToG: Add license header

#include <linux/types.h>
#include "device_abstraction.h"
#include "clara_e.h"

static bool clara_e_hw_revision_valid(u8 rev)
{
	if (rev < CLARA_E_MIN_HW_REVISION)
		return false;
	if (rev > CLARA_E_MAX_HW_REVISION)
		return false;
	return true;
}

static void clara_e_get_hw_revision_range(struct valid_hw_revision_range *range)
{
	if (range == NULL)
		return;
	range->min = CLARA_E_MIN_HW_REVISION;
	range->max = CLARA_E_MAX_HW_REVISION;
}


void clara_e_register_device_specifics(struct device_specifics *dev_specifics)
{
	if (dev_specifics == NULL)
		return;
	dev_specifics->hw_revision_valid = clara_e_hw_revision_valid;
	dev_specifics->get_hw_revision_range = clara_e_get_hw_revision_range;
}
