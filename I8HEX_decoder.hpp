#ifndef I8HEX_decoder_HPP
#define I8HEX_decoder_HPP

#include<stdint.h>
#include<stddef.h>
#include<string.h>

namespace I8HEX
{
	class Decoder
	{
	public:
		// callback type used to notify of decoded I8HEX buffer
		// It should return an error string in case of failure
		// dealing with a full buffer or nullptr if success.
		// The buffer address in ram, and address on target device
		// (and page size) can be retrieved from Decoder argument.
		typedef const char* (*buffer_full_callback_type)
			(const Decoder&);

		// construct with external buffer to use to decode I8HEX
		// bytes into, it must be the same size as target chip
		// page size
		Decoder(void* const buffer,
			const size_t page_size,
			buffer_full_callback_type bfc)
			: buffer(static_cast<uint8_t*>(buffer))
			, page_size(page_size)
			, buffer_full_callback(bfc)
		{
			memset(buffer, -1, page_size);
		}

		// return number of char consumed
		// check result of done(), error()
		// when it returns
		size_t decode(const char* str, size_t length);

		// return true if char consumed, false if an error
		// condition occurred in which case error()
		// will return a valid pointer (not nullptr)
		// check result of done(), error()
		// when it returns
		bool decode(const char c)
		{
			return (this->*decoder_func)(c);
		}

		// return address where buffer should be loaded on target
		// chip, this address is decoded from I8Hex and aligned
		// on a page_size boundary
		uint16_t get_buffer_address_on_target() const
		{
			return buffer_address;
		}

		// return true if all input was decoded and record type 0x01
		// was decoded
		// (check error() to check if decode was successful)
		bool done() const
		{
			return done_flag;
		}

		// return nullptr if no errors, otherwise string to error
		const char* error() const
		{
			return error_str;
		}

		// public for callback to retrieve
		uint8_t* const buffer;
		const size_t page_size;

		// may replace callback if required
		buffer_full_callback_type buffer_full_callback;;

	private:

		// bufptr points to the next byte into which to decode
		// a value of nullptr indicates no decoding has taken place
		uint8_t* bufptr = nullptr;  // next byte to decode into
		size_t remaining;           // remaining bytes to decode
		bool done_flag = false;
		const char* error_str = nullptr;

		struct hexbyte
		{
			uint8_t val;       // deliberately not initialised
			uint8_t shift = 9; // set to not done but large
			                   // enough to force initialisation
			                   // on first use
			bool done() const { return shift == 8; }
			bool decode(const char, const char*&);
			void reset() { shift = 9; } // twiddle twiddle I know
		};

		hexbyte expected_length; // number of payload bytes to decode
		uint16_t address;        // decode address into this field
		hexbyte address_msb;     // helper to decode address
		hexbyte address_lsb;     // helper to decode address
		hexbyte record_type;     // 0x00 or 0x01 for last decode
		hexbyte payload_byte;    // current payload byte being decoded
		hexbyte checksum;        // decode into this

		uint16_t buffer_address; // address of page aligned buffer
		uint8_t calculated_checksum;

		void call_buffer_full_callback();

		typedef bool (Decoder::*decfunc)(const char);

		bool decode_colon(const char);
		bool decode_expected_length(const char);
		bool decode_address(const char);
		bool address_check(const char);
		bool decode_record_type(const char);
		bool decode_payload(const char);
		bool adjust_after_full(const char);
		bool decode_checksum(const char);
		bool decode_upto_newline(const char);
		bool decode_done(const char);

		decfunc decoder_func = &Decoder::decode_colon;
	};
}

#endif
