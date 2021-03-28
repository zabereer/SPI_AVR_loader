#ifndef HIGH_VOLT_PROGRAMMER_HPP
#define HIGH_VOLT_PROGRAMMER_HPP

#include"spi_programmer.hpp"

namespace high_volt_programmer
{
	// Will prompt via serial interface, 12V source must not be connected
	// yet when calling this function.
	// Set inverted_high_voltage_level_shifter to true if pin 8 is
	// connected to base of NPN transistor used in inverted level shifter.
	void read_fuses(const bool inverted_high_voltage_level_shifter,
			spi_programmer::fuses_t&);

	// Will prompt via serial interface, 12V source must not be connected
	// yet when calling this function.
	// Set inverted_high_voltage_level_shifter to true if pin 8 is
	// connected to base of NPN transistor used in inverted level shifter.
	void write_fuses(const bool inverted_high_voltage_level_shifter,
			 const spi_programmer::fuses_t&);
}

#endif
