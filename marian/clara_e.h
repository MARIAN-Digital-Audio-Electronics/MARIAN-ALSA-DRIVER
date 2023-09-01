// TODO ToG: Add license header

#ifndef MARIAN_CLARA_E_H
#define MARIAN_CLARA_E_H

#define CLARA_E_DEVICE_ID 0x9050

// revisions this driver is able to handle
#define CLARA_E_MIN_HW_REVISION 0x05
#define CLARA_E_MAX_HW_REVISION 0x05

#define CLARA_E_CARD_NAME "ClaraE"

void clara_e_register_device_specifics(struct device_specifics *dev_specifics);

#endif