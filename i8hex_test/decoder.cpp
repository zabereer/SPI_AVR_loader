#include<array>
#include<cassert>
#include<cstddef>
#include<cstring>
#include<deque>
#include<iomanip>
#include<iostream>
#include<sstream>
#include<string>
#include<utility>

#include"I8HEX_decoder.hpp"

template <std::size_t page_size>
class TestBase
{
protected:

	typedef std::array<std::uint8_t, page_size> raw_buffer;
	typedef std::pair<std::uint16_t, raw_buffer> addr_raw_buffer;

	TestBase()
	{
		assert(current_test_base == nullptr);
		current_test_base = this;
	}

	~TestBase()
	{
		assert(current_test_base);
		current_test_base = nullptr;
	}

public:

	void run();

private:

	virtual const char* get_i8hex() const
	{
		return "";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return std::deque<addr_raw_buffer>{};
	}

	virtual const char* expected_error() const
	{
		return nullptr;
	}

	virtual const char* name() const = 0;

	void verify_decoder_initial_state() const;
	void verify_full_raw_buffer();
	void verify_buffer(const addr_raw_buffer&);
	bool verify_error();
	void unexpected_buffer();
	std::string buffers_decoded_str() const;

	raw_buffer buffer;
	I8HEX::Decoder decoder{buffer.data(),
			buffer.size(),
			&buffer_full_verify};

	std::deque<addr_raw_buffer> expected_buffers;
	unsigned successful_buffers = 0;

	// hack to get to current TestBase via buffer full callback
	static TestBase* current_test_base;
	static const char* buffer_full_verify(const I8HEX::Decoder&);
	static const char* fail_unexpected_buffer(const I8HEX::Decoder&);
};

template <std::size_t page_size>
void TestBase<page_size>::run()
{
	std::cout << "Running test " << name() << std::endl;

	expected_buffers = get_raw_buffers();

	verify_decoder_initial_state();

	const char* i8hex_ptr = get_i8hex();
	std::size_t i8hex_len = std::strlen(i8hex_ptr);

	while (expected_buffers.size())
	{
		std::size_t len = decoder.decode(i8hex_ptr,
						 i8hex_len);
		i8hex_ptr += len;
		i8hex_len -= len;

		if (verify_error())
			return; // error was as expected
	}

	decoder.buffer_full_callback = &fail_unexpected_buffer;

	while (i8hex_len && !decoder.done()) // leftovers to consume
	{
		std::size_t len = decoder.decode(i8hex_ptr,
						 i8hex_len);
		i8hex_ptr += len;
		i8hex_len -= len;

		if (verify_error())
			return; // error was as expected
	}

	// no more input
	if (expected_error())
	{
		std::cout << name() << " : out of data "
			  << "but expected error did not occur : "
			  << expected_error()
			  << buffers_decoded_str()
			  << std::endl;
		std::exit(1);
	}
}

template <std::size_t page_size>
void TestBase<page_size>::verify_decoder_initial_state() const
{
	if (decoder.error() || decoder.done())
	{
		std::cout << name() << " : invalid initial "
			  << "decoder state"
			  << buffers_decoded_str()
			  << std::endl;
		std::exit(1);
	}
}

template <std::size_t page_size>
void TestBase<page_size>::verify_full_raw_buffer()
{
	verify_buffer(expected_buffers.front());
	expected_buffers.pop_front();
	++successful_buffers;
}

template <std::size_t page_size>
void TestBase<page_size>::verify_buffer(const addr_raw_buffer& arb)
{
	if (decoder.get_buffer_address_on_target() != arb.first)
	{
		std::cout << name() << " : target address 0x"
			  << std::setfill('0') << std::setw(4)
			  << std::hex
			  << decoder.get_buffer_address_on_target()
			  << " does not match expected address of "
			  << std::setfill('0') << std::setw(4)
			  << std::hex
			  << arb.first
			  << buffers_decoded_str()
			  << std::endl;
		std::exit(1);
	}
	for (std::size_t ix = 0; ix < page_size; ++ix)
	{
		unsigned decoded_byte =
			static_cast<unsigned>(buffer[ix]);
		unsigned expected_byte =
			static_cast<unsigned>(arb.second[ix]);
		const auto addr =
			decoder.get_buffer_address_on_target();

		if (decoded_byte != expected_byte)
		{

			std::cout << name() << " buffer data "
				  << "mismatched, target address 0x"
				  << std::setfill('0') << std::setw(4)
				  << std::hex << addr
				  << std::setw(2) << std::dec
				  << " at index "
				  << ix << ", decoded 0x"
				  << std::setfill('0') << std::setw(2)
				  << std::hex
				  << decoded_byte
				  << " but expected 0x"
				  << std::setfill('0') << std::setw(2)
				  << std::hex
				  << expected_byte
				  << buffers_decoded_str()
				  << std::endl;
			std::exit(1);
		}
	}
}

