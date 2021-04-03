# SPI_AVR_loader
SPI loader running on Arduino Uno using SPI to load code onto other AVR chips (like ATtiny85)

This code runs on an Arduino Uno and can be used to use SPI to program another AVR device.
I wrote this to program Adafruit Trinket (ATtiny85) devices from Linux.

It depends on Arduino-Makefile (https://github.com/sudar/Arduino-Makefile) to be present in an adjacent directory as this project.  
The Uno should be connected to a target AVR device as described by https://www.arduino.cc/en/reference/SPI  
It uses the serial interface to present a menu which can be used to read and modify the target device fuse bytes
and backup or load a new program image in I8HEX format (https://en.wikipedia.org/wiki/Intel_HEX) onto the device.

Any Arduino project build with the Arduino-Makefile will have an I8HEX file in its output build directory.
This file can be loaded onto a target device via the serial output menus from the Uno using miniterm.py:

## Wiring
### ISP
```
  Connections when using Arduino UNO as ISP for Adafruit Trinket
  +===============+===============+===============+
  |    Trinket    |  Arduino Uno  |  SPI (target) |
  +===============+===============+===============+
  |     VBAT      |   3V / 5V     |     Vcc       |
  +---------------|---------------|---------------+
  |     GND       |      GND      |     GND       |
  +---------------|---------------|---------------+
  |     RST       |      #10      |      SS       |
  +---------------|---------------|---------------+
  |      #0       |      #11      |     MOSI      |
  +---------------|---------------|---------------+
  |      #1       |      #12      |     MISO      |
  +---------------|---------------|---------------+
  |      #2       |      #13      |     SCK       |
  +---------------|---------------|---------------+
```
Also see the AVR [ATtiny85 datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf) section 20.5 for more details.

### High voltage serial programming
Use this only to reset fuses of a bricked chip or chip erase. The pins were chosen to match the SPI pins as closely as possible to avoid having to completely rewire.
```
  Connections for high voltage programming
  +===============+===============+===============+
  |    Trinket    |  Arduino Uno  | HVSP (target) |
  +===============+===============+===============+
  |     VBAT      |      #9       |     Vcc       |
  +---------------|---------------|---------------+
  |     GND       |      GND      |     GND       |
  +---------------|---------------|---------------+
  |     RST       |#8 (transistor)|     12V*      |
  +---------------|---------------|---------------+
  |      #0       |      #11      |     SDI       |
  +---------------|---------------|---------------+
  |      #1       |      #12      |     SII       |
  +---------------|---------------|---------------+
  |      #2       |      #13      |     SDO       |
  +---------------|---------------|---------------+
  |      #3       |      #10      |     SCI       |
  +---------------|---------------|---------------+
```
External 12V supply on RESET pin is required, ensure an NPN transistor's base is connected to the Uno pin #8 to control the switching of the 12V to the Trinket's RESET pin.  
The miniterm menu will prompt whether you used an inverted level shifter or not. If inverted then pin #8 will be HIGH to shut off the 12V supply on Trinket's RESET pin.  
Remember resistors on all connections (SDI, SII, SDO, SCI, #8) (1K) and on 12V input source, but not on Vcc. (Depending on 12V source adjust resistor - 10K worked for me using 9V 6LR61 and 3V CR2032).
Also see the AVR [ATtiny85 datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf) section 20.6 and 20.7 for more details.

## Example use
After uno is wired to Trinket and plugged in.
```
$ pyserial-miniterm

--- Available ports:
---  1: /dev/ttyACM0         'ttyACM0'
--- Enter port index or full name: 1
--- Miniterm on /dev/ttyACM0  9600,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---

AVR SPI programmer

=== Main menu ===
v - toggle verbose (current N)
s - display device signature
f - read/write fuses
b - read flash (backup) to serial
e - chip erase (sometimes required before load)
l - write flash from serial (load target)
z - zap fuses with high voltage serial programming

f
Failed to enable programming at clock 8000000 - reducing to 4000000
=== Fuses ===
r - read fuses
w - write fuses
q - quite fuses menu

r
lock = 0xFF  low = 0xF1  high = 0xD5  ext = 0xFE

```

A device's image can be backed up using `b`, and then need to be copied from the output into a file.
To load a file select `l` and then press Ctrl+T Ctrl+U and enter the filename to load.
Sometimes this only works after a chip erase has been performed with `e`.

```
=== Main menu ===
v - toggle verbose (current N)
s - display device signature
f - read/write fuses
b - read flash (backup) to serial
e - chip erase (sometimes required before load)
l - write flash from serial (load target)
z - zap fuses using high voltage serial programming

l
Failed to enable programming at clock 8000000 - reducing to 4000000
CPU ATtiny85
Paste image below or upload hex file

--- File to upload: blinker_trinket/build-trinket3/blinker_trinket_.hex
```

Example of high voltage serial programming menu - it is quite fiddly to use, but in the end I did manage to unbrick an Adafruit trinket by resetting its fuses.

```
High Voltage fuses reset - don't brick it further
Set to using inverted level shifter on pin 8
i - change to using non-inverted level shifter on pin 8
k - modify lock fuse for next write
l - modify low fuse for next write
h - modify high fuse for next write
x - modify ext fuse for next write
d - crude delay loop iterations - set to 0x10
r - read fuses (will prompt to connect 12V)
w - write fuses (will prompt to connect 12V)
e - chip erase (will prompt to connect 12V)
q - quite high voltage fuses reset menu

  ! DO NOT connect 12V source yet !

About to read fuses using high voltage
Pin #8 set to HIGH to disable external 12V source
Connect external 12V power source now
Press y to continue, n to abandon
```
