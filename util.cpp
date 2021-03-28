#include"util.hpp"

#include<ctype.h>

namespace
{
	// return nibble in least significant 4 bits, or 0xff if invalid
	uint8_t nibble_for_hex_digit(const char dig)
	{
		if (dig >= '0' && dig <= '9')
			return dig - '0';
		if (dig >= 'A' && dig <= 'F')
			return dig - 'A' + 10;
		if (dig >= 'a' && dig <= 'f')
			return dig - 'a' + 10;
		return 0xff;
	}

	// hex digits for input (mixed case) and output (upper case only)
	const char hexdigits[] = "0123456789ABCDEFabcdef";

	void serial_write_byte(const uint8_t byte)
	{
		Serial.print(hexdigits[byte >> 4]);
		Serial.print(hexdigits[byte & 0x0f]);
	}
}

char util::serial_read_char()
{
	char c;
	do
	{
		do { } while (!Serial.available());
		c = Serial.read();
	} while (c < 0 || c == 127); // ignore DEL
	return c;
}

char util::serial_read_char_of(const char *options)
{
	int ix = -1;
	do
	{
		char c = serial_read_char();
		for (int i = 0; options[i]; ++i)
		{
			if (c == options[i])
			{
				ix = i;
				break;
			}
		}
	} while (ix < 0);

	return options[ix];
}

uint8_t util::serial_read_hex_byte()
{
	Serial.println(F("Enter byte as 2 hex digits: "));
	char msn = serial_read_char_of(hexdigits);
	Serial.print(msn);
	char lsn = serial_read_char_of(hexdigits);
	Serial.print(lsn);
	Serial.println();
	// no need to verify return value of nibble_for_hex_digit()
	// because serial_read_char_of() will only return valid values
	return nibble_for_hex_digit(msn) << 4 | nibble_for_hex_digit(lsn);
}

uint16_t util::serial_read_power_of_two(uint16_t start)
{
	char valid_chars[] = "idq0123456789";
	static const int slots = 8;
	uint16_t values[slots];
	uint16_t value = 0;
	do
	{
		Serial.println(F("Select one of these sizes:"));
		uint16_t t = start;
		for (int i = 0; i < slots && t; ++i)
		{
			Serial.print(i);
			Serial.print(F(" - for "));
			Serial.println(t);
			valid_chars[3 + i] = i + '0';
			valid_chars[4 + i] = 0;
			values[i] = t;
			t <<= 1;
		}
		Serial.println(F("i - increase values"));
		Serial.println(F("d - decrease values"));
		Serial.println(F("q - quit to abort"));

		char c = serial_read_char_of(valid_chars);
		switch (c)
		{
		case 'i' :
		{
			t = start;
			for (int i = 0; i < slots && t; ++i)
			{
				start = t;
				t <<= 1;
			}
			break;
		}
		case 'd' :
		{
			t = start;
			for (int i = 0; i < slots && t; ++i)
			{
				start = t;
				t >>= 1;
			}
			break;
		}
		case 'q' :
		{
			return 0;
		}
		default:
		{
			value = values[c - '0'];
		}
		}

		if (value)
		{
			Serial.print(F("You selected "));
			Serial.println(value);
			Serial.println(F("is this correct? (y/n)"));
			char y = serial_read_char_of("yn");
			value = y == 'y' ? value : 0;
		}

	} while (!value);

	return value;
}

size_t util::serial_read_until_nl(char* buffer, size_t length)
{
	if (length)
	{
		for (size_t t = 0; t < length; ++t)
		{
			buffer[t] = serial_read_char();
			if (buffer[t] == '\n')
				length = t + 1;
		}
	}
	return length;
}

void util::serial_write_I8HEX(const uint16_t address,
			      const char* data, uint8_t bytes)
{
	Serial.print(':');
	serial_write_byte(bytes);
	serial_write_byte(address >> 8);
	serial_write_byte(address & 0xff);
	Serial.print("00");

	uint8_t checksum = bytes + (address >> 8) + (address & 0xff);

	for (; bytes; ++data, --bytes)
	{
		serial_write_byte(*data);
		checksum += *data;
	}

	serial_write_byte(-checksum);

	Serial.println();
}

bool util::impl::serial_read_value(
	uint32_t& value,
	const size_t max_digits,
	char& last_char)
{
	size_t digits_read = 0;
	char c = 0;
	for (; digits_read < max_digits; ++digits_read)
	{
		c = serial_read_char();
		uint8_t nibble = nibble_for_hex_digit(c);
		if (nibble == 0xff)
			break;
		value = value << 8 | nibble;
	}

	if (digits_read && digits_read <= max_digits && isspace(c))
	{
		c = last_char;
		return true;
	}

	if (digits_read == max_digits)
		Serial.println(F("Too many hex digits for value"));
	else
		Serial.println(F("Invalid non-hex digit read"));
	return false;
}
