# spincoater-controller

Simple spincoater controller, written in C++ for the STM32 and 
[Tapper](https://github.com/mcbridejc/tapper) board.

## What it does

It provides a touchscreen UI for speed control with a quadcopter motor and ESC. I 
made it for a spin coater, but really it's more general than that.
It reads a tachometer pulse from a reflective optical sensor to get
RPM, and drives a PWM to a hobby ESC to regulate the motor speed. More info on 
the spincoater design are in [this repo](https://github.com/mcbridejc/spincoater).

## Checkout and build

You'll need: the arm gcc toolchain, OpenOCD, and lbuild. See the 
[modm installation guide](https://modm.io/guide/installation/) for detailed
instructions.

`git clone ssh://git@github.com/mcbridejc/spincoater-controller --recursive`

`lbuild build` to generate the modm libraries

`make` to compile

`make program` to download via the built-in STLink and OpenOCD. 

## Hardware

The [Tapper](https://github.com/mcbridejc/tapper) requires some modification
to add some resistors and connect GPIOs. 

![Board Mod Schematic](doc/tapper_modification_schematic.png)

![Board Modifications](doc/tapper_modifications.jpg)

## Embedded image updates

The UI uses a few bitmaps for buttons. These are created in Gimp and saved in 
the PPM format. The `images/convert_images.py` python script converts them
to header files. The header files are committed, so this only needs to be done
if images are added or changed. 

The images are stored uncompressed in full color, so they can get rather big
quickly. If flash becomes scarce, some compression may be in order. I suspect
a simple run-length compression would go a long way.