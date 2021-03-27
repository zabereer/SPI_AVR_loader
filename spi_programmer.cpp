#include"spi_programmer.hpp"
#include"util.hpp"

#include"Arduino.h"
#include"HardwareSerial.h"

bool spi_programmer::device_pgm_t::verify_signature(bool verbose) const
{
	const uint32_t sig = read_signature(verbose);
	const uint32_t expected = get_expected_signature();
	if (sig != expected)
	{
		Serial.print(F("Device signature read "));
		Serial.print(sig, HEX);
		Serial.print(F(" does not match expected "));
		Serial.println(expected, HEX);
		return false;
	}
	else if (verbose)
	{
		Serial.print(F("Device signature "));
		Serial.print(sig, HEX);
		Serial.println(F(" matched"));
	}
	return true;
}

bool spi_programmer::device_pgm_t::write_programming_fuses(bool verbose) const
{
	return write_verify_fuses(programming_fuses, verbose);
}

bool spi_programmer::device_pgm_t::write_default_fuses(bool verbose) const
{
	return write_verify_fuses(default_fuses, verbose);
}

void spi_programmer::powerup_avr()
{
	delay(100);
	pinMode(SCK, OUTPUT);
	pinMode(SS, OUTPUT);
	digitalWrite(SCK, LOW);
	digitalWrite(SS, LOW);
	delay(100);
}

void spi_programmer::powerdown_avr()
{
	digitalWrite(SS, HIGH);
}

bool spi_programmer::program_enable(SPISettings spi_settings,
				    int retries,
				    bool verbose)
{
	SPI.begin();
	SPI.beginTransaction(spi_settings);

	bool success;
	do
	{
		uint8_t sync = spi_trans(0xAC, 0x53, 0x00, 0x00,
					 verbose) >> 8;
		success = sync == 0x53;
		if (!success)
		{
			if (verbose)
			{
				Serial.print(
					F("program enable received sync 0x"));
				Serial.print(sync, HEX);
				Serial.print(F(" instead of 0x53 ("));
				Serial.print(retries);
				Serial.println(F(" remaining retries)"));
			}
			digitalWrite(SS, HIGH);
			delay(50);
			digitalWrite(SS, LOW);
			delay(50);
		}
	} while (!success && --retries >= 0);

	if (!success)
		program_disable();

	return success;
}

void spi_programmer::program_disable()
{
	SPI.end();
}

bool spi_programmer::write_image_pgm(const spi_programmer::image_pgm_t& image,
				     bool verbose)
{
	const device_pgm_t* device =
		reinterpret_cast<const device_pgm_t*>(
			pgm_read_ptr(&image.device));
	if (!write_verify_fuses(device->programming_fuses, verbose))
	{
		Serial.println(F("Failed to set programming fuses"));
		return false;
	}

	return true;
}

bool spi_programmer::verify_signature(
	const spi_programmer::image_pgm_t& image,
	bool verbose)
{
	const device_pgm_t* device =
		reinterpret_cast<const device_pgm_t*>(
			pgm_read_ptr(&image.device));
	const uint32_t exp_sig = pgm_read_dword(&device->expected_signature);
	const uint32_t sig = read_signature(verbose);
	if (sig != exp_sig)
	{
		Serial.print(F("Device signature read "));
		Serial.print(sig, HEX);
		Serial.print(F(" does not match expected "));
		Serial.println(exp_sig, HEX);
		return false;
	}
	else if (verbose)
	{
		Serial.print(F("Device signature "));
		Serial.print(sig, HEX);
		Serial.print(F(" matched"));
	}
	return true;
}

uint32_t spi_programmer::read_signature(bool verbose)
{
	if (verbose)
		Serial.println(F("Reading signature"));
	uint32_t sig   = spi_trans(0x30, 0x00, 0x00, 0x00, verbose);
	sig = sig << 8 | spi_trans(0x30, 0x00, 0x01, 0x00, verbose);
	sig = sig << 8 | spi_trans(0x30, 0x00, 0x02, 0x00, verbose);
	if (verbose)
	{
		Serial.print(F("Read signature "));
		Serial.println(sig, HEX);
	}
	return sig;
}

spi_programmer::fuses_t spi_programmer::read_fuses(bool verbose)
{
	if (verbose)
		Serial.println(F("Reading fuses"));
	fuses_t fuses{
		static_cast<uint8_t>(
			spi_trans(0x58, 0x00, 0x00, 0x00, verbose)),
		static_cast<uint8_t>(
			spi_trans(0x50, 0x00, 0x00, 0x00, verbose)),
		static_cast<uint8_t>(
			spi_trans(0x58, 0x08, 0x00, 0x00, verbose)),
		static_cast<uint8_t>(
			spi_trans(0x50, 0x08, 0x00, 0x00, verbose))
		};

	return fuses;
}

