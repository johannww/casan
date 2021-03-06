CASAN slave for Arduino
=====================

Introduction
------------

This is the source code for the slave processor running on
Arduino/Zigduino boards.


Prerequisites
-------------

The CASAN slave program needs:
- the Arduino IDE (not used at all, but installing it will bring all
    required dependancies for compilation of Arduino sketches)
- the `screen` program (in order to access serial ports)


On Linux, the needed packages are:
- arduino
- screen
- python-serial (libdevice-serialport-perl is no longer needed)

On FreeBSD, the needed ports are:
- devel/arduino
- sysutils/screen
- comms/p5-Device-SerialPort
- devel/gmake

Zigduino installation
---------------------

If you intend to use IEEE 802.15.4 with the Zigduino board, you
need to download the proper Zigduino firmware from
https://github.com/logos-electromechanical/Zigduino-1.0

To install it, do the following steps:

    # git clone https://github.com/logos-electromechanical/Zigduino-1.0
    # ZDIR=`pwd`/Zigduino-1.0/hardware/arduino
    # ADIR=/usr/share/arduino/hardware/arduino		# standard Arduino IDE
    # ln -s $ZDIR/cores/zigduino $ADIR/cores
    # ln -s $ZDIR/variants/zigduino_r2 $ADIR/variants
    # mv $ADIR/boards.txt $ADIR/boards.txt.old
    # ln -s $ZDIR/boards.txt $ADIR

If you plan to use the SPI module, add the following line:
    const static uint8_t SS   = 10;
to $ZDIR/variants/zigduino_r2/pins_arduino.h

On FreeBSD, ADIR must be set to `/usr/local/arduino/hardware/arduino`.
On MacOS, ADIR must be set to ?


Test sketches and examples
--------------------------

The `tests` subdirectory contains some individual tests sketches
for various C++ classes of the CASAN slave. Moreover, the `tests/casan`
subdirectory contains a ready-to-use example applcation.

To run these tests:

    $ cd test/casan/
    $ make && make upload			# or make test

There is a Makefile in every test sketch, change the device you have to use 
to upload your sketch on your platform (see ../README.md for a note on
hardwiring serial devices on Linux).

On FreeBSD, the command is:

    $ make ARDUINO_DIR=/usr/local/arduino AVR_TOOLS_DIR=/usr/local MONITOR_PORT=/dev/cuaU1 test


Documentation
-------------

Program documentation is generated with Doxygen. To generate it, you need
`doxygen`, `dot` (part of graphviz) and `pdflatex`.

Just type `make pdf` to generate the `doc/latex/refman.pdf` file.
the library documentation


Notice
------

The mk/ subdirectory is a complete Makefile for Arduino sketches,
thanks to Sudar
It is extracted from github:
    git clone git@github.com:sudar/Arduino-Makefile.git
