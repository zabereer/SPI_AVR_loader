# SPI_AVR_loader
Experimental SPI loader running on Arduino Uno using SPI to load code onto other AVR chips (like ATtiny85)

This code runs on an Arduino Uno. It depends on Arduino-Makefile to be build (https://github.com/sudar/Arduino-Makefile).
The Uno should be connected to a target AVR device as described by https://www.arduino.cc/en/reference/SPI  
It uses the serial interface to present a menu which can be used to read and modify the target device fuse bytes
and backup or load a new program image in I8HEX format (https://en.wikipedia.org/wiki/Intel_HEX) onto the device.