void spi_programmer::write_fuses(const spi_programmer::fuses_t& fuses,
				 bool verbose)
{
	if (verbose)
	{
		Serial.print(F("Writing fuses: "));
		output_fuses(fuses);
	}
	spi_trans(0xAC, 0xE0, 0x00, fuses.lock, verbose);
	wait_device_ready();
	spi_trans(0xAC, 0xA0, 0x00, fuses.low, verbose);
	wait_device_ready();
	spi_trans(0xAC, 0xA8, 0x00, fuses.high, verbose);
	wait_device_ready();
	spi_trans(0xAC, 0xA4, 0x00, fuses.ext, verbose);
	wait_device_ready();
}

bool spi_programmer::write_verify_fuses(
	const spi_programmer::fuses_t& fuses,
	bool verbose)
{
	write_fuses(fuses, verbose);
	fuses_t readfuses = read_fuses(verbose);
	if (readfuses != fuses)
	{
		Serial.println(F("Failed to write fuses, "
				 "tried to write:"));
		output_fuses_pgm(fuses);
		Serial.println(F("but read fuses back as:"));
		output_fuses(readfuses);
		return false;
	}
	return true;
}

void spi_programmer::read_program_memory(uint16_t address, void *buffer,
					 size_t bytes, bool verbose)
{
	char* bufptr = reinterpret_cast<char*>(buffer);
	address >>= 1;
	for (; bytes; ++address, bytes -= 2)
	{
		*bufptr++ = spi_trans(0x20,
				      address >> 8,
				      address & 0xff,
				      0x00,
				      verbose);
		*bufptr++ = spi_trans(0x28,
				      address >> 8,
				      address & 0xff,
				      0x00,
				      verbose);
	}
}

void spi_programmer::load_program_memory(uint16_t address,
					 const void *buffer,
					 size_t bytes,
					 bool verbose)
{
	const char* bufptr = reinterpret_cast<const char*>(buffer);
	address >>= 1;
	for (; bytes; ++address, bytes -= 2)
	{
		spi_trans(0x40,
			  address >> 8,
			  address & 0xff,
			  *bufptr++,
			  verbose);
		spi_trans(0x48,
			  address >> 8,
			  address & 0xff,
			  *bufptr++,
			  verbose);
	}
}

void spi_programmer::write_program_page(uint16_t address, bool verbose)
{
	address >>= 1;
	spi_trans(0x4c, address >> 8, address & 0xff, 0x00, verbose);
}

uint16_t spi_programmer::spi_trans(uint8_t a, uint8_t b,
				   uint8_t c, uint8_t d,
				   bool verbose)
{
	if (verbose)
	{
		Serial.println(F("SPI transaction"));
		Serial.print(F("byte1 = "));
		Serial.print(a, HEX);
		Serial.print(F(" byte2 = "));
		Serial.print(b, HEX);
		Serial.print(F(" byte3 = "));
		Serial.print(c, HEX);
		Serial.print(F(" byte4 = "));
		Serial.println(d, HEX);
	}

	uint8_t r1 = SPI.transfer(a);
	uint8_t r2 = SPI.transfer(b);
	uint8_t r3 = SPI.transfer(c);
	uint8_t r4 = SPI.transfer(d);

	if (verbose)
	{
		Serial.print(F(" result1 = "));
		Serial.print(r1, HEX);
		Serial.print(F(" result2 = "));
		Serial.print(r2, HEX);
		Serial.print(F(" result3 = "));
		Serial.print(r3, HEX);
		Serial.print(F(" result4 = "));
		Serial.println(r4, HEX);
	}

	return r3 << 8 | r4;
}

bool spi_programmer::device_busy()
{
	return spi_trans(0xF0, 0x00, 0x00, 0x00, false) & 0x01;
}

void spi_programmer::wait_device_ready()
{
	while (device_busy())
		delay(1);
}

void spi_programmer::output_fuses(const spi_programmer::fuses_t& fuses)
{
	Serial.print(F("lock = 0x"));
	Serial.print(fuses.lock, HEX);
	Serial.print(F("  low = 0x"));
	Serial.print(fuses.low, HEX);
	Serial.print(F("  high = 0x"));
	Serial.print(fuses.high, HEX);
	Serial.print(F("  ext = 0x"));
	Serial.println(fuses.ext, HEX);
}

void spi_programmer::output_fuses_pgm(const spi_programmer::fuses_t& fuses)
{
	fuses_t localfuses;
	memcpy_P(&localfuses, &fuses, sizeof(localfuses));
	output_fuses(localfuses);
}

void spi_programmer::perform_chip_erase(bool verbose)
{
	spi_trans(0xAC, 0x80, 0, 0, verbose);
}

void spi_programmer::failure(const char* error)
{
	Serial.println(error);
	while (true)
		delay(1000);
}

void spi_programmer::failure(const __FlashStringHelper* error)
{
	Serial.println(error);
	while (true)
		delay(1000);
}
