#include"I8HEX_decoder.hpp"

size_t I8HEX::Decoder::decode(const char* str, size_t length)
{
	size_t consumed = 0;

	while (length && !done() && !error())
	{
		if (decode(*str))
		{
			--length;
			++consumed;
			++str;
		}
	}

	return consumed;
}

bool I8HEX::Decoder::hexbyte::decode(const char c, const char*& error_str)
{
	uint8_t nibble;
	if (c >= '0' && c <= '9')
	{
		nibble = c - '0';
	}
	else if (c >= 'a' && c <= 'f')
	{
		nibble = c - 'a' + 10;
	}
	else if (c >= 'A' && c <= 'F')
	{
		nibble = c - 'A' + 10;
	}
	else
	{
		error_str = "Invalid character, expected hexadecimal digit";
		return false;
	}

	if (shift >= 8)
	{
		// reset for reuse (or first use)
		val = 0;
		shift = 0;
	}

	val = val << shift | nibble;
	shift += 4;
	return true;
}

void I8HEX::Decoder::call_buffer_full_callback()
{
	error_str = (*buffer_full_callback)(*this);
	bufptr = nullptr;
	memset(buffer, -1, page_size);
}

bool I8HEX::Decoder::decode_colon(const char c)
{
	if (c == ':')
	{
		decoder_func = &Decoder::decode_expected_length;
		return true;
	}

	error_str = "Invalid character, expected colon";
	return false;
}

bool I8HEX::Decoder::decode_expected_length(const char c)
{
	if (expected_length.decode(c, error_str))
	{
		if (expected_length.done())
		{
			decoder_func = &Decoder::decode_address;
			calculated_checksum = expected_length.val;
		}
		return true;
	}
	return false;
}

bool I8HEX::Decoder::decode_address(const char c)
{
	hexbyte* hb = address_msb.done() ? &address_lsb : &address_msb;

	if (hb->decode(c, error_str))
	{
		if (address_msb.done() && address_lsb.done())
		{
			address = address_msb.val << 8 | address_lsb.val;
			calculated_checksum += address_msb.val;
			calculated_checksum += address_lsb.val;
			decoder_func = &Decoder::address_check;
			address_msb.reset();
			address_lsb.reset();
		}
		return true;
	}
	return false;
}

bool I8HEX::Decoder::address_check(const char c)
{
	if (bufptr) // busy decoding buffer
	{
		if (address < buffer_address ||
		    address >= buffer_address + page_size)
		{
			// new address is outside range of current buffer
			if (expected_length.val)
			{
				// and there will be data to decode,
				// so flag current buffer as full
				call_buffer_full_callback();
				return false;
			}
			// although new address is outside of current
			// buffer range this I8HEX line does not have any
			// data to contribute so leave bufptr unchanged
		}
		else
		{
			// new address is within current buffer
			bufptr = buffer + (address - buffer_address);
		}
	}
	else // not busy decoding buffer
	{
		// round buffer_address down to nearest page_size
		size_t remainder = address % page_size;
		buffer_address = address - remainder;
		bufptr = buffer + remainder;
		remaining = page_size - remainder;
	}

	decoder_func = &Decoder::decode_record_type;
	return decode(c);
}

bool I8HEX::Decoder::decode_record_type(const char c)
{
	if (record_type.decode(c, error_str))
	{
		if (record_type.done())
		{
			calculated_checksum += record_type.val;
			if (record_type.val == 0x00 ||
			    record_type.val == 0x01)
			{
				decoder_func = &Decoder::decode_payload;
			}
			else
			{
				error_str = "Invalid/unsupported record "
					"type, must be 0 or 1";
				return false;
			}
		}
		return true;
	}
	return false;
}

bool I8HEX::Decoder::decode_payload(const char c)
{
	if (expected_length.val)
	{
		if (remaining == 0)
		{
			decoder_func = &Decoder::adjust_after_full;
			call_buffer_full_callback();
			return false;
		}
		if (payload_byte.decode(c, error_str))
		{
			if (payload_byte.done())
			{
				*bufptr = payload_byte.val;
				++bufptr;
				--remaining;
				--expected_length.val;
				calculated_checksum += payload_byte.val;
			}
			return true;
		}
	}
	else
	{
		decoder_func = &Decoder::decode_checksum;
		return decode(c);
	}
	return false;
}

bool I8HEX::Decoder::adjust_after_full(const char c)
{
	buffer_address += page_size;
	bufptr = buffer;
	remaining = page_size;
	decoder_func = &Decoder::decode_payload;
	return decode(c);
}

bool I8HEX::Decoder::decode_checksum(const char c)
{
	if (checksum.decode(c, error_str))
	{
		if (checksum.done())
		{
			calculated_checksum += checksum.val;
			if (calculated_checksum)
			{
				error_str = "Invalid checksum";
				return false;
			}

			decoder_func = &Decoder::decode_upto_newline;
		}
		return true;
	}
	return false;
}

bool I8HEX::Decoder::decode_upto_newline(const char c)
{
	if (c == '\n')
	{
		if (record_type.val == 0x01)
		{
			done_flag = true;
			// buffer is full if there was decoding
			// in progress
			decoder_func = &Decoder::decode_done;
			call_buffer_full_callback();
			return false;
		}
		decoder_func = &Decoder::decode_colon;
	}
	return true;
}

bool I8HEX::Decoder::decode_done(const char c)
{
	if (c == '\n')
	{
		decoder_func = &Decoder::decode_colon;
		return true;
	}
	error_str = "Invalid character, expected newline";
	return false;
}