template <std::size_t page_size>
bool TestBase<page_size>::verify_error()
{
	if (decoder.error())
	{
		if (!expected_error())
		{
			std::cout << name() << " : failed "
				  << "with decoder error : "
				  << decoder.error()
				  << buffers_decoded_str()
				  << std::endl;
			std::exit(1);
		}

		if (std::strcmp(decoder.error(),
				expected_error()))
		{
			std::cout << name() << " : expected "
				  << "error did not occur, "
				  << "instead error was : "
				  << decoder.error()
				  << buffers_decoded_str()
				  << std::endl;
			std::exit(1);
		}

		return true; // error was as expected
	}

	return false;
}

template <std::size_t page_size>
void TestBase<page_size>::unexpected_buffer()
{
	std::cout << name() << " : another buffer "
		  << "was decoded but the test has "
		  << "none left"
		  << buffers_decoded_str()
		  << std::endl;
}

template <std::size_t page_size>
std::string TestBase<page_size>::buffers_decoded_str() const
{
	std::ostringstream oss;
	oss << " (buffers successfully decoded and verified = "
	    << successful_buffers
	    << ')';
	return oss.str();
}

template <std::size_t page_size>
TestBase<page_size>* TestBase<page_size>::current_test_base;

template <std::size_t page_size>
const char* TestBase<page_size>::buffer_full_verify(const I8HEX::Decoder&)
{
	current_test_base->verify_full_raw_buffer();
	return nullptr;
}

template <std::size_t page_size>
const char* TestBase<page_size>::fail_unexpected_buffer(const I8HEX::Decoder&)
{
	current_test_base->unexpected_buffer();
	std::exit(1);
	return nullptr; // as if this will happen
}


// derived test cases from TestBase

#define STR(x) STR_EXPAND(x)
#define STR_EXPAND(x) #x

class Single_buffer1 : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Single buffer 32 bytes partial\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":100020000C94E5030C94E503"
			"0C94E5030C94E503B0\n"
			":00000001FF\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0020, {0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff}}};
	}
};

class Single_buffer2 : public TestBase<64>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Single buffer 64 bytes partial and offset\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":200020001FBECFE5D2E0DEBFCDBF02D002C"
			"0E8CFFFCFF89400C0F894FFCF00000000000095\n"
			":00000001FF\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		// buffer size is 64 (0x40) but first record starts
		// at 0x0020, so that is rounded down to address 0x0000
		return {{0x0000, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				0xff, 0xff,
				0x1F, 0xBE, 0xCF, 0xE5, 0xD2, 0xE0,
				0xDE, 0xBF, 0xCD, 0xBF, 0x02, 0xD0,
				0x02, 0xC0, 0xE8, 0xCF, 0xFF, 0xCF,
				0xF8, 0x94, 0x00, 0xC0, 0xF8, 0x94,
				0xFF, 0xCF, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00}}};
	}
};

class Single_buffer3 : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Single buffer 32 bytes overlays\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":010f300012ae\n"
		        ":010f2100349b\n"
			":010f3f00565b\n"
			":00000001ff\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0f20, {
				0xff, 0x34, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0x12, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0x56}}};
	}
};

class Single_buffer4 : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Single buffer 32 bytes with blanks\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":010f300012ae\n"
			":010f2100349b\n"
			":000f2000d1\n" // blank payload record same page
			":0000000000\n" // blank payload record different page
			":010f3f00565b\n"
			":00000001ff\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0f20, {0xff, 0x34, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0x12, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0x56}}};
	}
};

class Single_buffer5 : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Single buffer 32 bytes final record payload\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":010f300012ae\n"
			":010f2100349b\n"
			":010f3f00565b\n"
			":020f3c01abcd3a\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0f20, {0xff, 0x34, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0x12, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xab, 0xcd, 0xff, 0x56}}};
	}
};

class Single_buffer_checksum_error : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Single buffer 32 bytes "
			"partial and checksum error\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":0F0030000C94E5030C94E503"
			"0C94E5030C94E5A4\n"
			":010030008748\n"
			":010080008748\n"; // checksum error (note address)
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0020, {0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0x87, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0xff}}};
	}

	virtual const char* expected_error() const
	{
		return "Invalid checksum";
	}
};

class No_buffer_checksum_error : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"No buffer checksum error\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":100020000C94E5030C94E503"
			"0C94E5030C94E503A0\n"
			":0100300087FF\n"; // checksum error here
	}

	virtual const char* expected_error() const
	{
		return "Invalid checksum";
	}
};

class Single_buffer_no_colon_error : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Single buffer 32 bytes "
			"partial and no colon error\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":100028000C94E5030C94E503"
			"0C94E5030C94E503A8\n"
			":010028008750\n"
			":0100800087f8\n"
			"0100800087f8\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0020, {0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0x87, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff}}};
	}

	virtual const char* expected_error() const
	{
		return "Invalid character, expected colon";
	}
};

class No_buffer_no_colon_error : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"No buffer no colon error\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":100030000C94E5030C94E503"
			"0C94E5030C94E503A0\n"
			"0100300087FF\n";
	}

	virtual const char* expected_error() const
	{
		return "Invalid character, expected colon";
	}
};

