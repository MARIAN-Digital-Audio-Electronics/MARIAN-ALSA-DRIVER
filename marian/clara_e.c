// TODO ToG: Add license header

#include <linux/types.h>
#include "device_abstraction.h"
#include "device_generic.h"
#include "clara.h"
#include "clara_e.h"

struct clara_e_chip {
	u32 _dummy;
};

static bool hw_revision_valid(u8 rev)
{
	if (rev < CLARA_E_MIN_HW_REVISION)
		return false;
	if (rev > CLARA_E_MAX_HW_REVISION)
		return false;
	return true;
}

static void get_hw_revision_range(struct valid_hw_revision_range *range)
{
	if (range == NULL)
		return;
	range->min = CLARA_E_MIN_HW_REVISION;
	range->max = CLARA_E_MAX_HW_REVISION;
}

static void chip_free(struct generic_chip *chip)
{
	struct clara_chip *clara_chip = chip->specific;
	struct clara_e_chip *clara_e_chip = clara_chip->specific;

	if (clara_chip == NULL)
		return;
	if (clara_e_chip == NULL)
		return;
	kfree(clara_e_chip);
};

static int chip_new(struct snd_card *card,
	struct pci_dev *pci_dev,
	struct generic_chip **rchip)
{
	int err = 0;
	struct generic_chip *chip = NULL;
	struct clara_chip *clara_chip = NULL;
	struct clara_e_chip *clara_e_chip = NULL;

	err = clara_chip_new(card, pci_dev, &chip);
	if (err < 0)
		return err;

	clara_e_chip = kzalloc(sizeof(*clara_e_chip), GFP_KERNEL);
	if (clara_e_chip == NULL)
		return -ENOMEM;
	clara_e_chip->_dummy = 0x5CA1AB1E;

	clara_chip = chip->specific;
	clara_chip->specific = clara_e_chip;
	clara_chip->specific_free = chip_free;

	*rchip = chip;
	return 0;

	/* If some more resources are acquired and something can go wrong
	 * do not forget to clean up! */
	// error:
	// kfree(clara_e_chip);
	// return err;
};

void clara_e_register_device_specifics(struct device_specifics *dev_specifics)
{
	if (dev_specifics == NULL)
		return;
	dev_specifics->hw_revision_valid = hw_revision_valid;
	dev_specifics->get_hw_revision_range = get_hw_revision_range;
	dev_specifics->card_name = CLARA_E_CARD_NAME;
	dev_specifics->chip_new = chip_new;
	dev_specifics->chip_free = generic_chip_free;
	dev_specifics->detect_hw_presence = clara_detect_hw_presence;
}
