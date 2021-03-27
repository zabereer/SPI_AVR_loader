#include"devices.hpp"
#include"util.hpp"

namespace
{
	const PROGMEM char attiny25_name[] = "Attiny25";
	const PROGMEM devices::device_pgm_t attiny25 = {
		attiny25_name,            // device name
		0x1E910B,                 // device signature
		2048,                     // flash size
		32                        // page size
	};

	const PROGMEM char attiny45_name[] = "Attiny45";
	const PROGMEM devices::device_pgm_t attiny45 = {
		attiny45_name,            // device name
		0x1E920B,                 // device signature
		4096,                     // flash size
		64                        // page size
	};

	const PROGMEM char attiny85_name[] = "ATtiny85";
	const PROGMEM devices::device_pgm_t attiny85 = {
		attiny85_name,            // device name
		0x1E930B,                 // device signature
		8192,                     // flash size
		64                        // page size
	};

	const PROGMEM devices::device_pgm_t* const device_ptr_array[] = {
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

const devices::device_pgm_t* devices::device_for_signature(uint32_t sig)
{
	for (int i = 0; i < pgm_read_byte(&device_count); ++i)
	{
		const device_pgm_t* dp =
			reinterpret_cast<const devices::device_pgm_t*>(
				pgm_read_ptr(&device_ptr_array[i]));

		if (sig == pgm_read_dword(&dp->expected_signature))
			return dp;
	}

	return nullptr;
}
