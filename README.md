Logitech G27 Software Debouncer

This tool reads the shifting paddle state reported by the G27 Windows driver.
It then filters the data and forwards it as output of a second virtual device.
The two buttons of the virtual device can then be used as input in the settings
of your games and report just one instead of many buttons clicks. Especially
useful for slightly defect wheels and games without their own input filters.

Motivation:

The right shifting paddle of my G27 driving wheel started to report multiple
button activations while I pulled it only once. Since I could not find any
available replacement parts and didn't like the idea of soldering a third
party switch onto the original board, I went ahead and tried to find a
software solution. This repository contains the result.

How to compile:

The software is currently only available for 64bit Windows operating systems
and indented to be compiled with Microsoft Visual Studio 2010. Newer MSVC
should also work after upgrading the solution. The required external libraries
SDL and vJoy are included. However, the vJoy driver must be installed
separately. See the vJoy website for more details.

How to use:

1. Compile with MSVC >= 2010
2. Install the vJoy driver and set a device with at least two buttons
3. Configure the software using the INI file (defaults should work)
4. Start the tool
5. Start the game and use the F5/F6 keys to bind the virtual device buttons.
   How it works: After pressing F5/F6 it will wait 5 seconds and then trigger
   the virtual left/right joystick button. If you just pull the shifting
   paddle to assign the button, it will most probably bind the original button
   instead of the virtual filtered one!
6. Play the game and enjoy the filtered input!
 
Filtering details:

The software uses a very simple algorithm: If the hardware reports a button
down event, the virtual device will always activate the corresponding virtual
button for 300ms, no matter how long or short the hardware button was pressed.
While the virtual button is pressed, further input changes are ignored.
The next button down event from the hardware will not activate the virtual
button again unless the event occurs at least 400ms after the last time the
virtual button was activated. The software polls the hardware every 10ms for
new events. All of the above timings can be changed using the INI configuration
file.

Third party components:

* SDL 2 - https://www.libsdl.org/ (zlib License)
  Used to read the output of the G27 Windows driver.
* vJoy - http://vjoystick.sourceforge.net/ (Public Domain)
  Used to create and feed a virtual joystick device.
