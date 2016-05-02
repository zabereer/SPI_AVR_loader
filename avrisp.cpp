#include"I8HEX_decoder.hpp"
#include"images.hpp"
#include"spi_programmer.hpp"
#include"util.hpp"

#include"Arduino.h"
#include"avr/pgmspace.h"
#include"HardwareSerial.h"

namespace spipgm = spi_programmer;

/*
  Output an ATtinyXX AVR chip flash in I8HEX format to serial.
  Input from serial an I8HEX image and load into ATtinyXX AVR chip.

  Connections when using Arduino UNO as ISP for Adafruit Trinket
  +===============+===============+===============+
  |    Trinket	  |  Arduino Uno  |	SPI	  |
  +===============+===============+===============+
  |	VBAT	  |   3V / 5V	  |	Vcc	  |
  +---------------+---------------+---------------+
  |	RST	  |	 #10	  |	 SS	  |
  +---------------+---------------+---------------+
  |	 #0	  |	 #11	  |	MOSI	  |
  +---------------+---------------+---------------+
  |	 #1	  |	 #12	  |	MISO	  |
  +---------------+---------------+---------------+
  |	 #2	  |	 #13	  |	SCK	  |
  +---------------+---------------+---------------+
 */

namespace
{
	bool verbose = false;
	bool perform_load_image = false; // used by load_image
}

void setup()
{
	Serial.begin(9600);
	Serial.println(F("\nExperimental SPI programmer\n"));
}

// enable SPI programming of target device, return clock rate
uint32_t enable_programming(bool verbose = false)
{
	uint32_t clock_rate = 8000000;
	while (!spipgm::program_enable(
		       SPISettings(clock_rate, MSBFIRST, SPI_MODE0),
		       2, // retries
		       verbose))
	{
		Serial.print(F("Failed to enable programming at clock "));
		Serial.print(clock_rate);
		clock_rate /= 2;
		if (clock_rate < 5)
		{
			spipgm::powerdown_avr();
			spipgm::failure(F("\nunable to enable programming"));
			// unreachable, spipgm::failure does not return
		}
		Serial.print(F(" - reducing to "));
		Serial.println(clock_rate);
		delay(250);
	}

	if (verbose)
	{
		Serial.print(F("Programming enabled at clock rate "));
		Serial.print(clock_rate);
	}

	return clock_rate;
}

void toggle_verbose()
{
	verbose = !verbose;
	Serial.print(F("verbose set "));
	if (verbose)
		Serial.println(F("ON"));
	else
		Serial.println(F("OFF"));
}

void display_device_signature()
{
	spipgm::powerup_avr();
	enable_programming(verbose);
	uint32_t sig = spipgm::read_signature(verbose);
	Serial.print(F("Device signature "));
	Serial.print(sig, HEX);
	spipgm::program_disable();
	spipgm::powerdown_avr();
}

void write_fuses()
{
	// device already powered up
	spipgm::fuses_t fuses = spipgm::read_fuses(verbose);
	bool done = false;
	do
	{
		spipgm::output_fuses(fuses);
		Serial.println(F("Think carefully, don't brick your AVR"));
		Serial.println(F("k - modify lock fuse"));
		Serial.println(F("l - modify low fuse"));
		Serial.println(F("h - modify high fuse"));
		Serial.println(F("x - modify ext fuse"));
		Serial.println(F("w - write fuses"));
		Serial.println(F("q - quit write fuses menu"));

		char c = util::serial_read_char_of("klhxwq");
		Serial.println();
		switch (c)
		{
		case 'k' :
			fuses.lock = util::serial_read_hex_byte();
			break;
		case 'l' :
			fuses.low =util::serial_read_hex_byte();
			break;
		case 'h' :
			fuses.high = util::serial_read_hex_byte();
			break;
		case 'x' :
			fuses.ext =util::serial_read_hex_byte();
			break;
		case 'w' :
			spipgm::write_verify_fuses(fuses, verbose);
		case 'q' :
			done = true;
			break;
		}
	} while (!done);
}

void read_write_fuses()
{
	spipgm::powerup_avr();
	enable_programming(verbose);

	bool done = false;
	do
	{
		Serial.println(F("=== Fuses ==="));
		Serial.println(F("r - read fuses"));
		Serial.println(F("w - write fuses"));
		Serial.println(F("q - quite fuses menu"));

		char c = util::serial_read_char_of("rwq");
		Serial.println();
		switch (c)
		{
		case 'r' :
			spipgm::output_fuses(spipgm::read_fuses(verbose));
			break;
		case 'w' :
			write_fuses();
			break;
		case 'q' :
			done = true;
			break;
		}
	} while (!done);

	spipgm::program_disable();
	spipgm::powerdown_avr();
}

