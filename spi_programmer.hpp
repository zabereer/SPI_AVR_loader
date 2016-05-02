#ifndef SPI_PROGRAMMER_HPP_
#define SPI_PROGRAMMER_HPP_

#include"avr/pgmspace.h"
#include"SPI.h"           // included for SPISettings

#include<stddef.h>
#include<stdint.h>

namespace spi_programmer
{
	// These functions use Serial for errors, ensure it is initialised
	// with Serial.begin() prior to calling any of these.

	// represents fuse bytes in flash (program memory)
	struct fuses_pgm_t
	{
		uint8_t lock;
		uint8_t low;
		uint8_t high;
		uint8_t ext;
	};

	// represents fuse bytes in SRAM (on stack)
	struct fuses_t
	{
		uint8_t lock;
		uint8_t low;
		uint8_t high;
		uint8_t ext;

		fuses_t() = default;
		fuses_t(const uint8_t lock,
			const uint8_t low,
			const uint8_t high,
			const uint8_t ext)
			: lock(lock)
			, low(low)
			, high(high)
			, ext(ext)
		{ }

		// conversion constructor from flash to SRAM based fuses
		fuses_t(const fuses_pgm_t& fuses)
		{
			memcpy_P(this, &fuses, sizeof(fuses_t));
		}
	};

	inline bool operator==(const fuses_t& a, const fuses_t& b)
	{
		return a.lock  == b.lock &&
			a.low  == b.low  &&
			a.high == b.high &&
			a.ext  == b.ext;
	}

	inline bool operator!=(const fuses_t& a, const fuses_t& b)
	{
		return a.lock  != b.lock ||
			a.low  != b.low  ||
			a.high != b.high ||
			a.ext  != b.ext;
	}

	static_assert(sizeof(fuses_pgm_t) == sizeof(fuses_t),
		      "sizeof fuses_pgm_t != sizeof fuses_t");

	// target device details - must reside in program memory
	struct device_pgm_t
	{
		const char*       device_name;
		const uint32_t    expected_signature;
		const fuses_pgm_t programming_fuses;
		const fuses_pgm_t default_fuses;
		const uint16_t    flash_size;
		const uint16_t    page_size;

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

		bool verify_signature(bool verbose = false) const;
		bool write_programming_fuses(bool verbose = false) const;
		bool write_default_fuses(bool verbose = false) const;
	};

	struct image_pgm_t
	{
		const char          key; // for image selection
		const char*         image_name;
		const device_pgm_t* device;
		const char*         hexcode;

		const device_pgm_t& get_device() const
		{
			return *reinterpret_cast<const device_pgm_t*>(
				pgm_read_ptr(&device));
		}

		bool write_to_device(bool verbose = false) const;
	};

	void powerup_avr();
	void powerdown_avr();

	bool program_enable(SPISettings, int retries = 2,
			    bool verbose = false);
	void program_disable();

	// verify image signature from program memory with device signature
	bool verify_signature(const image_pgm_t&,
			      bool verbose = false);

	// write image from program memory
	bool write_image_pgm(const image_pgm_t&,
			     bool verbose = false);

	uint32_t read_signature(bool verbose = false);
	fuses_t read_fuses(bool verbose = false);

	void write_fuses(const fuses_t&, bool verbose = false);
	bool write_verify_fuses(const fuses_t&, bool verbose = false);

	// read program memory bytes of target device from an even
	// address into buffer (address and bytes *must* be even)
	void read_program_memory(uint16_t address, void* buffer,
				 size_t bytes, bool verbose = false);

	// load program memory bytes into target device page buffer
	// from an even address from buffer
	// (address and bytes *must* be even)
	void load_program_memory(uint16_t address, const void* buffer,
				 size_t bytes, bool verbose = false);

	// write previously loaded page buffer into program memory
	void write_program_page(uint16_t address, bool verbose = false);

	uint16_t spi_trans(uint8_t, uint8_t, uint8_t, uint8_t,
			   bool verbose = false);

	// return true if rdy/bsy flag is set (true when device is busy)
	bool device_busy();
	void wait_device_ready();

	// output fuses on Serial
	void output_fuses(const fuses_t&);
	// output fuses from program memory to Serial
	void output_fuses_pgm(const fuses_t&);

	// erase flash and EEPROM
	void perform_chip_erase(bool verbose = false);

	[[noreturn]] void failure(const char* error);
	[[noreturn]] void failure(const __FlashStringHelper*);
}

#endif
