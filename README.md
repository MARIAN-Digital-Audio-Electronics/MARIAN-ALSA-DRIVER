# MARIAN - Digital Audio Electronics PCIe ALSA driver

## What's this all about?
This driver is intended to enable MARIAN PCIe audio cards to work with Linux.
It is structured as a framework to support multiple cards and multiple series.
As a first implementation the Clara E (MARIAN's Dante card) is provided. Within
the framework it should be pretty easy to also add other Clara cards and even
cards from the Seraph series.

## OS support
The driver has been developed and tested on Ubuntu systems with Kernel versions
5.x and 6.x. Since no funny magic is used it should easily work on other systems
that support ALSA.

## Building the driver
As soon as all prerequisites to compile a Linux module are met simply run
**make** from the root directory. You can also use the **update.sh** script to
automatically remove and insert the module after compilation.

## Clara E specifics
As of 09/2023 the Clara E does not (yet) support changing the sample rate from
the PCIe side. The sample rate has to be set from the Dante Controller Software
prior to opening the audio device. The driver will report the currently set
Dante sample rate as the only supported rate when opening the PCM devices.

There is an ALSA control named "Sample Rate" associated with the card interface
for reference in user space. The driver also sends notifications to this control
in case the sample rate is changed via the Dante Controller.

To get a glance at the ALSA controls without writing any custom software simply
try:
```shell
amixer -c ClaraE contents
```
To test the notification functionality use:
```shell
amixer -c ClaraE events
```
