#include "high_volt_programmer.hpp"

#include<avr/pgmspace.h>
#include<HardwareSerial.h>

#include"util.hpp"

namespace
{
	void initial_set_pin_8(const bool inverted_high_voltage_level_shifter)
	{
		pinMode(8, OUTPUT);
		Serial.print(F("Pin #8 set to "));
		if (inverted_high_voltage_level_shifter)
		{
			digitalWrite(8, HIGH);
			Serial.print(F("HIGH"));
		}
		else
		{
			digitalWrite(8, LOW);
			Serial.print(F("LOW"));
		}
		Serial.println(F(" to disable external 12V source"));
	}

	bool continue_with_hvsp()
	{
		Serial.println(F("Connect external 12V power source now"));
		Serial.println(F("Press y to continue, n to abandon\n"));
		char go_for_hvsp = util::serial_read_char_of("yn");
		if (go_for_hvsp == 'n')
			Serial.println(F("its ok to be chicken"));
		return go_for_hvsp == 'y';
	}

	void finish_with_hvsp()
	{
		Serial.println(F("Disconnect external 12V power source now"));
		Serial.println(
			F("Press any key to continue once disconnected"));
		util::serial_read_char();
		digitalWrite(8, LOW);
	}

	// our pins with names from target perspective
	constexpr int VCC = 9;
	constexpr int SCI = 10;
	constexpr int SDI = 11;
	constexpr int SII = 12;
	constexpr int SDO = 13;

	void pins_output_low()
	{
		static constexpr int pins[]{VCC, SCI, SDI, SII, SDO};
		for (int pin : pins)
		{
			pinMode(pin, OUTPUT);
			digitalWrite(pin, LOW);
		}
	}

	void enable_programming(const bool inverted_high_voltage_level_shifter)
	{
		pins_output_low();

		// power on and enter high voltage serial programming
		digitalWrite(VCC, HIGH);
		// delay(100); // oddly a longer delay is sometimes required
		delayMicroseconds(40); // required delay is between 20-60 us
		digitalWrite(8,        // enable 12V source
			     inverted_high_voltage_level_shifter ? LOW : HIGH);
		delayMicroseconds(10); // ensure Prog_enable signature latched
		pinMode(SDO, INPUT);   // release SDO (output from target)
		delayMicroseconds(300);
		noInterrupts();
	}

	void disable_programming(const bool inverted_high_voltage_level_shifter)
	{
		interrupts();
		pins_output_low();
		digitalWrite(8, // disable 12V source
			     inverted_high_voltage_level_shifter ? HIGH : LOW);
	}

	void crude_delay_loop(uint16_t count)
	{
		while (count--)
		{
			volatile uint8_t inner_count = 0x10;
			while (inner_count--)
				asm volatile ("" : : : "memory");
		}
	}

	uint8_t shift_data_inst(uint8_t data,
				uint8_t inst,
				const uint8_t crude_delay)
	{
		uint8_t res = 0;
		digitalWrite(SDI, LOW);
		digitalWrite(SII, LOW);
		crude_delay_loop(crude_delay);
		digitalWrite(SCI, HIGH);
		crude_delay_loop(crude_delay);
		digitalWrite(SCI, LOW);
		for (int b = 0; b < 8; ++b)
		{
			bool i = digitalRead(SDO) == HIGH;
			digitalWrite(SDI, (data & 0x80) == 0x80 ? HIGH : LOW);
			digitalWrite(SII, (inst & 0x80) == 0x80 ? HIGH : LOW);
			data <<= 1;
			inst <<= 1;
			res <<= 1;
			res |= i;
			crude_delay_loop(crude_delay);
			digitalWrite(SCI, HIGH);
			crude_delay_loop(crude_delay);
			digitalWrite(SCI, LOW);
		}
		crude_delay_loop(crude_delay);
		digitalWrite(SDI, LOW);
		digitalWrite(SII, LOW);
		crude_delay_loop(crude_delay);
		digitalWrite(SCI, HIGH);
		crude_delay_loop(crude_delay);
		digitalWrite(SCI, LOW);
		crude_delay_loop(crude_delay);
		digitalWrite(SCI, HIGH);
		crude_delay_loop(crude_delay);
		digitalWrite(SCI, LOW);
		return res;
	}
}

