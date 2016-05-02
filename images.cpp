#include"images.hpp"
#include"util.hpp"

#include"avr/pgmspace.h"
#include"HardwareSerial.h"

namespace spipgm = spi_programmer;

namespace
{
	const PROGMEM char attiny25_name[] = "Attiny25";
	const PROGMEM spipgm::device_pgm_t attiny25 = {
		attiny25_name,            // device name
		0x1E910B,                 // device signature
		{0xFF, 0xF1, 0xD5, 0xFF}, // programming fuses
		{0xFF, 0x62, 0xDF, 0xFF}, // default fuses
		2048,                     // flash size
		32                        // page size
	};

	const PROGMEM char attiny45_name[] = "Attiny45";
	const PROGMEM spipgm::device_pgm_t attiny45 = {
		attiny45_name,            // device name
		0x1E920B,                 // device signature
		{0xFF, 0xF1, 0xD5, 0xFF}, // programming fuses
		{0xFF, 0x62, 0xDF, 0xFF}, // default fuses
		4096,                     // flash size
		64                        // page size
	};

	const PROGMEM char attiny85_name[] = "ATtiny85";
	const PROGMEM spipgm::device_pgm_t attiny85 = {
		attiny85_name,            // device name
		0x1E930B,                 // device signature
		{0xFF, 0xF1, 0xD5, 0xFF}, // programming fuses
		// {0xFF, 0x62, 0xDF, 0xFF}, // programming fuses
		{0xFF, 0x62, 0xDF, 0xFF}, // default fuses
		// {0xFF, 0xF1, 0xD5, 0xFE}, // bootloader settings
		8192,                     // flash size
		64                        // page size
	};

	const PROGMEM char zaberzaber[] = "zaberzaber";
	const PROGMEM char experiment[] = "experiment";
	const PROGMEM char foobar[]     = "foobar";

	const PROGMEM spipgm::image_pgm_t image_array[] = {
		{'a', zaberzaber, &attiny85, 0},
		{'e', experiment, &attiny85, 0},
		{'f', foobar, &attiny85, 0}
	};

	const PROGMEM uint8_t image_count =
		sizeof(image_array) / sizeof(image_array[0]);

	static_assert(
		sizeof(image_array) / sizeof(image_array[0]) < 256,
		"too many images, must be less than 256");

	const PROGMEM spipgm::device_pgm_t* const device_ptr_array[] = {
		&attiny25,
		&attiny45,
		&attiny85
	};

	const PROGMEM uint8_t device_count =
		sizeof(device_ptr_array) / sizeof(device_ptr_array[0]);

	static_assert(
		sizeof(device_ptr_array) / sizeof(device_ptr_array[0]) < 256,
		"too many devices, must be less than 256");
}

const spipgm::image_pgm_t& images::user_selected_image()
{
	Serial.println(F("Enter key to load one of following images:"));
	for (int i = 0; i < pgm_read_byte(&image_count); ++i)
	{
		const spipgm::image_pgm_t& img = image_array[i];
		Serial.print(F(" enter '"));
		Serial.print(static_cast<char>(pgm_read_byte(&img.key)));
		Serial.print(F("' for "));
		Serial.println(util::FF(&img.image_name));
	}

	int ix = -1;
	do
	{
		char c = Serial.read();
		for (int i = 0; i < image_count; ++i)
		{
			const spipgm::image_pgm_t& img = image_array[i];
			if (c == pgm_read_byte(&img.key))
			{
				ix = i;
				break;
			}
		}
	} while(ix < 0);

	Serial.print("\nselected image ");
	Serial.println(util::FF(&image_array[ix].image_name));
	return image_array[ix];
}

const spipgm::device_pgm_t* images::device_for_signature(uint32_t sig)
{
	for (int i = 0; i < pgm_read_byte(&device_count); ++i)
	{
		const spipgm::device_pgm_t* dp =
			reinterpret_cast<const spipgm::device_pgm_t*>(
				pgm_read_ptr(&device_ptr_array[i]));

		if (sig == pgm_read_dword(&dp->expected_signature))
			return dp;
	}

	return nullptr;
}
