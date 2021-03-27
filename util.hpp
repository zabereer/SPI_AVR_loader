#ifndef UTIL_HPP
#define UTIL_HPP

#include<stddef.h>
#include<stdint.h>

#include<HardwareSerial.h>

namespace util
{
	// Serial must be initialised with Serial.begin() before using these

	// helpers for printing a string in program memory to Serial
	// (unlike the F macro which places a literal in program memory)
	inline const __FlashStringHelper* FF(const char* s)
	{
		return reinterpret_cast<const __FlashStringHelper*>(s);
	}

	inline const __FlashStringHelper* FF(const char* const* s)
	{
		return reinterpret_cast<const __FlashStringHelper*>(
			pgm_read_ptr(s));
	}

	// read single char from serial, blocks until one is available
	char serial_read_char();

	// read single character from serial and return it, keep on
	// reading until one of characters in the C string options
	// is read
	char serial_read_char_of(const char* options);

	// read 2 hex digits case insensitive and return the binary
	// equivalent byte - ignore invalid hex digits on serial
	uint8_t serial_read_hex_byte();

	// presents menu on serial prompting for a power of 2
	// returns 0 if user selected to abort
	// start must be a power of two, no validation/correction is done
	uint16_t serial_read_power_of_two(uint16_t start = 2);

	// read into buffer up to and including newline or length,
	// return bytes stored in buffer including newline
	size_t serial_read_until_nl(char* buffer, size_t length);

	// read hex digits from serial converting to binary value up to
	// first non-hex or overflow digit and store it in last_char
	// return true if valid value, false if overflow or no hex digits
	template <typename T>
	bool serial_read_value(T& value, char& last_char);

	// write I8HEX format output on serial representing data for
	// a length of bytes (address is where the data resided in
	// program memory of the target device)
	void serial_write_I8HEX(const uint16_t address,
				const char* data,
				uint8_t bytes);

	namespace impl
	{
		bool serial_read_value(
			uint32_t& value,
			const size_t max_digits,
			char& last_char);
	}
}

template <typename T>
bool util::serial_read_value(T& value, char& last_char)
{
	uint32_t tv = 0;
	if (impl::serial_read_value(tv, sizeof(T) * 2, last_char))
	{
		value = tv;
		return true;
	}
	return false;
}

#endif
