# SPI_AVR_loader
Experimental SPI loader running on Arduino Uno using SPI to load code onto other AVR chips (like ATtiny85)

This code runs on an Arduino Uno and can be used to use SPI to program another AVR device.
I wrote this to program Adafruit Trinket (ATtiny85) devices from Linux.

It depends on Arduino-Makefile (https://github.com/sudar/Arduino-Makefile) to be present in an adjacent directory as this project.  
The Uno should be connected to a target AVR device as described by https://www.arduino.cc/en/reference/SPI  
It uses the serial interface to present a menu which can be used to read and modify the target device fuse bytes
and backup or load a new program image in I8HEX format (https://en.wikipedia.org/wiki/Intel_HEX) onto the device.

Any Arduino project build with the Arduino-Makefile will have an I8HEX file in its output build directory.
This file can be loaded onto a target device via the serial output menus from the Uno using miniterm.py:
```
  $ miniterm.py --eol=LF /dev/ttyACM0
  --- Miniterm on /dev/ttyACM0  9600,8,N,1 ---
  --- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---

  Experimental SPI programmer

  === Main menu ===
  v - toggle verbose (current N)
  s - display device signature
  f - read/write fuses
  b - read flash (backup) to serial
  e - chip erase (seems required before load)
  l - write flash from serial (load target)
```

A device's image can be backed up using `b`, and then need to be copied from the output into a file.
To load a file select `l` and then press Ctrl+T Ctrl+U and enter the filename to load.
*However* this only seem to work after a chip erase has been performed with `e`.

I will add more documentation later.
