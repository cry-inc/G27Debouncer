Logitech G27 Software Debouncer

The right shifting paddle of my G27 driving wheel started to report multiple
button activations while I pulled only once. Since I could not find any
available replacement parts and didn't like the idea of soldering a third
party switch onto the original board, I went ahead and tried to find a
software solution. This repository contains the result.

This tool uses the SDL library to read the button state reported by the
original G27 driver. It then filters the data and forwards it to a virtual
second input device. The virtual input device is provided by a third party
library called vJoy. The buttons of the virtual device should then be
used as input in the settings of your games.

Third party components:
* SDL 2 - https://www.libsdl.org/ (zlib License)
* vJoy - http://vjoystick.sourceforge.net/ (Public Domain)

How to compile:

The software is currently only available for 64bit Windows operating systems
and indented to be compiled with Microsoft Visual Studio 2010. Newer MSVC
should also work after upgrading the solution. The required external libraries
SDL and vJoy are included. However, the vJoy driver must be installed
separately. The see the vJoy website for details.