void high_volt_programmer::read_fuses(
	const bool inverted_high_voltage_level_shifter,
	spi_programmer::fuses_t& fuses,
	const uint8_t crude_delay)
{
	Serial.println(F("About to read fuses using high voltage"));
	initial_set_pin_8(inverted_high_voltage_level_shifter);
	if (continue_with_hvsp())
	{
		Serial.println(F("reading fuses"));
		enable_programming(inverted_high_voltage_level_shifter);

		shift_data_inst(0x00, 0x4c, crude_delay); // NOP

		shift_data_inst(0x04, 0x4c, crude_delay);
		shift_data_inst(0x00, 0x78, crude_delay);
		fuses.lock = shift_data_inst(0x00, 0x7c, crude_delay);

		shift_data_inst(0x00, 0x4c, crude_delay); // NOP

		shift_data_inst(0x04, 0x4c, crude_delay);
		shift_data_inst(0x00, 0x68, crude_delay);
		fuses.low = shift_data_inst(0x00, 0x6c, crude_delay);

		shift_data_inst(0x00, 0x4c, crude_delay); // NOP

		shift_data_inst(0x04, 0x4c, crude_delay);
		shift_data_inst(0x00, 0x7a, crude_delay);
		fuses.high = shift_data_inst(0x00, 0x7e, crude_delay);

		shift_data_inst(0x00, 0x4c, crude_delay); // NOP

		shift_data_inst(0x04, 0x4c, crude_delay);
		shift_data_inst(0x00, 0x6a, crude_delay);
		fuses.ext = shift_data_inst(0x00, 0x6e, crude_delay);

		disable_programming(inverted_high_voltage_level_shifter);
	}
	finish_with_hvsp();
}

void high_volt_programmer::write_fuses(
	const bool inverted_high_voltage_level_shifter,
	const spi_programmer::fuses_t& fuses,
	const uint8_t crude_delay)
{
	Serial.println(F("About to write fuses using high voltage"));
	spi_programmer::output_fuses(fuses);
	initial_set_pin_8(inverted_high_voltage_level_shifter);
	if (continue_with_hvsp())
	{
		Serial.println(F("writing fuses"));
		enable_programming(inverted_high_voltage_level_shifter);

		shift_data_inst(0x20, 0x4c, crude_delay);
		shift_data_inst(fuses.lock & 3, 0x2c, crude_delay);
		shift_data_inst(0x00, 0x64, crude_delay);
		shift_data_inst(0x00, 0x6c, crude_delay);
		while (digitalRead(SDO) != HIGH);

		shift_data_inst(0x40, 0x4c, crude_delay);
		shift_data_inst(fuses.low, 0x2c, crude_delay);
		shift_data_inst(0x00, 0x64, crude_delay);
		shift_data_inst(0x00, 0x6c, crude_delay);
		while (digitalRead(SDO) != HIGH);

		shift_data_inst(0x40, 0x4c, crude_delay);
		shift_data_inst(fuses.high, 0x2c, crude_delay);
		shift_data_inst(0x00, 0x74, crude_delay);
		shift_data_inst(0x00, 0x7c, crude_delay);
		while (digitalRead(SDO) != HIGH);

		shift_data_inst(0x40, 0x4c, crude_delay);
		shift_data_inst(fuses.ext & 1, 0x2c, crude_delay);
		shift_data_inst(0x00, 0x66, crude_delay);
		shift_data_inst(0x00, 0x6e, crude_delay);
		while (digitalRead(SDO) != HIGH);

		disable_programming(inverted_high_voltage_level_shifter);
	}
	finish_with_hvsp();
}

void high_volt_programmer::chip_erase(
	const bool inverted_high_voltage_level_shifter,
	const uint8_t crude_delay)
{
	Serial.println(F("About to perform chip erase using high voltage"));
	initial_set_pin_8(inverted_high_voltage_level_shifter);
	if (continue_with_hvsp())
	{
		Serial.println(F("erasing chip"));
		enable_programming(inverted_high_voltage_level_shifter);

		shift_data_inst(0x80, 0x4c, crude_delay);
		shift_data_inst(0x00, 0x64, crude_delay);
		shift_data_inst(0x00, 0x6c, crude_delay);
		while (digitalRead(SDO) != HIGH);
		shift_data_inst(0x00, 0x4c, crude_delay); // NOP required

		disable_programming(inverted_high_voltage_level_shifter);
	}
	finish_with_hvsp();
}
