# MARIAN - Digital Audio Electronics PCIe ALSA driver

## What's this all about?
This driver is intended to enable MARIAN PCIe audio cards to work with Linux. It is structured as a framework to support multiple cards and multiple series.
As a first implementation the Clara E (MARIAN's Dante card) is provided. Within the framework it should be pretty easy to also add other Clara cards and even cards from the Seraph series.

## OS support
The driver has been developed and tested on following systems:
* Ubuntu 20.04, Kernel 5.15 (x64)
* Ubuntu 22.04, Kernel 6.8 (x64)
* Ubuntu 24.04, Kernel 6.14 (x64)
* Debian 11, Kernel 5.10 RT (x64)
* Debian 12, Kernel 6.1 RT (x64)
* Debian 13, Kernel 6.12 (x64)
* Raspberry Pi OS (bookworm), Kernel 6.12 (ARM64) on a Raspberry Pi 5

Since no funny magic is used it should easily work on other systems that support ALSA.

## Supported Devices
* MARIAN Clara E, Rev.09-0B
* MARIAN Clara Emin, Rev.04-05

Always make sure to have the corresponding firmware installed on the card by checking the PCI revision or looking it up in the Dante Controller.

## History
* Version 1.3.0:
    - Implemented MSI support
    - Removed calls to deprecated snd_printk()
    - Update script supports pulse and pipewire
* Version 1.2.0:
    - Moved from 24Bit LSB aligned to 32Bit transfer (Clara E, Clara Emin)
* Version 1.1.1:
    - Fixed locking issue
    - Preload skipping is enabled by default
* Version 1.1.0: Added support for Clara Emin
* Version 1.0.0: Initial release for Clara E

## Building the driver
This is no detailed instruction on to how make your system fit for building kernel drivers but to give a pointer in the right direction, on Debian based systems you might want to start with something like:
```bash
sudo apt-get install -y build-essential linux-headers-$(uname -r) libasound2-dev
```
As soon as all prerequisites to compile a Linux module are met, simply run **make** from the root directory. You can also use the **update.sh** script to automatically remove and insert the module after compilation.

## Permanently installing the module
After successfully building the driver automatically enable it at every boot with the following commands:
```bash
sudo make install
sudo depmod
echo "snd_marian" | sudo tee -a /etc/modules
```

## Support
Please note that the author does not provide free support in case you encounter any issues with the driver on your specific system.
**Should you need any help please contact support@marian.de.**
Compile the driver with debug outputs by changing DBG_LEVEL to ```DBG_LEVEL := $(DBG_LVL_DEBUG)``` in the top level Makefile and provide at least the following information (after inserting the module!):
```bash
uname -a > system.log
sudo dmesg > kernel.log
sudo lspci -vv > pci.log
ps -aux > process.log
```

## Clara E / Emin specifics
The Clara E does not support changing the sample rate from the PCIe side. The sample rate has to be set from the Dante Controller Software prior to opening the audio device. The driver will report the currently set Dante sample rate as the only supported rate when opening the PCM devices.

There is an ALSA control named "Sample Rate" associated with the card interface for reference in user space. The driver also sends notifications to this control in case the sample rate is changed via the Dante Controller.

To get a glance at the ALSA controls without writing any custom software simply try:
```bash
amixer -c ClaraE contents
```
To test the notification functionality use:
```bash
amixer -c ClaraE events
```