class Single_buffer_invalid_hex_error : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Single buffer 32 bytes "
			"partial invalid checksum digit\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":100028000C94E5030C94E503"
			"0C94E5030C94E503A8\n"
			":010028008750\n"
			":0100800087#8\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0020, {0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0x87, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff}}};
	}

	virtual const char* expected_error() const
	{
		return "Invalid character, expected hexadecimal digit";
	}
};

class No_buffer_invalid_hex_error : public TestBase<32>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"No buffer invalid checksum digit\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":100030000C94E5030C94E5=9"
			"0C94E5030C94E503A0\n";
	}

	virtual const char* expected_error() const
	{
		return "Invalid character, expected hexadecimal digit";
	}
};

class Multi_buffer1 : public TestBase<16>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Multi buffer 16 bytes\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":100020000C94E5030C94E503"
			"0C94E5030C94E503B0\n"
			":100030000C94E5030C94E503"
			"0C94E5030C94E503A0\n"
			":00000001FF\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0020, {0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03}},
			{0x0030, {0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03}}};
	};
};

class Multi_buffer2 : public TestBase<16>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Multi buffer 16 bytes middle overlap\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":100028000C94E5030C94E503"
			"0C94E5030C94E503A8\n"
			":00000001FF\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0020, {0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03}},
			{0x0030, {0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff}}};
	};
};

class Multi_buffer3 : public TestBase<16>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Multi buffer 16 bytes front overlap\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":110020000C94E5030C94E503"
			"0C94E5030C94E5038728\n"
			":00000001FF\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0020, {0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03,
				0x0C, 0x94, 0xE5, 0x03}},
			{0x0030, {0x87, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff}}};
	};
};

class Multi_buffer4 : public TestBase<16>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Multi buffer 16 bytes rear overlap\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":11001F000C94E5030C94E503"
			"0C94E5030C94E5038729\n"
			":00000001FF\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0010, {0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0x0c}},
			{0x0020, {0x94, 0xE5, 0x03, 0x0c,
				0x94, 0xE5, 0x03, 0x0c,
				0x94, 0xE5, 0x03, 0x0c,
				0x94, 0xE5, 0x03, 0x87}}};
	};
};

class Multi_buffer5 : public TestBase<16>
{
	virtual const char* name() const
	{
		return __FILE__ ":" STR(__LINE__)
			" \"Multi buffer 16 bytes various\"";
	}

	virtual const char* get_i8hex() const
	{
		return ":10001F000C94E5030C94E503"
			"0C94E5030C94E503B1 two buffers\n"
			":0100280011C6 tweak one byte in middle\n"
			":00FFFF0002  no data for last byte \n"
			":04100000AabbCCddDE next block\n"
			":280078000123456789abcdef"
			"12123434ababcdcd"
			"fedcba9876543210"
			"abcdabcd12123434"
			"9876543212345678C8 0x28 bytes of joy\n"
			":02123401123471 and a final buffer just bcoz\n";
	}

	virtual std::deque<addr_raw_buffer> get_raw_buffers() const
	{
		return {{0x0010, {0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0x0c}},
			{0x0020, {0x94, 0xE5, 0x03, 0x0c,
				0x94, 0xE5, 0x03, 0x0c,
				0x11, 0xE5, 0x03, 0x0c,
				0x94, 0xE5, 0x03, 0xff}},
			{0x1000, {0xaa, 0xbb, 0xcc, 0xdd,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff}},
			{0x0070, {0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0x01, 0x23, 0x45, 0x67,
				0x89, 0xab, 0xcd, 0xef}},
			{0x0080, {0x12, 0x12, 0x34, 0x34,
				0xab, 0xab, 0xcd, 0xcd,
				0xfe, 0xdc, 0xba, 0x98,
				0x76, 0x54, 0x32, 0x10}},
			{0x0090, {0xab, 0xcd, 0xab, 0xcd,
				0x12, 0x12, 0x34, 0x34,
				0x98, 0x76, 0x54, 0x32,
				0x12, 0x34, 0x56, 0x78}},
			{0x1230, {0xff, 0xff, 0xff, 0xff,
				0x12, 0x34, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff,
				0xff, 0xff, 0xff, 0xff}}};
	};
};

int main()
{
	Single_buffer1().run();
	Single_buffer2().run();
	Single_buffer3().run();
	Single_buffer4().run();
	Single_buffer5().run();

	Single_buffer_checksum_error().run();
	No_buffer_checksum_error().run();
	Single_buffer_no_colon_error().run();
	No_buffer_no_colon_error().run();
	Single_buffer_invalid_hex_error().run();
	No_buffer_invalid_hex_error().run();

	Multi_buffer1().run();
	Multi_buffer2().run();
	Multi_buffer3().run();
	Multi_buffer4().run();
	Multi_buffer5().run();

	return 0;
}
