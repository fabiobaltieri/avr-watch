AVR Watch
=========

This project is small wrist watch using an AVR microcontroller, a 7-segment
display and the V-USB soft-usb stack.

License
-------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Contents
--------

    COPYING     text version of the GPL
    README      this file
    bootloader/ source directory for bootloader
    firmware/   source directory for firmware
    hardware/   source directory for the hardware design
    scripts/    support scripts

Bootloader
----------

Building the bootloader requires an avr-gcc toolchain, in the bootloader/
directory, to build run:

    make

Flashing the firmware on the device requires avrdude and a compatible hardware
programmer. Default configuration is stored at the beginning of the Makefile.
To program with the default configuration, run:

    make load

Fuses can be programmed running:

    make fuse

Firmware
--------

Building the firmwware requires an avr-gcc toolchain, in the firmware/
directory, to build run:

    make

Flashing the firmware on the device requires avrdude and a compatible hardware
programmer. Default configuration is stored at the beginning of the Makefile.
To program with the default configuration, run:

    make load

or using the bootloader (this will reset the device if possible):

    make boot

Hardware
--------

All hardware files (schematic, layout and libraries) are in CadSoft Eagle
format.
