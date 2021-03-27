#ifndef DEVICES_HPP
#define DEVICES_HPP

#include<stdint.h>
#include<avr/pgmspace.h>

namespace devices
{
	// target device details - must reside in program memory
	struct device_pgm_t
	{
		const char*    device_name;
		const uint32_t expected_signature;
		const uint16_t flash_size;
		const uint16_t page_size;

		uint32_t get_expected_signature() const
		{
			return pgm_read_dword(&expected_signature);
		}

		uint16_t get_flash_size() const
		{
			return pgm_read_word(&flash_size);
		}

		uint16_t get_page_size() const
		{
			return pgm_read_word(&page_size);
		}
	};

	// Return the device struct in program memory for given signature.
	// Return nullptr if device not found.
	const device_pgm_t* device_for_signature(uint32_t);
}

#endif
