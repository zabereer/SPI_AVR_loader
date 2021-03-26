BOARD_TAG = uno
ARDMK_VENDOR = arduino
ARDUINO_LIBS = SPI

# path where arduino IDE is (from https://www.arduino.cc/en/software)
ARDUINO_DIR = ../arduino
include ../Arduino-Makefile/Arduino.mk

CXXFLAGS += --std=c++11

test:
	$(MAKE) -C i8hex_test

test_build:
	$(MAKE) -C i8hex_test build

test_clean:
	$(MAKE) -C i8hex_test clean
