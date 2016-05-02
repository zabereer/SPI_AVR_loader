BOARD_TAG = uno
AVRDUDE = /usr/bin/avrdude
AVRDUDE_CONF = /etc/avrdude.conf
ARDUINO_LIBS = SPI

ARDUINO_DIR = ../arduino-ide
include ../Arduino-Makefile/Arduino.mk

CXXFLAGS += --std=c++11

test:
	$(MAKE) -C i8hex_test

test_build:
	$(MAKE) -C i8hex_test build

test_clean:
	$(MAKE) -C i8hex_test clean