const spipgm::device_pgm_t* get_signature_flash_page_sizes(uint32_t& sig,
							   uint16_t& flash,
							   uint16_t& page)
{
	sig = spipgm::read_signature(verbose);
	const spipgm::device_pgm_t* dev_ptr =
		images::device_for_signature(sig);

	flash = 0;
	page = 0;

	if (dev_ptr)
	{
		Serial.print(F("CPU "));
		Serial.println(util::FF(&dev_ptr->device_name));
		flash = pgm_read_word(&dev_ptr->flash_size);
		page = pgm_read_word(&dev_ptr->page_size);
	}
	else
	{
		Serial.print(F("Unknown CPU - signature was "));
		Serial.println(sig, HEX);
		Serial.println(F("\nSelect flash size in bytes\n"));
		flash = util::serial_read_power_of_two(1024);
		if (flash)
		{
			Serial.println(F("\nSelect page size in bytes\n"));
			page = util::serial_read_power_of_two();
		}
	}

	return dev_ptr;
}

void chip_erase()
{
	Serial.println(F("chip erase! are you sure? (y/n)"));
	char y = util::serial_read_char_of("yn");
	if (y == 'y')
	{
		spipgm::powerup_avr();
		enable_programming(verbose);
		Serial.print(F("erasing..."));
		spipgm::wait_device_ready();
		spipgm::perform_chip_erase(verbose);
		spipgm::wait_device_ready();
		Serial.println(F("done"));
		spipgm::program_disable();
		spipgm::powerdown_avr();
	}
	else
	{
		Serial.println(F("phew, not erased"));
	}
}

namespace
{
	const PROGMEM char directive_sig[] = ";sig";
	const PROGMEM char directive_fuses[] = ";fuses";
	const PROGMEM char directive_end[] = ";end";
}

void output_backup_image()
{
	spipgm::powerup_avr();
	enable_programming(verbose);

	uint32_t sig;
	uint16_t flash_size;
	uint16_t page_size;
	const spipgm::device_pgm_t* dev_ptr =
		get_signature_flash_page_sizes(sig, flash_size, page_size);

	if (flash_size && page_size)
	{
		spipgm::fuses_t fuses = spipgm::read_fuses(false);

		Serial.print(F("  flash="));
		Serial.print(flash_size);
		Serial.print(F("  page size="));
		Serial.println(page_size);

		Serial.println(F("\nCopy and paste lines "
				 "below starting with ':' and ';'"));

		Serial.print(util::FF(directive_sig));
		Serial.print(' ');
		Serial.print(sig, HEX);
		Serial.print(F("  ("));
		if (dev_ptr)
			Serial.print(util::FF(&dev_ptr->device_name));
		else
			Serial.print(F("unknown"));
		Serial.println(')');

		Serial.print(util::FF(directive_fuses));
		Serial.print(' ');
		Serial.print(fuses.lock, HEX);
		Serial.print(' ');
		Serial.print(fuses.low, HEX);
		Serial.print(' ');
		Serial.print(fuses.high, HEX);
		Serial.print(' ');
		Serial.print(fuses.ext, HEX);
		Serial.println(F("  (lock low high ext)"));

		uint16_t addr = 0;
		uint8_t bytes = page_size < 32 ? page_size : 32;
		for (; addr < flash_size; addr += bytes)
		{
			char data[bytes];
			spipgm::read_program_memory(addr, data, bytes);
			util::serial_write_I8HEX(addr, data, bytes);
		}
		Serial.println(F(":00000001FF"));
		Serial.println(util::FF(directive_end));

		Serial.println(F("\nCopy and paste lines "
				 "above starting with ':' and ';'"));
	}

	spipgm::program_disable();
	spipgm::powerdown_avr();
}

void drain_serial()
{
	while (Serial.available())
		Serial.read();
}

void drain_serial_to_nl(char& last_char)
{
	while (last_char != '\n')
	{
		last_char = util::serial_read_char();
	}
}

void serial_print_error()
{
	Serial.println(F("\n***error***\n"));
}

// verify serial reads return the value of directive_pgm in program memory
// from its 2nd offset (3rd character) up to just before terminating nil
bool verify_directive_from_offset2(const char* directive_pgm,
				   size_t directive_pgm_size)
{
	Serial.println(util::FF(directive_pgm));
	--directive_pgm_size;
	for (size_t ix = 2; ix < directive_pgm_size; ++ix)
	{
		const char c = util::serial_read_char();
		if (pgm_read_byte(&directive_pgm[ix]) != c)
		{
			serial_print_error();
			Serial.print(F("\nInvalid directive, expecting "));
			Serial.println(util::FF(directive_pgm));
			return false;
		}
	}
	return true;
}

bool verify_expected_signature_serial(const uint32_t sig, char& last_char)
{
	uint32_t expected_sig;
	if (util::serial_read_value(expected_sig, last_char))
	{
		if (sig == expected_sig)
		{
			return true;
		}
		else
		{
			serial_print_error();
			Serial.println(F("Device signature mismatch"));
			return false;
		}
	}
	serial_print_error();
	Serial.println(F("Unable to verify device signature"));
	return false;
}

bool set_fuses_from_serial(char& last_char)
{
	spipgm::fuses_t fuses;
	return util::serial_read_value(fuses.lock, last_char) &&
		util::serial_read_value(fuses.low, last_char) &&
		util::serial_read_value(fuses.high, last_char) &&
		util::serial_read_value(fuses.ext, last_char) &&
		spipgm::write_verify_fuses(fuses, verbose);
}

bool process_load_directive(const uint32_t sig)
{
	bool done = true;

	char last_char = util::serial_read_char();
	switch (last_char)
	{
	case 's' :
		if (verify_directive_from_offset2(directive_sig,
						  sizeof(directive_sig)))
		{
			if (verify_expected_signature_serial(sig, last_char))
				perform_load_image = false;
		}
		break;
	case 'f' :
		if (verify_directive_from_offset2(directive_fuses,
						  sizeof(directive_fuses)))
		{
			if (set_fuses_from_serial(last_char))
				perform_load_image = false;
		}
		break;
	case 'e' :
		if (verify_directive_from_offset2(directive_end,
						  sizeof(directive_end)))
		{
			spipgm::wait_device_ready();
			Serial.println("## done ##");
		}
		break;
	default:
		serial_print_error();
		Serial.println(F("\nInvalid input, expected directive\n"));
	}

	drain_serial_to_nl(last_char);
	return done;
}

bool process_i8hex_directive(char i8hex_buffer[],
			     const size_t i8hex_buffer_size,
			     const uint16_t flash_size,
			     I8HEX::Decoder& decoder)
{
	// read more data to follow the colon
	size_t bytes_in_buffer = util::serial_read_until_nl(
		&i8hex_buffer[1], // append after initial ':'
		i8hex_buffer_size - 1) + 1; // account for ':'

	do
	{
		const bool nl_in_buffer =
			i8hex_buffer[bytes_in_buffer - 1] == ':';
		decoder.decode(&i8hex_buffer[0], bytes_in_buffer);
		if (nl_in_buffer || decoder.done() || decoder.error())
			break;

		// read more data
		bytes_in_buffer = util::serial_read_until_nl(
			&i8hex_buffer[0],
			i8hex_buffer_size);

	} while (true);

	if (decoder.error())
	{
		Serial.println(F("I8HEX decode failed:"));
		Serial.println(decoder.error());
		perform_load_image = false;
	}

	return decoder.done() || decoder.error();
}

// called whenever a I8HEX buffer is decoded into raw
const char* decoded_full_buffer(const I8HEX::Decoder& decoder)
{
	if (perform_load_image)
	{
		spipgm::wait_device_ready();
		spipgm::load_program_memory(
			decoder.get_buffer_address_on_target(),
			decoder.buffer,
			decoder.page_size,
			verbose);
		spipgm::write_program_page(
			decoder.get_buffer_address_on_target(),
			verbose);
	}
	return nullptr;
}

void load_image()
{
	spipgm::powerup_avr();
	enable_programming(verbose);

	uint32_t sig;
	uint16_t flash_size;
	uint16_t page_size;
	get_signature_flash_page_sizes(sig, flash_size, page_size);
	perform_load_image = true;

	if (flash_size && page_size)
	{
		char target_buffer[page_size];
		I8HEX::Decoder decoder(target_buffer,
				       page_size,
				       &decoded_full_buffer);
		char i8hex_buffer[100];
		Serial.println(F("Paste image below"));
		bool done = false;
		do
		{
			i8hex_buffer[0] = util::serial_read_char_of(":;");
			if (i8hex_buffer[0] == ':')
			{
				done = process_i8hex_directive(
					&i8hex_buffer[0],
					sizeof(i8hex_buffer),
					flash_size,
					decoder);
			}
			else
			{
				done = process_load_directive(sig);
			}
		} while (!done);

		drain_serial();
	}

	perform_load_image = false;
	spipgm::program_disable();
	spipgm::powerdown_avr();
}

void loop()
{
	Serial.println(F("=== Main menu ==="));
	Serial.print(F("v - toggle verbose (current "));
	Serial.print(verbose ? 'Y' : 'N');
	Serial.println(F(")"));
	Serial.println(F("s - display device signature"));
	Serial.println(F("f - read/write fuses"));
	Serial.println(F("b - read flash (backup) to serial"));
	Serial.println(F("e - chip erase (seems required before load)"));
	Serial.println(F("l - write flash from serial (load target)"));

	char c = util::serial_read_char_of("vsfbel");
	Serial.println();
	switch (c)
	{
	case 'v' :
		toggle_verbose();
		break;
	case 's':
		display_device_signature();
		break;
	case 'f' :
		read_write_fuses();
		break;
	case 'b' :
		output_backup_image();
		break;
	case 'e' :
		chip_erase();
		break;
	case 'l' :
		load_image();
		break;
	}
}
